#include "tile_color_optimizer_func.h"

void tile_color_optimizer_func(
	hls::stream< agg_outputs > &dout,
	hls::stream< agg_inputs > &din
	)
{	// HLS INTERFACE stram setting
	#pragma HLS AGGREGATE compact=bit variable=dout
	#pragma HLS AGGREGATE compact=bit variable=din
//        #pragma HLS DATA_PACK variable=dout
//        #pragma HLS DATA_PACK variable=din
	#pragma HLS INTERFACE axis register both port=dout
	#pragma HLS INTERFACE axis register both port=din
	// module setting
	#pragma HLS INTERFACE ap_ctrl_none port=return
//		#pragma HLS DATAFLOW disable_start_propagation

	// Initialization
	vr_prototype::Tile_color_optimizer<2, 0 ,1> blue_optimizer; // Blue opt

	agg_outputs out;
	agg_inputs in;

	#pragma HLS PIPELINE II=16
	in = din.read();
	blue_optimizer(out, in);
	dout.write(out);
}
