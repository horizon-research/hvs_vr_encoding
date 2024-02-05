#include "test_pattern_generator.h"

// #define Cosim
void test_pattern_generator(hls::stream<dma_t> &axis_s2mm)
{
    // ap_none
    #ifndef Cosim
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #endif
    //axis
    #pragma HLS INTERFACE axis port=axis_s2mm
    // compact
    #pragma HLS AGGREGATE compact=bit variable=axis_s2mm

#ifdef Cosim
    for(int i = 0; i < hw_frame_num; i++){
#endif
        ap_uint<32> counter = 0;
        for (int j = 0; j < hw_per_frame_trans; j++){
            #pragma HLS PIPELINE II = 1  rewind
            dma_t dma;
            dma.data = counter;
            if (j == hw_per_frame_trans - 1){
                dma.last = 1;
            }
            else{
                dma.last = 0;
            }
            axis_s2mm.write(dma);
            counter++;
        }
#ifdef Cosim
    }
#endif
}
