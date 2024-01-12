#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>

#ifndef TYPE_H
#define TYPE_H // Prevent duplicate definition
struct Pixel_t { // Pynq is BGR format
	ap_uint<8> b;
	ap_uint<8> g;
	ap_uint<8> r;
};

struct FourPixel_t { // Pynq is BGR format
	Pixel_t data[4];
};

struct Memory_query_t {
	ap_uint<11> rows[4];
	ap_uint<10> cols[4];
	ap_uint<1> yield;
	ap_uint<1> read;
};

struct Memory_write_t {
	Pixel_t data;
	ap_uint<11> rows;
	ap_uint<10> cols;
	ap_uint<11> yield_num;
};

struct Bilinear_info_t {
	FourPixel_t data; //x11, x12, x21, x22
	float dx;
	float dy;
	bool valid;
};

struct row_trigger_t { // Pynq is BGR format
	ap_uint<11> yiled_num;
};

struct Partial_bilinear_info_t {
	float dx;
	float dy;
	bool valid;
};
#endif 