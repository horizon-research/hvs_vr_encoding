#include "BD_dec.h"

void bd_dec(hls::stream<SixteenPixel_t> &outs, hls::stream<dma_t> &ins) {
    #pragma HLS INTERFACE axis port=outs
    #pragma HLS INTERFACE axis port=ins
    #pragma HLS AGGREGATE compact=bit variable=outs
    #pragma HLS AGGREGATE compact=bit variable=ins
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW

    hls::stream<rec_info_t> rec_infos("rec_infos");
    #pragma HLS STREAM variable=rec_infos depth=16 // if set to 8, it will cause vivado crash (Why??)
    #pragma HLS AGGREGATE compact=bit variable=rec_infos
    decoder(rec_infos, ins);
    deserializer(outs, rec_infos);

}


void decoder(hls::stream<rec_info_t> &rec_infos, hls::stream<dma_t> &ins) {
    static Pixel_t base;
    static bitlens_t bitlens;
    static ap_uint<5> delta_query_num;
    for (int j = 0; j < 1080 * 960 /16 ; j++) {
        for  (int i = 0; i < 17; i++) {
            #pragma HLS PIPELINE II=1 rewind
            if(i==0) {
                data_t rdata;
                compact_reader(rdata, ins, ap_uint<6>(36), false );
                ap_uint<36> meta_info = rdata;
                base.r = meta_info.range(7,0);
                base.g = meta_info.range(15,8);
                base.b = meta_info.range(23,16);
                rec_info_t rec_info;
                bitlens.r = meta_info.range(27,24);
                bitlens.g = meta_info.range(31,28);
                bitlens.b = meta_info.range(35,32);
                delta_query_num = bitlens.r + bitlens.g + bitlens.b;
            }
            else {
                bool flush_after_read = (i==16) && (j==1080 * 960 /16 - 1);
                data_t rdata;
                compact_reader(rdata, ins, ap_uint<6>(delta_query_num), flush_after_read);
                ap_uint<24> deltas = rdata;
                rec_info_t rec_info;
                rec_info.delta.r = deltas;
                rec_info.delta.g = deltas >> bitlens.r;
                rec_info.delta.b = deltas >> (bitlens.r + bitlens.g);

                // mask the invalid high bits
                rec_info.delta.r = rec_info.delta.r  & (~(~ap_uint<data_size>(0) << bitlens.r));
                rec_info.delta.g = rec_info.delta.g  & (~(~ap_uint<data_size>(0) << bitlens.g));
                rec_info.delta.b = rec_info.delta.b  & (~(~ap_uint<data_size>(0) << bitlens.b));
                rec_info.base = base;
                rec_infos.write(rec_info);
            }
        }
    }
}


void compact_reader(data_t &rdata, hls::stream<dma_t> &ins, ap_uint<6> n, bool flush_after_read) { // 6 is enough for 36 per query
    #pragma HLS INLINE
    static ap_uint<data_size_bitlen> n_buf = 0;
    static data_t buf = 0;
    static data_t in_data;
    auto _buf = buf;
    auto _n_buf = n_buf;
     // first step, update n_buf , buf (those have feedback)
     if (n_buf <= n) {
         in_data = ins.read().data;
         buf = in_data >> ap_uint<6>(n - ap_uint<6>(n_buf));
         n_buf = n_buf + data_size - n;
     }
    else {
        n_buf = n_buf - n;
        buf = buf >> n;
    }

    if (flush_after_read) {
             n_buf = 0;
    }


    // second step, output the data (no feedback)
    if (_n_buf > n) {
        rdata = _buf;
    }
    else {
        // need to conbine the buf and in_data
        rdata =  _buf | (in_data << (_n_buf));
    }
}

void deserializer(hls::stream<SixteenPixel_t> &outs, hls::stream<rec_info_t> &rec_infos) {
    for (int j = 0; j < 1080 * 960 /16 ; j++) {
        SixteenPixel_t out;
        #pragma HLS ARRAY_PARTITION variable=out.data complete
        for (int i = 0; i < 16; i++) {
            #pragma HLS PIPELINE II=1 rewind
            rec_info_t rec_info = rec_infos.read();
            out.data[i].r = rec_info.delta.r + rec_info.base.r;
            out.data[i].g = rec_info.delta.g + rec_info.base.g;
            out.data[i].b = rec_info.delta.b + rec_info.base.b;
            if (i==15) {
                outs.write(out);
            }
        }
    }
}
