// #define FP32
// #ifdef FP32
#include "tile_color_optimizer_func.h"
// #else
// 	#include "tile_color_optimizer_func_fix.h"
// #endif

void tile_color_optimizer_func(
	hls::stream< agg_outputs_srgb > &dout,
	hls::stream< agg_inputs > &din
	)
{	// HLS INTERFACE stram setting
	#pragma HLS AGGREGATE compact=bit variable=dout
	#pragma HLS AGGREGATE compact=bit variable=din
	#pragma HLS INTERFACE axis register both port=dout
	#pragma HLS INTERFACE axis register both port=din
	#pragma HLS INTERFACE ap_ctrl_none port=return
	// #pragma HLS INTERFACE s_axilite port=return
	// Initialization
	vr_prototype::Tile_color_optimizer_blue<vr_prototype::Color::BLUE, vr_prototype::Color::RED, vr_prototype::Color::GREEN> blue_optimizer; // Blue opt
	#pragma HLS DATAFLOW disable_start_propagation
	blue_optimizer(dout, din);
}
