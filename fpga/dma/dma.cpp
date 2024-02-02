#include"dma.h"
#include <hls_stream.h>
#include <hls_task.h>
// TODO: Add memory sharing between two ddr_reader and ddr_writer

void ddr_writer(hls::stream<ap_uint<1>> &lasts, hls::burst_maxi<data_t> axi_s2mm, hls::stream<dma_t> &axis_s2mm, hls::stream<ap_uint<1>> &reader_resps, const ap_uint<32> frame_offset);
void ddr_reader(hls::stream<dma_t> &axis_mm2s, hls::stream<ap_uint<1>> &reader_resps, hls::burst_maxi<data_t> axi_mm2s, hls::stream<ap_uint<1>> &lasts, const ap_uint<32> frame_offset);

void axi_dma(hls::burst_maxi<data_t> axi_mm2s, hls::burst_maxi<data_t>  axi_s2mm, hls::stream<dma_t> &axis_mm2s, hls::stream<dma_t> &axis_s2mm, const ap_uint<32> &frame_offset){
#pragma HLS INTERFACE axis register both port=axis_s2mm
#pragma HLS INTERFACE axis register both port=axis_mm2s
#pragma HLS aggregate variable=axis_s2mm compact=bit
#pragma HLS aggregate variable=axis_mm2s compact=bit
#pragma HLS interface mode=m_axi port=axi_mm2s offset=slave bundle=axi_mm2s max_read_burst_length=MaxBurstSize num_read_outstanding=1 latency=100
#pragma HLS interface mode=m_axi port=axi_s2mm offset=slave bundle=axi_s2mm max_write_burst_length=MaxBurstSize num_write_outstanding=1 latency=100
#pragma HLS dataflow

 #pragma HLS interface ap_ctrl_none port=return

#pragma HLS interface s_axilite port=frame_offset bundle=control
#pragma HLS STABLE variable=frame_offset

hls_thread_local hls::stream<ap_uint<1>> reader_resps;
hls_thread_local hls::stream<ap_uint<1>> lasts;
#pragma HLS STREAM variable=reader_resps depth=200
#pragma HLS STREAM variable=lasts depth=200
hls_thread_local hls::task t1(ddr_writer, lasts, axi_s2mm, axis_s2mm, reader_resps, frame_offset);
hls_thread_local hls::task t2(ddr_reader, axis_mm2s, reader_resps, axi_mm2s, lasts, frame_offset);




// hls::stream<ap_uint<1>> reader_resps;
// hls::stream<ap_uint<1>> lasts;
// ddr_writer(lasts, axi_s2mm, axis_s2mm, reader_resps, frame_offset);
// ddr_reader(axis_mm2s, reader_resps, axi_mm2s, lasts, frame_offset);

}


void ddr_writer(hls::stream<ap_uint<1>> &lasts, hls::burst_maxi<data_t> axi_s2mm, hls::stream<dma_t> &axis_s2mm, hls::stream<ap_uint<1>> &reader_resps, const ap_uint<32> frame_offset){
    // #ifndef  __SYNTHESIS__
    //     int frame_num = 0;
    // #endif 

    std::cout << "In write" << std::endl;

    burst_len_t burst_len;
    ap_uint<32> offset = 0; // or size_t
    ap_uint<1> wait_for_reader_resp = 0;

    while (true){
        // send request
        std::cout << "In write loop" << std::endl;
        axi_s2mm.write_request(offset, MaxBurstSize);
        std::cout << "offset: " << offset << std::endl;
        
        //update offset
        offset += MaxBurstSize;

        // send data
        ap_uint<1> frame_done = 0;
        for (int i = 0; i < MaxBurstSize; i++){
            #pragma HLS PIPELINE II = 1
            if (frame_done == 0){
                dma_t dma = axis_s2mm.read();
                data_t data = dma.data;
                frame_done = dma.last;
                axi_s2mm.write(data);
                std::cout << "data: " << data << std::endl;
                std::cout << "frame_done: " << frame_done << std::endl;
            } else {
                axi_s2mm.write(0); // padding
            }
        }

        // wait for response
        axi_s2mm.write_response();

        // notify reader about this burst by writing a bit to lasts stream
        {
            if (frame_done){
                lasts.write(1);

                if ( offset >= 0 && offset <= frame_offset){
                    offset = frame_offset;
                }
                else {
                    offset = 0;
                }


                // since we are using double buffering, we need to wait for the reader to finish reading the previous frame after first two frames
                if (wait_for_reader_resp){
                    reader_resps.read();
                }
                else {
                    wait_for_reader_resp = 1;
                }

                // #ifndef  __SYNTHESIS__
                //     frame_num++;
                //     if (frame_num == CsimFrameNum)
                //         break;
                // #endif 
            }
            else {
                lasts.write(0);
            }
        }
    }
}

void ddr_reader(hls::stream<dma_t> &axis_mm2s, hls::stream<ap_uint<1>> &reader_resps, hls::burst_maxi<data_t> axi_mm2s, hls::stream<ap_uint<1>> &lasts, const ap_uint<32> frame_offset){
    ap_uint<32> offset = 0;
    while (true) {
        // read info from writer
        std::cout << "lasts.size(): " << lasts.size() << std::endl;
        ap_uint<1> last = lasts.read();
        // send request
        std::cout << "offset: " << offset << std::endl;
        axi_mm2s.read_request(offset, MaxBurstSize);
        // update offset
        offset += MaxBurstSize;
        // read data
        for (int i = 0; i < MaxBurstSize; i++){
            #pragma HLS PIPELINE II = 1
            data_t data = axi_mm2s.read();
            dma_t dma;
            dma.data = data;
            std::cout << "data: " << dma.data << std::endl;
            dma.last = (i == MaxBurstSize - 1) && last;
            axis_mm2s.write(dma);
        }

        // notify writer aboutfinishig a frame by  writing a bit to reader_resps stream (enable it to write this frame)
        if (last){
            reader_resps.write(1);
            if ( offset >= 0 && offset <= frame_offset){
                offset = frame_offset;
            }
            else {
                offset = 0;
            }
        }
    }
}
