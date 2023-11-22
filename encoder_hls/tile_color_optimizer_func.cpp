#include "tile_color_optimizer_func.h"

void rgb2srgb(agg_outputs_srgb &out_srgb, agg_outputs_rgb &out_rgb)
{
	for(int i = 0; i < 16; i++)
	{
		#pragma HLS PIPELINE II=1 style=stp rewind
		#pragma HLS UNROLL factor=1
		for(int j = 0; j < 3; j++)
		{
			#pragma HLS UNROLL
			#pragma HLS ARRAY_PARTITION variable=RGB2sRGB_LUT dim=0 complete
			ap_uint<8> idx;
			idx = out_rgb.rgb[i][j].range(15, 8);
			out_srgb.rgb[i][j] = RGB2sRGB_LUT[idx];
		}
	}
}


void axis_reader(
	agg_inputs &in,
	hls::stream< agg_inputs > &din
)
{
	#pragma HLS PIPELINE II=1
	in = din.read();
}

void axis_writer(
	hls::stream< agg_outputs_srgb > &dout,
		agg_outputs_srgb &out_srgb
)
{
	#pragma HLS PIPELINE II=8
	dout.write(out_srgb);
}

void tile_color_optimizer_func(
	hls::stream< agg_outputs_srgb > &dout,
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
	#pragma HLS DATAFLOW disable_start_propagation

	// Initialization
	vr_prototype::Tile_color_optimizer_blue<vr_prototype::Color::BLUE, vr_prototype::Color::RED, vr_prototype::Color::GREEN> blue_optimizer; // Blue opt

	agg_outputs_rgb out;
	agg_outputs_srgb out_srgb;
	agg_inputs in;
	#pragma HLS ARRAY_PARTITION variable=out.rgb dim=2 complete
	#pragma HLS ARRAY_PARTITION variable=out_srgb.rgb dim=2 complete

	// Read data from axis
	axis_reader(in, din);
	blue_optimizer(out, in);
	rgb2srgb(out_srgb, out);
	axis_writer(dout, out_srgb);
}
