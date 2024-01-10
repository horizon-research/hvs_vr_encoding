#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>

#ifndef TYPE_H
#define TYPE_H // Prevent duplicate definition
struct Pixel { // Pynq is BGR format
	ap_uint<8> b;
	ap_uint<8> g;
	ap_uint<8> r;
};

struct FourPixel { // Pynq is BGR format
	Pixel data[4];
};

struct Memory_query_t {
	ap_uint<11> rows[4];
	ap_uint<10> cols[4];
	ap_uint<1> yield;
};

struct Memory_write_t {
	Pixel data[4];
	ap_uint<11> rows[4];
	ap_uint<10> cols[4];
	ap_uint<1> yield;
};

struct Bilinear_info {
	ap_uint<2> xy11_idx;
	ap_uint<2> xy12_idx;
	ap_uint<2> xy21_idx;
	ap_uint<2> xy22_idx;
	float dx;
	float dy;
	bool valid;
};


#endif 