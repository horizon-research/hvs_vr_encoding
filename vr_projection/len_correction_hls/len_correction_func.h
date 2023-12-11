#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>




namespace vr_prototype
{

	template <int opt_channel = Color::BLUE, int color1 = Color::RED, int color2 = Color::GREEN>
	// TODO: make template automatically inferenced
	class Len_corrector
	{
		public:
		// Init Config file (hard coded)
		
			
		void operator()(hls::stream<agg_outputs_srgb> &dout, hls::stream<agg_inputs> &din) {
			// Partitioning const arrays inside the class
			#pragma HLS ARRAY_PARTITION variable=max_vec_rgb dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=min_vec_rgb dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=max_vec_dkl dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=min_vec_dkl dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=DKL2RGB dim=0 complete
			#pragma HLS DATAFLOW disable_start_propagation

			// Input Management  184 rows, 960 columns
			ap_uint<24> pixel_buffer_1[92][480];
			ap_uint<24> pixel_buffer_2[92][480];
			ap_uint<24> pixel_buffer_2[92][480];
			ap_uint<24> pixel_buffer_2[92][480];
			
			input_manage(din, pixel_buffer_1, pixel_buffer_2, pixel_buffer_3, pixel_buffer_4);

			// Len correction using Biliner interpolation
			len_correction(dout, pixel_buffer_1, pixel_buffer_2, pixel_buffer_3, pixel_buffer_4);
			
		}
	}

}

