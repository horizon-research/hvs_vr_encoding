#include<ap_int.h>
#include "hls_task.h"
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include <math.h>
const int MaxBurstSize = 64;
typedef ap_uint<32> data_t;
typedef ap_uint<10> burst_len_t;
struct dma_t
{
	data_t data;
	ap_uint<1> last;
};

void axi_dma(hls::burst_maxi<data_t> axi_mm2s, hls::burst_maxi<data_t>  axi_s2mm, hls::stream<dma_t> &axis_mm2s, hls::stream<dma_t> &axis_s2mm, const ap_uint<32> &frame_offset);
void ddr_writer(hls::stream<ap_uint<1>> &lasts, hls::burst_maxi<data_t> axi_s2mm, hls::stream<dma_t> &axis_s2mm, hls::stream<ap_uint<1>> &reader_resps, const ap_uint<32> frame_offset);
void ddr_reader(hls::stream<dma_t> &axis_mm2s, hls::stream<ap_uint<1>> &reader_resps, hls::burst_maxi<data_t> axi_mm2s, hls::stream<ap_uint<1>> &lasts, const ap_uint<32> frame_offset);

const int frame_num = 10;
const int frame_size = 12;
const int test_size = frame_size * frame_num;
const int sim_times = ceil(float(frame_size) / float(MaxBurstSize)) * frame_num; // if no float, it will stuck
const int tb_mem_size = ceil( float(frame_size) / float(MaxBurstSize) ) * 2 * MaxBurstSize;
