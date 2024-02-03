#include<ap_int.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include <math.h>
typedef ap_uint<128> data_t;
const int MaxBurstSize = 256;
typedef ap_uint<10> burst_len_t;
struct dma_t
{
	data_t data;
	ap_uint<1> last;
};

struct burst_info_t
{
	burst_len_t burst_len;
	ap_uint<1> last;
};

void axi_dma(hls::burst_maxi<data_t> axi_mm2s, hls::burst_maxi<data_t>  axi_s2mm, hls::stream<dma_t> &axis_mm2s, hls::stream<dma_t> &axis_s2mm, const ap_uint<32> &frame_offset);
void ddr_writer(hls::stream<burst_info_t> &burst_infos2, hls::burst_maxi<data_t> axi_s2mm, hls::stream<data_t> &input_fifo, hls::stream<burst_info_t> &burst_infos1, hls::stream<ap_uint<1>> &reader_resps, const ap_uint<32> frame_offset);
void ddr_reader(hls::stream<dma_t> &axis_mm2s, hls::stream<ap_uint<1>> &reader_resps, hls::burst_maxi<data_t> axi_mm2s, hls::stream<burst_info_t> &burst_infos2, const ap_uint<32> frame_offset);
void input_counter(hls::stream<data_t> &input_fifo, hls::stream<burst_info_t> &burst_lens, hls::stream<dma_t> &axis_s2mm);

// For Csim Test Bench
const int frame_num = 10;
const int frame_size = 3742;
const int sim_times = ceil(float(frame_size) / float(MaxBurstSize)) * frame_num; // if no float, it will stuck



// For HW Test Bench (dma tester)
const int hw_frame_num = 10;
const int hw_per_frame_trans = 3742;

