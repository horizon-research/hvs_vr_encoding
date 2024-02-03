#include "dma.h"
#include <iostream>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

int main() {
    hls::stream<dma_t> axis_mm2s("axis_mm2s");
    hls::stream<dma_t> axis_s2mm("axis_s2mm");
    data_t axi_mem[tb_mem_size];
    for (int i = 0; i < tb_mem_size; i++){
        axi_mem[i] = i;
    }

    for (int i = 0; i < test_size; i++){
        dma_t dma;
        dma.data = i;
        dma.last = (i % frame_size == frame_size - 1);
        axis_s2mm.write(dma);
    }

    ap_uint<32> frame_offset;
    if(frame_size % MaxBurstSize == 0)
        frame_offset = frame_size / MaxBurstSize * MaxBurstSize;
    else
        frame_offset = (frame_size / MaxBurstSize + 1) * MaxBurstSize;
        
    std::cout << "frame_offset: " << frame_offset << std::endl; 
    hls::burst_maxi<data_t> myBurstMaxi(axi_mem);
    axi_dma(myBurstMaxi, myBurstMaxi, axis_mm2s, axis_s2mm, frame_offset);

    for (int j = 0; j < frame_num; j++){
        dma_t dma;
        for (int i = 0; i < frame_size; i++){
            dma = axis_mm2s.read();
            if (dma.data != (i+j*frame_size)){
                std::cout << "Error: " << dma.data << " != " << (i+j*frame_size) << std::endl;
                return 1;
            }
        }
        if (frame_size % MaxBurstSize > 0){
            for (int i = 0; i < MaxBurstSize - (frame_size % MaxBurstSize); i++){
                dma = axis_mm2s.read();
            }
        }

        if (dma.last != true){
                std::cout << "Error: " << dma.last << " != " << true << std::endl;
                return 1;
        }
    }
    return 0;
}
