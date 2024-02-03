#include "dma_tester.h"
#include <iostream>

int main() {
    hls::stream<dma_t> axis_mm2s("axis_mm2s");
    hls::stream<dma_t> axis_s2mm("axis_s2mm");
    hls::stream<ap_uint<32>> error_nums("error_nums");


    data_t counter = 0;
    // Generate test pattern
    for(int i = 0; i < hw_frame_num; i++){
        for (int j = 0; j < hw_per_frame_trans; j++){
            dma_t dma;
            dma.data = counter;
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


    dma_tester(axis_s2mm, error_nums, axis_mm2s);

    // Validate the result
    for (int i = 0; i < hw_frame_num; i++){
        for (int j = 0; j < hw_per_frame_trans; j++){
            auto error_num = error_nums.read();
            if (error_num != 0){
                std::cout << "Error: " << error_num << std::endl;
                return 1;
            }
        }
    }

    counter = 0;
    for(int i = 0; i < hw_frame_num; i++){
        for (int j = 0; j < hw_per_frame_trans; j++){
            dma_t dma;
            dma.data = counter;
            if (j == hw_per_frame_trans - 1){
                dma.last = 1;
            }
            else{
                dma.last = 0;
            }
            auto _dma = axis_s2mm.read();
            if (_dma.data != dma.data){
                std::cout << "Error: " << dma.data << " != " << counter << std::endl;
                return 1;
            }
            if (_dma.last != dma.last){
                std::cout << "Error: " << dma.last << " != " << dma.last << std::endl;
                return 1;
            }
            counter++;
        }
    }

    return 0;
}
