#include<ap_int.h>
#include<hls_stream.h>

typedef ap_uint<128> data_t;
struct dma_t
{
	/* data */
	data_t data;
	bool last;
};

typedef ap_uint<6> burst_info_t;


void axi_dma(data_t* axi_rd,data_t* axi_wr,hls::stream<data_t> axis_mm2s,hls::stream<data_t> axis_s2mm,
		     int rd_len,int wr_len);
