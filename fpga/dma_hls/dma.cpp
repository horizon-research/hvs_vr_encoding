#include"dma.h"
#include <hls_stream.h>


//  #define Cosim
// Manual for burst memory access: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Using-Manual-Burst
void axi_dma(hls::burst_maxi<data_t> axi_mm2s, hls::burst_maxi<data_t>   axi_s2mm, hls::stream<dma_t> &axis_mm2s, hls::stream<dma_t> &axis_s2mm, const ap_uint<32> &frame_offset){
#pragma HLS AGGREGATE compact=bit variable=axis_s2mm
#pragma HLS AGGREGATE compact=bit variable=axis_mm2s

#pragma HLS INTERFACE axis register both port=axis_s2mm
#pragma HLS INTERFACE axis register both port=axis_mm2s
#ifndef Cosim
#pragma HLS interface mode=m_axi port=axi_mm2s offset=slave bundle=axi_mm2s max_read_burst_length=MaxBurstSize num_read_outstanding=1 depth=MaxBurstSize
#pragma HLS interface mode=m_axi port=axi_s2mm offset=slave bundle=axi_s2mm max_write_burst_length=MaxBurstSize num_write_outstanding=1 depth=MaxBurstSize
#else
#pragma HLS interface mode=m_axi port=axi_mm2s offset=slave bundle=axi_mm2s max_read_burst_length=MaxBurstSize num_read_outstanding=1 depth=frame_size*2
#pragma HLS interface mode=m_axi port=axi_s2mm offset=slave bundle=axi_s2mm max_write_burst_length=MaxBurstSize num_write_outstanding=1 depth=frame_size*2
#endif


#pragma HLS interface s_axilite port=frame_offset bundle=control
#pragma HLS interface s_axilite port=axi_mm2s bundle=control
#pragma HLS interface s_axilite port=axi_s2mm bundle=control
#pragma HLS STABLE variable=frame_offset
#pragma HLS STABLE variable=axi_s2mm
#pragma HLS STABLE variable=axi_mm2s

#pragma HLS INTERFACE mode=ap_none port=frame_offset
#pragma HLS INTERFACE mode=s_axilite port=return bundle=control

#pragma HLS dataflow

hls::stream<ap_uint<1>> reader_resps("reader_resps");
hls::stream<data_t> input_fifo("input_fifo");
hls::stream<burst_info_t> burst_infos1("burst_infos1");
hls::stream<burst_info_t> burst_infos2("burst_infos2");
#pragma HLS STREAM variable=reader_resps depth=8
#pragma HLS STREAM variable=input_fifo depth=MaxBurstSize
#pragma HLS STREAM variable=burst_infos1 depth=2
#pragma HLS STREAM variable=burst_infos2 depth=2
#ifndef  __SYNTHESIS__
// Because squential nature of csim , I need to use this loop to prevent stall
// hls::task (multi-thread) is not supported well
for(int k = 0; k < sim_times+1; k++)
{
#endif
    input_counter(input_fifo, burst_infos1, axis_s2mm);
    ddr_writer(burst_infos2, axi_s2mm, input_fifo, burst_infos1, reader_resps, frame_offset);
    ddr_reader(axis_mm2s, reader_resps, axi_mm2s, burst_infos2, frame_offset);
#ifndef  __SYNTHESIS__
}
if (reader_resps.size() > 0)
    reader_resps.read(); // to make sure the reader_resps stream is empty
#endif

}

void input_counter(hls::stream<data_t> &input_fifo, hls::stream<burst_info_t> &burst_lens, hls::stream<dma_t> &axis_s2mm){
#ifdef  __SYNTHESIS__
#ifdef Cosim
    for(int j = 0; j < sim_times+1; j++){
#else
    while (true){
#endif
#endif
        burst_len_t burst_len = 0;
        ap_uint<1> last=0;
        for (int i = 0; i < MaxBurstSize; i++){
            #pragma HLS PIPELINE II = 1
            dma_t dma = axis_s2mm.read();
            input_fifo.write(dma.data);
            burst_len++;
            if (dma.last){
                last = 1;
                break;
            }
        }
        burst_info_t burst_info;
        burst_info.last = last;
        burst_info.burst_len = burst_len;
        burst_lens.write(burst_info);
#ifdef  __SYNTHESIS__
    }   
#endif
}

void burst_write(hls::burst_maxi<data_t>  axi_s2mm, hls::stream<data_t> &input_fifo, burst_len_t &burst_len, ap_uint<32> &offset){
    #pragma HLS INLINE off
    // send request
    axi_s2mm.write_request(offset, burst_len);

    //update offset
    offset += burst_len;

    // send data
    // set the min max trip count to the burst_len
    for (int i = 0; i < burst_len; i++){
        #pragma HLS loop_tripcount max = MaxBurstSize
        #pragma HLS PIPELINE II = 1 rewind
        data_t data = input_fifo.read();
        axi_s2mm.write(data);
    }

    axi_s2mm.write_response();
}

void ddr_writer(hls::stream<burst_info_t> &burst_infos2, hls::burst_maxi<data_t>  axi_s2mm, hls::stream<data_t> &input_fifo, hls::stream<burst_info_t> &burst_infos1, hls::stream<ap_uint<1>> &reader_resps, const ap_uint<32> &frame_offset){
    static ap_uint<32> offset = 0; // or size_t
    static ap_uint<1> wait_for_reader_resp = 0;
    static burst_info_t burst_info;
    static bool first_iter = true;

#ifdef  __SYNTHESIS__
#ifdef Cosim
    for(int j = 0; j < sim_times+1; j++){
#else
    while (true){
#endif
#endif
        if(!first_iter) {
        	 burst_infos2.write(burst_info); // need to move here, or it won't be blocked by the write resp, and will trigger reader too early.
        }
        first_iter = false;
        // read burst_len
        burst_info = burst_infos1.read();
        burst_len_t burst_len = burst_info.burst_len;
        ap_uint<1> last = burst_info.last;

        // burst write
        burst_write(axi_s2mm, input_fifo, burst_len, offset);

        if (last){
            // change offset
            if ( offset <= frame_offset){
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
        }
        // notify reader about finishing a frame by writing a bit to burst_infos2 stream (enable it to read this transmition)
#ifdef  __SYNTHESIS__
    }   
#endif
}

void ddr_reader(hls::stream<dma_t> &axis_mm2s, hls::stream<ap_uint<1>> &reader_resps, hls::burst_maxi<data_t>  axi_mm2s, hls::stream<burst_info_t> &burst_infos2, const ap_uint<32> &frame_offset){
    static ap_uint<32> offset = 0;
    static bool first_iter = true;
#ifdef  __SYNTHESIS__
#ifdef Cosim
    for(int j = 0; j < sim_times + 1; j++){
#else
    while (true){
#endif
#endif
    	if (first_iter) {first_iter = false;}
    	else{
			// read info from writer
			burst_info_t burst_info = burst_infos2.read();
			burst_len_t burst_len = burst_info.burst_len;
			ap_uint<1> last = burst_info.last;
			// send request
			axi_mm2s.read_request(offset, burst_len);
			// update offset
			offset += burst_len;
			// read data
			for (int i = 0; i < burst_len; i++){
				#pragma HLS loop_tripcount max = MaxBurstSize
				#pragma HLS PIPELINE II = 1
				data_t data = axi_mm2s.read();
				dma_t dma;
				dma.data = data;
				dma.last = (i == burst_len - 1) && last;
				axis_mm2s.write(dma);
			}

			// notify writer aboutfinishig a frame by  writing a bit to reader_resps stream (enable it to write this frame)
			if (last){
				reader_resps.write(1);
				if ( offset <= frame_offset){
					offset = frame_offset;
				}
				else {
					offset = 0;
				}
			}
    	}
    #ifdef  __SYNTHESIS__
    }
    #endif
}
