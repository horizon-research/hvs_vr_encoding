#include "result_checker.h"
#include <iostream>

int main() {
    hls::stream<dma_t> axis_mm2s("axis_mm2s");

    // Generate test pattern
    int Err_Num = 0; // introduce Err_Num errors
    for(int i = 0; i < hw_frame_num; i++){
        ap_uint<32> counter = 0;
        for (int j = 0; j < hw_per_frame_trans; j++){
            dma_t dma;
            if (i == 3 && j < Err_Num){ 
                dma.data = counter + 1;
            }
            else{
                dma.data = counter;
            }
            if (j == hw_per_frame_trans - 1){
                dma.last = 1;
            }
            else{
                dma.last = 0;
            }
            axis_mm2s.write(dma);
            counter++;
        }
    }

    // Run the test
    ap_uint<32> error_num = 0;
    ap_uint<32> test_frame_num = hw_frame_num;
    result_checker(  error_num,  test_frame_num, axis_mm2s);

    // Validate the result
    std::cout << "Error Num: " << error_num << std::endl;
    std::cout << "Frame Num: " << frame_num << std::endl;
    
    return 0;
}
