#include "BD_enc.h"

void bd_enc(hls::stream<dma_t> &outs, hls::stream<SixteenPixel_t> &ins) {
    #pragma HLS INTERFACE axis port=outs
    #pragma HLS INTERFACE axis port=ins
    #pragma HLS AGGREGATE compact=bit variable=outs
    #pragma HLS AGGREGATE compact=bit variable=ins
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS dataflow disable_start_propagation

    hls::stream<pack_info_t> pack_infos("pack_infos"); // max = base (24) + bitlen (12) + delta (24) = 60
    #pragma HLS STREAM variable=pack_infos depth=8
    #pragma HLS AGGREGATE compact=bit variable=pack_infos

    hls::stream<Pixel_t> s_ins("s_ins");
    #pragma HLS STREAM variable=s_ins depth=8
    #pragma HLS AGGREGATE compact=bit variable=s_ins

    hls::stream<compact_write_t> compact_writes("compact_writes");
    #pragma HLS STREAM variable=compact_writes depth=8
    #pragma HLS AGGREGATE compact=bit variable=compact_writes

    #ifndef  __SYNTHESIS__
    for (int i = 0; i < 1080 * 960 /16 ; i++) {
    #endif
    
    // std::cout << "Tile = " << i << std::endl;

    serializer(s_ins, ins);
    encoder(pack_infos, s_ins);
    packer(compact_writes, pack_infos);

    
    #ifndef  __SYNTHESIS__
    }
    #endif

    compact_writer(outs, compact_writes);

    
}

void serializer(hls::stream<Pixel_t> &s_ins, hls::stream<SixteenPixel_t> &ins) {
        SixteenPixel_t in;
        in = ins.read();
        //array partition
    #pragma HLS ARRAY_PARTITION variable=in.data complete dim=0
        for (int j = 0; j < 16; j++) {
            #pragma HLS PIPELINE II=1 rewind
            s_ins.write(in.data[j]);
        }

}

void encoder(hls::stream<pack_info_t> &pack_infos, hls::stream<Pixel_t> &ins) {
        #pragma HLS DATAFLOW
        hls::stream<Pixel_t> ins2("ins2");
        #pragma HLS STREAM variable=ins2 depth=32
        hls::stream<Pixel_t> mins("mins");
        hls::stream<Pixel_t> maxs("maxs");
        find_min_max(mins, maxs, ins2, ins);
        get_encoded_data(pack_infos, mins, maxs, ins2);

}

void find_min_max(hls::stream<Pixel_t> &mins, hls::stream<Pixel_t> &maxs, hls::stream<Pixel_t> &ins2, hls::stream<Pixel_t> &ins) {
        Pixel_t min;
        Pixel_t max;
        for (int j = 0; j < 16; j++) {
            #pragma HLS PIPELINE II=1 rewind
            Pixel_t in = ins.read();
            ins2.write(in);
            if (j == 0) {
                min = in;
                max = in;
            }
            else {
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
    Pixel_t min;
    Pixel_t max;
    for (int j = 0; j < 16; j++) {
        #pragma HLS PIPELINE II=1 rewind

        if (j==0){
            min = mins.read();
            max = maxs.read();
        }
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

        // std::cout << "min = " << int(min.r) << " " << int(min.g) << " " << int(min.b) << std::endl;
        // std::cout << "max = " << int(max.r) << " " << int(max.g) << " " << int(max.b) << std::endl;
        // std::cout << "in = " << int(in.r) << " " << int(in.g) << " " << int(in.b) << std::endl;
        // std::cout << "pack_info.base = " << int(pack_info.base.r) << " " << int(pack_info.base.g) << " " << int(pack_info.base.b) << std::endl;
        // std::cout << "pack_info.delta = " << int(pack_info.delta.r) << " " << int(pack_info.delta.g) << " " << int(pack_info.delta.b) << std::endl;
        // std::cout << "pack_info.bitlens = " << int(pack_info.bitlens.r) << " " << int(pack_info.bitlens.g) << " " << int(pack_info.bitlens.b) << std::endl;
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


void packer(hls::stream<compact_write_t> &compact_writes, hls::stream<pack_info_t> &pack_infos) {
    for (int j = 0; j < 16; j++) {
        #pragma HLS PIPELINE II=1 rewind
        pack_info_t pack_info = pack_infos.read();

        // accumulate input to output
        ap_uint<36> meta_info;
        meta_info.range(7, 0) = pack_info.base.r;
        meta_info.range(15, 8) = pack_info.base.g;
        meta_info.range(23, 16) = pack_info.base.b;
        meta_info.range(27, 24) = pack_info.bitlens.r;
        meta_info.range(31, 28) = pack_info.bitlens.g;
        meta_info.range(35, 32) = pack_info.bitlens.b;

        ap_uint<24> deltas = pack_info.delta.r  | (ap_uint<24>(pack_info.delta.g) << pack_info.bitlens.r) | (ap_uint<24>(pack_info.delta.b) << (pack_info.bitlens.r + pack_info.bitlens.g));
        ap_uint<5> delta_lens = pack_info.bitlens.r + pack_info.bitlens.g + pack_info.bitlens.b;

        compact_write_t compact_write;
        if (j == 0) {
            compact_write.data.range(35, 0) = meta_info;        
            compact_write.data.range(59, 36) = deltas;
            compact_write.n = delta_lens + 36;
        }
        else {
            compact_write.data = deltas;
            compact_write.n = delta_lens;
        }
    //     print the compact_write
    //    print bits of data
        // for (int i = 0; i < 60; i++) {
        //     std::cout << compact_write.data[59-i];
        // }
        // std::cout << std::endl;
        // // print n
        // std::cout << compact_write.n << std::endl;
        compact_writes.write(compact_write);
    }
}

void compact_writer(hls::stream<dma_t> &outs, hls::stream<compact_write_t> &compact_writes) {
    static ap_uint<data_size_bitlen> n_buf = 0;
    static ap_uint<data_size> buf = 0;
    for (int j = 0; j < 1080 * 960 +1 ; j++) {
        #pragma HLS PIPELINE II=1 rewind
        if (j == 1080 * 960) {
            if (n_buf > 0) {
                dma_t out;
                n_buf = 0;
                out.data = buf;
                // mask the invalid high bits to zero 
                ap_uint<data_size_bitlen> valid_num = n_buf;
                if (valid_num < data_size) {
                    out.data = out.data & (~(~ap_uint<data_size>(0) << valid_num));
                }
                out.last = 1;
                outs.write(out);
            }
        }
        else {
            compact_write_t compact_write = compact_writes.read();
            ap_uint<60> d = compact_write.data;
            data_t _d = d;
            ap_uint<6> n = compact_write.n; // 63 is enough for 60 per pixel

            // get temp variables for second step
            auto _buf = buf;
            auto _n_buf = n_buf;

            // first step, update n_buf , buf (those have feedback)
            if (n_buf + n < data_size) {
                buf = buf | (_d << n_buf);
                n_buf += n;
            }
            else {
                n_buf = n_buf + n - data_size;
                buf = _d >> n-n_buf;
            }

            ap_uint<data_size_bitlen> valid_num = _n_buf + n;
            bool final_pixel = (j == 1080 * 960 -1);
            if (final_pixel) {
                if (valid_num <= data_size) {
                    n_buf = 0;
                }
            }


            // second step, write out if needed, no feedback
            if (valid_num >= data_size || final_pixel) {
                dma_t out;
                out.data = _buf | (_d << _n_buf);

                // mask the invalid high bits to zero 
                if (valid_num < data_size) {
                    out.data = out.data & (~(~ap_uint<data_size>(0) << valid_num));
                }

                if (final_pixel) {
                    if (valid_num <= data_size) {
                        out.last = 1;
                    }
                    else {
                        out.last = 0;
                    }
                }
                else {
                    out.last = 0;
                }
                outs.write(out);
            }
        }
       
    }
}
