#include "tile_color_optimizer_func.h"
// #include "hls_streamofblocks.h"
//WARNING: [XSIM 43-3431] One or more environment variables have been detected which affect the operation of the C compiler. These are typically not set in standard installations and are not tested by Xilinx, however they may be appropriate for your system, so the flow will attempt to continue.  If errors occur, try running xelab with the "-mt off -v 1" switches to see more information from the C compiler. The following environment variables have been detected:
//    LIBRARY_PATH


template<int TB_TIMES>
void rgb2srgb(hls::stream<agg_outputs_srgb> &dout, hls::stream<rgb_t_array> &opt_points_stream)
{
	ap_uint<4> count = 0;
	#ifndef __SYNTHESIS__
	for(int i = 0; i < TB_TIMES; i++)
	#else
	while(true) 
	#endif
	{
		#pragma HLS PIPELINE II=1
		rgb_t_array opt_points_i = opt_points_stream.read();
		agg_outputs_srgb out_srgb;
		for(int j = 0; j < 3; j++)
		{
			#pragma HLS UNROLL
			#pragma HLS ARRAY_PARTITION variable=RGB2sRGB_LUT dim=0 complete
			ap_uint<8> idx;
			idx = opt_points_i.data[j].range(15, 8);
			out_srgb.rgb[count][j] = RGB2sRGB_LUT[idx];
		}
		if (count == 15) {
			dout.write(out_srgb);
			count = 0;
		}
		else {
			count++;
		}
	}
}

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

	// Initialization
	vr_prototype::Tile_color_optimizer_blue<vr_prototype::Color::BLUE, vr_prototype::Color::RED, vr_prototype::Color::GREEN> blue_optimizer; // Blue opt
	hls::stream< rgb_t_array > opt_points_stream("opt_points_stream");
	#pragma HLS AGGREGATE compact=bit variable=opt_points_stream
	
	#pragma HLS DATAFLOW disable_start_propagation
	blue_optimizer(opt_points_stream, din);
	rgb2srgb<TB_SMALL_BLOCKS_NUM>(dout, opt_points_stream);
}
