#include "test_pattern_generator.h"
#include <iostream>

int main() {
    hls::stream<dma_t> axis_s2mm("axis_s2mm");


    // Run the test
    test_pattern_generator(axis_s2mm);

    for(int i = 0; i < hw_frame_num; i++){
        ap_uint<32> counter = 0;
        for (int j = 0; j < hw_per_frame_trans; j++){
            dma_t dma;
            dma.data = counter;
            if (j == hw_per_frame_trans - 1){
                dma.last = 1;
            }
            else{
                dma.last = 0;
            }
            dma_t _dma =  axis_s2mm.read();
            if (dma.data != _dma.data || dma.last != _dma.last){
                std::cout << "Error: " << dma.data << " " << _dma.data << " " << dma.last << " " << _dma.last << std::endl;
            }

            counter++;
        }
    }


    return 0;
}
