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

struct bitlens_t { // Pynq is BGR format
	ap_uint<4> b;
	ap_uint<4> g;
	ap_uint<4> r;
};

struct pack_info_t
{
	Pixel_t delta;
	bitlens_t bitlens;
    Pixel_t base;
};

struct compact_write_t
{
	ap_uint<60> data;
	ap_uint<6> n; // 63 is enough for 60 per pixel
};




struct SixteenPixel_t { // Pynq is BGR format
    Pixel_t data[16];
};


void bd_enc(hls::stream<dma_t> &outs, hls::stream<SixteenPixel_t> &ins);
void packer(hls::stream<compact_write_t> &compact_writes, hls::stream<pack_info_t> &pack_infos);
ap_uint<4> my_log2(ap_uint<8> x);
void find_min_max(hls::stream<Pixel_t> &mins, hls::stream<Pixel_t> &maxs, hls::stream<Pixel_t> &ins2, hls::stream<Pixel_t> &ins);
void encoder(hls::stream<pack_info_t> &pack_infos, hls::stream<Pixel_t> &ins) ;
void get_encoded_data(hls::stream<pack_info_t> &pack_infos, hls::stream<Pixel_t> &mins, hls::stream<Pixel_t> &maxs, hls::stream<Pixel_t> &ins2);

void serializer(hls::stream<Pixel_t> &s_ins, hls::stream<SixteenPixel_t> &ins);
void compact_writer(hls::stream<dma_t> &outs, hls::stream<compact_write_t> &compact_writes);
