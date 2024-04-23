#include "rearrangement.h"

void rearrangement_func(
        hls::stream<Pixel> &out,
        hls::stream<Agg_in_srgb> &in
    ) {
    #pragma HLS INTERFACE axis port=out
    #pragma HLS INTERFACE axis port=in
    #pragma HLS INTERFACE ap_ctrl_none port=return
    
    vr_prototype::rearragement<4, 960>(out, in);
    
    }
