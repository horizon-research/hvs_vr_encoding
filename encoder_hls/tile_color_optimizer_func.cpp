#include "tile_color_optimizer_func.h"

namespace vr_prototype
{
	void tile_color_optimizer_func(
		hls::stream< agg_outputs > &dout,
		hls::stream< agg_inputs > &din
        )
	{	// HLS INTERFACE stram setting
        #pragma HLS AGGREGATE compact=bit variable=dout
        #pragma HLS AGGREGATE compact=bit variable=din
        #pragma HLS DATA_PACK variable=dout
        #pragma HLS DATA_PACK variable=din
        #pragma HLS INTERFACE axis register both port=dout
		#pragma HLS INTERFACE axis register both port=din
        // module setting
        #pragma HLS INTERFACE ap_ctrl_none port=return
		#pragma HLS DATAFLOW disable_start_propagation

		// optimizer initailization
		const 












	//#pragma HLS INTERFACE ap_ctrl_none port = return
	#pragma HLS DATAFLOW disable_start_propagation
		hls::stream<ap_uint<3>> tx_ctrl_rx("tx_ctrl_rx_ldpc_enc");
	#pragma HLS STREAM variable = tx_ctrl_rx depth = 8

		hls::stream<ap_uint<64>> _din1("_din1_ldpc_enc");
		hls::stream<ap_uint<40>> _ctrl1("_ctrl1_ldpc_enc");
		hls::stream<ap_uint<64>> _din2("_din2_ldpc_enc");
		hls::stream<ap_uint<40>> _ctrl2("_ctrl2_ldpc_enc");
		sdfec_enc_tx(
			din,
			ctrl,
			_din1,
			_din2,
			_ctrl1,
			_ctrl2,
			tx_ctrl_rx
			);

		hls::stream<ap_uint<64>> sdfec_out1;
		hls::stream<ap_uint<64>> sdfec_out2;

		sdfec_enc_multicore_hls(
			_ctrl1,
			_din1,
			sdfec_out1,
			_ctrl2,
			_din2,
			sdfec_out2);

		sdfec_enc_rx(
			tx_ctrl_rx,
			sdfec_out1,
			sdfec_out2,
			dout);


	}
}
