#include<ap_int.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

const int data_size = 128;

struct dma_t
{
	data_t data;
	ap_uint<1> last;
};

struct pack_info_t
{
	Pixel_t delta;
	bitlens_t bitlens;
    Pixel_t base;
};

typedef ap_uint<data_size> data_t;

struct Pixel_t { // Pynq is BGR format
	ap_uint<8> b;
	ap_uint<8> g;
	ap_uint<8> r;
};

struct bitlens_t { // Pynq is BGR format
	ap_uint<4> b;
	ap_uint<4> g;
	ap_uint<4> r;
};

struct SixteenPixel_t { // Pynq is BGR format
    Pixel_t data[16];
};


void bd_enc(hls::stream<dma_t> &outs, hls::stream<SixteenPixel_t> &ins);