#include "dma.h"
#include <iostream>
#include <hls_stream.h>

int main() {
    const int frame_num = 2;
    const int frame_size = 32;
    const int test_size = frame_size * frame_num;
    hls::stream<dma_t> axis_mm2s;
    hls::stream<dma_t> axis_s2mm;
    data_t axi_mm2s[500];
    data_t axi_s2mm[500];

    for (int i = 0; i < test_size; i++){
        dma_t dma;
        dma.data = i;
        dma.last = (i % frame_size == frame_size - 1);
        axis_s2mm.write(dma);
    }

    ap_uint<32> frame_offset = 32;
    // if(frame_size % MaxBurstSize == 0)
    //     frame_offset = frame_size / MaxBurstSize * MaxBurstSize;
    // else
    //     frame_offset = (frame_size / MaxBurstSize + 1) * MaxBurstSize;

    std::cout << "frame_offset: " << frame_offset << std::endl;
    std::cout << "csim start" << std::endl;
    axi_dma(axi_mm2s, axi_s2mm, axis_mm2s, axis_s2mm, frame_offset);
    std::cout << "csim end" << std::endl;

    for (int i = 0; i < test_size; i++){
        
    }

    // std::cout << "axis_mm2s.size(): " << axis_mm2s.size() << std::endl;

    // for (int j = 0; j < frame_num; j++){
    //     dma_t dma;
    //     for (int i = 0; i < frame_size; i++){
    //         dma = axis_mm2s.read();
    //         // if (dma.data != (i+j*frame_size)){
    //         //     std::cout << "Error: " << dma.data << " != " << (i+j*frame_size) << std::endl;
    //         //     return 1;
    //         // }
    //     }
    //     for (int i = 0; i < MaxBurstSize - (frame_size % MaxBurstSize); i++){
    //         dma = axis_mm2s.read();
    //     }
    //     // if (dma.last != true){
    //     //         std::cout << "Error: " << dma.last << " != " << true << std::endl;
    //     //         return 1;
    //     // }
    // }
    return 0;
}
