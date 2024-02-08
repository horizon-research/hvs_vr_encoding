
#include<ap_int.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

const int data_size = 128;
const int data_size_bitlen = 8;
typedef ap_uint<data_size> data_t;
struct dma_t
{
	data_t data;
	ap_uint<1> last;
};

struct Pixel_t { // Pynq is BGR format
	ap_uint<8> b;
	ap_uint<8> g;
	ap_uint<8> r;
};

struct bitlens_t {
    ap_uint<4> b;
    ap_uint<4> g;
    ap_uint<4> r;
};

struct rec_info_t { // Pynq is BGR format
    Pixel_t delta;
    Pixel_t base;
};


struct SixteenPixel_t { // Pynq is BGR format
    Pixel_t data[16];
};


void bd_dec(hls::stream<SixteenPixel_t> &outs, hls::stream<dma_t> &ins);

void deserializer(hls::stream<SixteenPixel_t> &outs, hls::stream<rec_info_t> &rec_infos);
void compact_reader(data_t &rdata, hls::stream<dma_t> &ins, ap_uint<6> n, bool flush_after_read) ;


void decoder(hls::stream<rec_info_t> &rec_infos, hls::stream<dma_t> &ins);
