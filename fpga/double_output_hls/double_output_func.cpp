#include "double_output_func.h"

void double_output_func(hls::stream<Axis_pixel_t> &os, hls::stream<Pixel_t> &is) {
    #pragma HLS INTERFACE axis port=os
    #pragma HLS INTERFACE axis port=is
    #pragma HLS INTERFACE ap_ctrl_none port=return
    Pixel_t buf[960];
    #pragma HLS AGGREGATE compact=bit variable=buf// need to aggregate to prevent 3x resource usage, it will use ram in the way
                                                // 24xd  instead 8xd 8xd 8xd (3x memory usage)
    #pragma HLS bind_storage variable=buf type=ram_1p impl=bram
    for (int i = 0; i < 1080; i++) {
        for (int j = 0; j < 1920; j++) {
            #pragma HLS PIPELINE II=1 rewind
            if (j < 960 ) {
                Pixel_t p;
                #pragma HLS AGGREGATE compact=bit variable=p
            
                p = is.read();
                buf[j] = p;
                Axis_pixel_t _p;
                #pragma HLS AGGREGATE compact=bit variable=_p.data
                _p.data = p;
                _p.keep = 0b111;
                _p.last = 0;
                os.write(_p);
            }
            else {
                Axis_pixel_t _p;
                #pragma HLS AGGREGATE compact=bit variable=_p.data
                _p.data = buf[j - 960];
                _p.keep = 0b111;
                if (j == 1919 && i == 1079) {
                    _p.last = 1;
                }
                else {
                    _p.last = 0;
                }
                os.write(_p);
            }

        }
    }
}