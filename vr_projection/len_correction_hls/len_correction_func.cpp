#include "len_correction_func.h"


void len_correction_func(
	hls::stream< agg_outputs_srgb > &dout,
	hls::stream< agg_inputs > &din
	)
{	// HLS INTERFACE stram setting
	#pragma HLS AGGREGATE compact=bit variable=dout
	#pragma HLS AGGREGATE compact=bit variable=din
	#pragma HLS INTERFACE axis register both port=dout
	#pragma HLS INTERFACE axis register both port=din
	#pragma HLS INTERFACE ap_ctrl_none port=return

	// Initialization
	vr_prototype::Len_corrector len_corrector; // Blue opt
	#pragma HLS DATAFLOW disable_start_propagation
	len_corrector(dout, din);
}
