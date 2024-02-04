
#include "dma_tester.h"

void dma_tester (hls::stream<dma_t> &axis_s2mm, hls::stream<ap_uint<32>> &error_nums, hls::stream<dma_t> &axis_mm2s)
{
    #pragma HLS DATAFLOW
    // ap_none
    #pragma HLS INTERFACE ap_ctrl_none port=return
    //axis
    #pragma HLS INTERFACE axis port=axis_s2mm
    #pragma HLS INTERFACE axis port=axis_mm2s
    #pragma HLS INTERFACE axis port=error_nums
    // compact
    #pragma HLS AGGREGATE compact=bit variable=axis_s2mm
    #pragma HLS AGGREGATE compact=bit variable=axis_mm2s
    

    test_pattern_generator(axis_s2mm);
    result_checker(error_nums, axis_mm2s);
}

void test_pattern_generator(hls::stream<dma_t> &axis_s2mm)
{
    data_t counter = 0;
    for(int i = 0; i < hw_frame_num; i++){
        for (int j = 0; j < hw_per_frame_trans; j++){
            #pragma HLS PIPELINE II = 2
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
    }
}


void result_checker(hls::stream<ap_uint<32>> &error_nums, hls::stream<dma_t> &axis_mm2s)
{
    data_t counter = 0;
    ap_uint<32> error_count = 0;
    for(int i = 0; i < hw_frame_num; i++){
        for (int j = 0; j < hw_per_frame_trans; j++){
            #pragma HLS PIPELINE II = 2
            dma_t dma = axis_mm2s.read();
            if (dma.data != counter){
                error_count++;
            }
            if (j == hw_per_frame_trans - 1){
                if (dma.last != 1){
                    error_count++;
                }
            }
            else{
                if (dma.last != 0){
                    error_count++;
                }
            }
            error_nums.write(error_count);
            counter++;
        }
    }

}



