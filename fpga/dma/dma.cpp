#include"dma.h"

// TODO: Add memory sharing between two ddr_reader and ddr_writer

const int Frame_Num = 2;
const int Max_Burst_Length = 32;

void axi_dma(hls::burst_maxi<data_t> axi_mm2s, hls::burst_maxi<data_t>  axi_s2mm, hls::stream<dma_t> axis_mm2s, hls::stream<dma_t> axis_s2mm){
#pragma HLS INTERFACE axis register both port=axis_s2mm
#pragma HLS INTERFACE axis register both port=axis_mm2s
#pragma HLS compact variable=axis_s2mm
#pragma HLS compact variable=axis_mm2s
#pragma HLS interface mode=m_axi port=axi_mm2s offset=slave bundle=axi_mm2s max_read_burst_length=Max_Burst_Length num_read_outstanding=8
#pragma HLS interface mode=m_axi port=axi_s2mm offset=slave bundle=axi_s2mm max_write_burst_length=Max_Burst_Length num_write_outstanding=8
#pragma HLS dataflow

hls::stream<data_t> input_fifo("input_fifo");
#pragma HLS stream variable=input_fifo depth=32
hls::stream<burst_len_t> burst_lens("burst_lens");
#pragma HLS stream variable=input_fifo depth=2
input_accumulator(input_fifo, burst_lens, axis_s2mm);

hls::stream<ap_uint<32>> frame_lens("frame_lens"); // depth=2 is important for ddr_writer and ddr_reader coordination
#pragma HLS stream variable=frame_lens depth=2

ddr_writer(frame_lens, axi_s2mm, input_fifo, burst_lens);

ddr_reader(axis_mm2s, axi_mm2s, frame_lens, frame_offset);

}


void input_accumulator(hls::stream<data_t> &input_fifo, hls::stream<burst_len_t> &burst_lens, hls::stream<dma_t> &axis_s2mm){
    bool last = false;
    burst_len_t burst_len = 0;
    while (!last){
        #pragma HLS PIPELINE II = 1
        dma_t dma = axis_s2mm.read();
        input_fifo.write(dma.data);
        last = dma.last;
        burst_len++;
        if (burst_len == Max_Burst_Length){
            burst_lens.write(burst_len);
            burst_len = 0;
        } 
    }
    if (burst_len != 0){
        #pragma HLS PIPELINE II = 1
        burst_lens.write(burst_len);
    }
}

void ddr_writer(hls::stream<ap_uint<32>> &frame_lens, hls::burst_maxi<data_t> axi_s2mm, hls::stream<data_t> &input_fifo, hls::stream<burst_len_t> &burst_lens){
    burst_len_t burst_len;
    burst_len = burst_lens.read();
    ap_uint<32> offset = 0; // or size_t
    ap_uint<32> sent_times = 0;
    while (burst_len == Max_Burst_Length){ 
        #pragma HLS PIPELINE II = Max_Burst_Length
        axi_s2mm.write_request(offset, Max_Burst_Length);
        offset += 32;
        sent_times += 1;
        for (int i = 0; i < Max_Burst_Length; i++){
            #pragma HLS PIPELINE II = 1
            data_t data;
            data = input_fifo.read();
            axi_s2mm.write(data);
        }
        burst_len = burst_lens.read();
    }

    if (burst_len != 0) {
        #pragma HLS PIPELINE II = Max_Burst_Length
        axi_s2mm.write_request(offset, burst_len);
        offset += burst_len;
        sent_times += 1;

        #pragma HLS loop_tripcount min=1 max=Max_Burst_Length
        for (int i = 0; i < burst_len; i++){
            #pragma HLS PIPELINE II = 1
            data_t data;
            data = input_fifo.read();
            axi_s2mm.write(data);
        }
    }

    // wait for all data response
    for (int i = 0; i < sent_times; i++){
        #pragma HLS PIPELINE II = 1
        axi_s2mm.write_response();
    }

    // write frame length to reader
    frame_lens.write(offset);
}

void ddr_reader(hls::stream<dma_t> &axis_mm2s, hls::burst_maxi<data_t> axi_mm2s, hls::stream<ap_uint<32>> &frame_lens, const ap_uint<32> frame_offset){
    ap_uint<32> offset = 0;
    ap_uint<32> frame_len = frame_lens.read();

    // pre-request
    const int pre_req_num = 8;
    for (int i = 0; i < pre_req_num; i++){
        #pragma HLS PIPELINE II = 1
        burst_len_t burst_len = 0;
        if (frame_len - offset >= 32){
            burst_len = 32;
        } else {
            burst_len = frame_len - offset;
        }
        axi_mm2s.read_request(offset, burst_len);
        offset += burst_len;
    }

    offset = 0;
    ap_uint<32> read_num = 0;
    while (offset < frame_len){
        #pragma HLS PIPELINE II = 32
        burst_len_t burst_len = 0;
        if (frame_len - offset >= 32){
            burst_len = 32;
        } else {
            burst_len = frame_len - offset;
        }
        if (read_num > pre_req_num){
            axi_mm2s.read_request(offset, burst_len);
        }
        read_num++;
        offset += burst_len;
        for (int i = 0; i < burst_len; i++){
            #pragma HLS PIPELINE II = 1
            data_t data;
            data = axi_mm2s.read();
            dma_t dma;
            dma.data = data;
            dma.last = (i == burst_len - 1);
            axis_mm2s.write(dma);
        }
    }
}


