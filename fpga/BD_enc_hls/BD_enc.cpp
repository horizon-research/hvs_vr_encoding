#include "BD_enc.h"

void bd_enc(hls::stream<dma_t> &outs, hls::stream<SixteenPixel_t> &ins) {
    #pragma HLS INTERFACE axis port=outs
    #pragma HLS INTERFACE axis port=ins
    #pragma HLS AGGREGATE compact=bit variable=outs
    #pragma HLS AGGREGATE compact=bit variable=ins
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS dataflow

    hls::stream<pack_info_t> pack_infos("pack_infos");
    #pragma HLS STREAM variable=pack_info depth=8
    #pragma HLS AGGREGATE compact=bit variable=pack_info

    hls::stream<Pixel_t> s_ins("s_ins");
    #pragma HLS STREAM variable=s_ins depth=8
    #pragma HLS AGGREGATE compact=bit variable=s_ins

    serializer(s_ins, ins);
    encoder(pack_infos, s_ins);
    packer(outs, pack_infos);
}

void serializer(hls::stream<Pixel_t> &s_ins, hls::stream<SixteenPixel_t> &ins) {
    #ifndef  __SYNTHESIS__
    for (int i = 0; i < 1080 * 960 /16 ; i++) {
    #else

        SixteenPixel_t in;
        in = ins.read();
        for (int j = 0; j < 16; j++) {
            #pragma HLS PIPELINE II=1 rewind
            s_ins.write(in.data[j]);
        }

    #ifndef  __SYNTHESIS__
    }
    #endif
}

void encoder(hls::stream<pack_info_t> &pack_infos, hls::stream<Pixel_t> &ins) {
    #ifndef  __SYNTHESIS__
    for (int i = 0; i < 1080 * 960 /16 ; i++) {
    #else
        #pragma HLS DATAFLOW
        hls::stream<Pixel_t> ins2("ins2");
        #pragma HLS STREAM variable=ins2 depth=16
        hls::stream<Pixel_t> mins("mins");
        hls::stream<Pixel_t> maxs("maxs");
        find_min_max(mins, maxs, ins2, ins);

        hls::stream<pack_info_t> pack_infos("pack_infos"); // max = base (24) + bitlen (12) + delta (24) = 60
        get_encoded_data(pack_infos, mins, maxs, ins2);

        packer(outs, pack_infos);
    #ifndef  __SYNTHESIS__
    }
    #endif
}

void find_min_max(hls::stream<Pixel_t> &mins, hls::stream<Pixel_t> &maxs, hls::stream<Pixel_t> &ins2, hls::stream<Pixel_t> &ins) {
        Pixel_t min;
        Pixel_t max;
        for (int j = 0; j < 16; j++) {
            #pragma HLS PIPELINE II=1 rewind
            Pixel_t in = ins.read();
            if (j == 0) {
                min = in;
                max = in;
            }
            else {
                ins2.write(in);
                if (in.b < min.b) {
                    min.b = in.b;
                }
                if (in.g < min.g) {
                    min.g = in.g;
                }
                if (in.r < min.r) {
                    min.r = in.r;
                }
                if (in.b > max.b) {
                    max.b = in.b;
                }
                if (in.g > max.g) {
                    max.g = in.g;
                }
                if (in.r > max.r) {
                    max.r = in.r;
                }
            }
            if (j == 15) {
                mins.write(min);
                maxs.write(max);
            }
        }
}

void get_encoded_data(hls::stream<pack_info_t> &pack_infos, hls::stream<Pixel_t> &mins, hls::stream<Pixel_t> &maxs, hls::stream<Pixel_t> &ins2) {
    Pixel_t min = mins.read();
    Pixel_t max = maxs.read();
    for (int j = 0; j < 16; j++) {
        #pragma HLS PIPELINE II=1 rewind
        Pixel_t in = ins2.read();
        pack_info_t pack_info;
        pack_info.base = min;
        pack_info.delta.b = in.b - min.b;
        pack_info.delta.g = in.g - min.g;
        pack_info.delta.r = in.r - min.r;
        pack_info.bitlens.b = my_log2(max.b - min.b);
        pack_info.bitlens.g = my_log2(max.g - min.g);
        pack_info.bitlens.r = my_log2(max.r - min.r);
        pack_infos.write(pack_info);
    }
}

ap_uint<4> my_log2(ap_uint<8> x) {
    #pragma HLS INLINE
    ap_uint<4> log2 = 0;
    for (int i = 0; i < 8; i++) {
        #pragma HLS UNROLL
        if (x[i] == 1) {
            log2 = i+1;
        }
    }
    return log2;
}

void packer(hls::stream<dma_t> &outs, hls::stream<pack_info_t> &pack_infos) {
    static int pixel_idx = 0;
    static ap_uint<10> acc_length = 0;
    static ap_uint<data_size*2> data = 0;
    for (int j = 0; j < 16; j++) {
        #pragma HLS PIPELINE II=1 rewind
        pack_info_t pack_info = pack_infos.read();

        // accumulate input to output
        if (j == 0) {
            data.range(acc_length + 7, acc_length) = pack_info.base.r;
            data.range(acc_length + 15, acc_length + 8) = pack_info.base.g;
            data.range(acc_length + 23, acc_length + 16) = pack_info.base.b;
            acc_length += 24;
            data.range(acc_length + 3, acc_length) = pack_info.bitlens.r;
            data.range(acc_length + 7, acc_length + 4) = pack_info.bitlens.g;
            data.range(acc_length + 11, acc_length + 8) = pack_info.bitlens.b;
            acc_length += 12;
        }
        data.range(acc_length + pack_info.bitlens.r-1, acc_length) = pack_info.delta.r;
        data.range(acc_length + pack_info.bitlens.g-1, acc_length) = pack_info.delta.g;
        data.range(acc_length + pack_info.bitlens.b-1, acc_length) = pack_info.delta.b;
        acc_length += pack_info.bitlens.r + pack_info.bitlens.g + pack_info.bitlens.b;

        pixel_idx++;
        if (pixel_idx == 1080 * 960 ) {
            dma_t out;
            out.data = data.range(data_size-1, 0);
            out.last = 1;
            pixel_idx = 0;
            acc_length = acc_length - data_size;
            outs.write(out);
            data = data >> data_size;
        }
        else {
            if (acc_length >= data_size){
                dma_t out;
                out.data = data.range(data_size-1, 0);
                out.last = 0;
                acc_length = acc_length - data_size;
                outs.write(out);
                data = data >> data_size;
            }
        }
    }
}