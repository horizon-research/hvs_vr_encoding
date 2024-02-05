#include "result_checker.h"


//  #define Cosim
void result_checker(  ap_uint<32> &error_num, ap_uint<32> &test_frame_num, hls::stream<dma_t> &axis_mm2s)
{
    // s_axilite
    #pragma HLS INTERFACE s_axilite port=return bundle=control 

    //axis
    #pragma HLS INTERFACE axis port=axis_mm2s 
    // s_axilite
    #pragma HLS INTERFACE s_axilite port=error_num bundle=control 
    #pragma HLS INTERFACE s_axilite port=test_frame_num bundle=control
    // compact
    #pragma HLS AGGREGATE compact=bit variable=axis_mm2s

    // #pragma HLS STABLE variable=error_num
    // #pragma HLS STABLE variable=frame_num

	#pragma HLS INTERFACE mode=ap_none port=error_num
	#pragma HLS INTERFACE mode=ap_none port=test_frame_num

    error_num = 0;
    for(int i = 0; i < test_frame_num; i++){
        ap_uint<32> counter = 0;
        for (int j = 0; j < hw_per_frame_trans; j++){
            #pragma HLS PIPELINE II = 1 rewind
            dma_t dma = axis_mm2s.read();
            if (j == hw_per_frame_trans - 1) {
                if (dma.data != counter || dma.last != 1){
                    error_num = error_num + 1;
                }
            }
            else{
                if (dma.data != counter || dma.last != 0){
                	error_num = error_num + 1;
                }
            }
            counter++;
        }
    }
}



