#include<ap_int.h>
#include "hls_task.h"
#include <hls_stream.h>
#include <hls_burst_maxi.h>
const int MaxBurstSize = 16;
const int BufferedBurstNum = 2;
const int CsimFrameNum = 10;
typedef ap_uint<128> data_t;
typedef ap_uint<10> burst_len_t;
struct dma_t
{
	data_t data;
	bool last;
};



void axi_dma(hls::burst_maxi<data_t> axi_mm2s, hls::burst_maxi<data_t>  axi_s2mm, hls::stream<dma_t> &axis_mm2s, hls::stream<dma_t> &axis_s2mm, const ap_uint<32> &frame_offset);
