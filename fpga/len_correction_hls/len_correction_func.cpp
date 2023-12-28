#include "len_correction_func.h"


void len_correction_func(
	hls::stream< Pixel > &dout,
	hls::stream< Agg_in_srgb > &din
	)
{	// HLS INTERFACE stram setting
	#pragma HLS AGGREGATE compact=bit variable=dout
	#pragma HLS AGGREGATE compact=bit variable=din
	#pragma HLS INTERFACE axis register both port=dout
	#pragma HLS INTERFACE axis register both port=din
	#pragma HLS INTERFACE ap_ctrl_none port=return

	// Initialization
	vr_prototype::Len_corrector len_corrector; 
	len_corrector(dout, din);
}
