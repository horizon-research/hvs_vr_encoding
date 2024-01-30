#include"dma.h"

void axi_dma(data_t* axi_rd, data_t* axi_wr, hls::stream<data_t> axis_mm2s, hls::stream<data_t> axis_s2mm,
		     int rd_len, int wr_len){
#pragma HLS STREAM variable=axis_s2mm depth=1024 dim=1
#pragma HLS STREAM variable=axis_mm2s depth=1024 dim=1
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=rd_len
#pragma HLS INTERFACE s_axilite port=wr_len
#pragma HLS INTERFACE axis register both port=axis_s2mm
#pragma HLS INTERFACE axis register both port=axis_mm2s
#pragma HLS INTERFACE m_axi depth=100 port=axi_wr offset=slave bundle=AXI_S2MM max_write_burst_length=256
#pragma HLS INTERFACE m_axi depth=100 port=axi_rd offset=slave bundle=AXI_MM2S max_read_burst_length=256
    for(int i=0;i<rd_len;i++){
#pragma HLS PIPELINE
    	data_t tmp;
    	tmp=*(axi_rd+i);
    	axis_mm2s<<tmp;
    }
    for(int i=0;i<wr_len;i++){
#pragma HLS PIPELINE
    	data_t tmp;
    	axis_s2mm>>tmp;
    	*(axi_wr+i)=tmp;
    }
}