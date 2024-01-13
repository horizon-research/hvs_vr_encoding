#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>

typedef half abc_t;
typedef half dkl_t;
typedef float rgb_t;

struct float_array {
    float data[3];
};

struct half_array {
    half data[3];
};

struct agg_outputs_srgb {
	ap_uint<8> rgb[16][3];
};

struct agg_inputs {
	abc_t as[16];
	abc_t bs[16];
	abc_t cs[16];
	dkl_t ds[16];
	dkl_t ks[16];
	dkl_t ls[16];
};

void tile_color_optimizer_func(
		hls::stream< agg_outputs_srgb > &dout,
		hls::stream< agg_inputs > &din
        );

namespace vr_prototype
{
	enum Color {
		RED,    // 0
		GREEN,  // 1
		BLUE    // 2
	};

	template <int opt_channel = Color::BLUE, int color1 = Color::RED, int color2 = Color::GREEN>
	// TODO: make template automatically inferenced
	class Tile_color_optimizer_blue
	{
		public:
			const float max_vec_rgb[3] = {0.14766317, -0.13674196, 0.97936063};
			const float min_vec_rgb[3] = {-0.14766317, 0.13674196, -0.97936063};
			const float max_vec_dkl[3] = {0.082895, 0.205911, 0.430501};
			const float min_vec_dkl[3] = {-0.082895, -0.205911, -0.430501};

			//  RED
			// 	const vec_t max_vec_rgb[3] = {0.61894476, -0.24312686, 0.62345751};
			// 	const vec_t min_vec_rgb[3] = {-0.61894476, 0.24312686, -0.62345751};
			// 	const vec_t max_vec_dkl[3] = {0.5015333, 0.0126745, 0.0271685};
			// 	const vec_t min_vec_dkl[3] = {-0.5015333, -0.0126745, -0.0271685};

				
			const float DKL2RGB[3][3] = {
				{10.60864043, 23.50260678, 21.01613594},
				{-3.17452434, -22.53568763, -20.37323115},
				{0.20807273, 154.02866473, 153.78039361}
			};

		void operator()(hls::stream<agg_outputs_srgb> &dout, hls::stream<agg_inputs> &din) {
			// Partitioning const arrays inside the class
			#pragma HLS ARRAY_PARTITION variable=max_vec_rgb dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=min_vec_rgb dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=max_vec_dkl dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=min_vec_dkl dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=DKL2RGB dim=0 complete
			#pragma HLS DATAFLOW disable_start_propagation

			// Stage 0

			hls::stream<agg_inputs> din_stream1, din_stream2;
			duplicate_stream<agg_inputs, 1>(din_stream1, din_stream2, din);

			// Stage 1

			// Read dkl from input stream and comress it to 3 * ele_size width
			hls::stream<float_array> dkl_stream;
			read_dkl_loop(dkl_stream, din_stream1);

			// Find inv_square of a, b, c, and compress it to 3 * ele_size width
			hls::stream<float_array> inv_square_abc_stream;
			inv_square_abc_loop(inv_square_abc_stream, din_stream2);

			// Stage 2

			hls::stream<float_array> dkl_stream1, dkl_stream2;
			duplicate_stream<float_array, 16>(dkl_stream1, dkl_stream2, dkl_stream);

			// Stage 3

			hls::stream<float_array> rgb_stream;
			dkl2rgb_loop(rgb_stream, dkl_stream1);

			// Stage 4
			// prevent deadlock
			#pragma HLS STREAM variable=dkl_stream2 depth=128
			#pragma HLS STREAM variable=inv_square_abc_stream depth=128
			hls::stream<float_array> opt_points_stream;
			adjust_tile(opt_points_stream, rgb_stream, dkl_stream2, inv_square_abc_stream);

			// Stage 5
			rgb2srgb(dout, opt_points_stream);
		}


		template<typename T, int TIMES>
		void duplicate_stream(hls::stream<T> &duplicated_stream1, hls::stream<T> &duplicated_stream2, hls::stream<T> &original_stream) {
			for(int i = 0; i < TIMES; i++)
			{
				#pragma HLS PIPELINE II=1 rewind
				T data = original_stream.read();
				duplicated_stream1.write(data);
				duplicated_stream2.write(data);
			}
		}

		void read_dkl_loop(hls::stream<float_array> &dkl_stream, hls::stream<agg_inputs> &din_stream) {
			agg_inputs din;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				if (i == 0){
					din = din_stream.read();
				}
				float_array dkl_t_array_i;
				#pragma HLS ARRAY_PARTITION variable=dkl_t_array_i.data dim=0 complete
				dkl_t_array_i.data[0] = float(din.ds[i]);
				dkl_t_array_i.data[1] = float(din.ks[i]);
				dkl_t_array_i.data[2] = float(din.ls[i]);
				dkl_stream.write(dkl_t_array_i);
			}
		}

		void inv_square_abc_loop( hls::stream<float_array> &inv_square_abc_stream, hls::stream<agg_inputs> &din_stream){
			agg_inputs din;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				if (i == 0){
					din = din_stream.read();
				}
				float_array inv_square_abcs_i;
				#pragma HLS ARRAY_PARTITION variable=inv_square_abcs_i.data dim=0 complete
				float asi = float(din.as[i]);
				float bsi = float(din.bs[i]);
				float csi = float(din.cs[i]);
				inv_square(inv_square_abcs_i.data, asi, bsi, csi);
				inv_square_abc_stream.write(inv_square_abcs_i);
			}
		}

		void dkl2rgb_loop(hls::stream<float_array> &rgb_stream, hls::stream<float_array> &dkl_stream) {
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				float_array dkl_centers_i = dkl_stream.read();
				float_array rgb_centers_i;
				#pragma HLS ARRAY_PARTITION variable=rgb_centers_i.data dim=0 complete
				mm_3x3to3(rgb_centers_i.data, DKL2RGB, dkl_centers_i.data);
				rgb_stream.write(rgb_centers_i);
			}
		}

		template<typename T1, typename T2, typename T3>
		void mm_3x3to3 (  T1 m3_out[3], const T2 m33[3][3], const T3 m3_in[3]) {
			#pragma HLS PIPELINE II=1 
			for (int i = 0; i < 3; i++) {
				#pragma HLS UNROLL
				m3_out[i] = m33[i][0] * m3_in[0] + m33[i][1] * m3_in[1] + m33[i][2] * m3_in[2];
			}
		}

		template<typename T1, typename T2>
		void inv_square(T1 inv_square_abcs_i[3], T2 &a, T2 &b, T2 &c){
			 #pragma HLS PIPELINE II=1 
			inv_square_abcs_i[0] = hls::recip(a * a);
			inv_square_abcs_i[1] = hls::recip(b * b);
			inv_square_abcs_i[2] = hls::recip(c * c);
		}

		template<typename T>
		void gate( T &out, T max, T min) {
			 #pragma HLS PIPELINE II=1 
			if (hls::isgreater(out, max)) {
				out = max;
			}
			else if ( hls::isless(out, min)) {
				out = min;
			}
		}

		void rgb2srgb(hls::stream<agg_outputs_srgb> &dout, hls::stream<float_array> &opt_points_stream)
		{
			agg_outputs_srgb out_srgb;
			#pragma HLS ARRAY_PARTITION variable=out_srgb.rgb complete dim=0
			for (int i = 0; i < 16; i++)
			{
				#pragma HLS PIPELINE II=1 rewind
				float_array opt_points_i = opt_points_stream.read();
				for(int j = 0; j < 3; j++)
				{
					float srgb;
					float rgb = opt_points_i.data[j];

					gate(rgb, float(1), float(0));

					if ( hls::isless(rgb, float(0.0031308)) ) {
						srgb = rgb * float(12.92);
					} 
					else {
						srgb = float(1.055) * hls::pow(rgb, float(0.4167)) - float(0.055);
					}

					gate(srgb, float(1), float(0));
					
					out_srgb.rgb[i][j] = ap_uint<8>(float( hls::round(srgb * float(255.0)) ));

				}
				if (i == 15)
				{
					dout.write(out_srgb);
				}
			}
		}

		void adjust_tile( hls::stream<float_array> &opt_points_stream, hls::stream<float_array> &rgb_stream, 
									hls::stream<float_array> &dkl_stream, hls::stream<float_array> &inv_square_abc_stream){
			
			#pragma HLS DATAFLOW disable_start_propagation

			// stage 0 

			hls::stream<float_array> rgb_stream1, rgb_stream2;
			duplicate_stream<float_array, 16>(rgb_stream1, rgb_stream2, rgb_stream);

			// stage 1

			hls::stream<float_array> min_p_stream, max_p_stream;
			min_p_max_p_loop(max_p_stream, min_p_stream, rgb_stream1, dkl_stream, inv_square_abc_stream);

			// stage 2

			hls::stream<float_array> min_p_stream1, min_p_stream2, max_p_stream1, max_p_stream2;
			duplicate_stream<float_array, 16>(min_p_stream1, min_p_stream2, min_p_stream);
			duplicate_stream<float_array, 16>(max_p_stream1, max_p_stream2, max_p_stream);

			hls::stream<half_array> min_p_stream_1_half, max_p_stream_1_half;
			change_array_type_loop(min_p_stream_1_half, min_p_stream1);
			change_array_type_loop(max_p_stream_1_half, max_p_stream1);

			// stage 3

			hls::stream<half> min_max_stream_half, max_min_stream_half;
			loopMax_16_3(max_min_stream_half, min_p_stream_1_half);
			loopMin_16_3(min_max_stream_half, max_p_stream_1_half);

			hls::stream<float> min_max_stream, max_min_stream;
			change_type_loop(min_max_stream, min_max_stream_half);
			change_type_loop(max_min_stream, max_min_stream_half);

			// stage 4
			// prevent deadlock
			#pragma HLS STREAM variable=min_p_stream2 depth=512
			#pragma HLS STREAM variable=max_p_stream2 depth=512
			#pragma HLS STREAM variable=rgb_stream2 depth=512
			converge_plane_loop(opt_points_stream, max_min_stream, min_max_stream, min_p_stream2, max_p_stream2, rgb_stream2);

			// stage 5

		}

		template<typename T1, typename T2>
		void change_array_type_loop(hls::stream<T1> &out_stream, hls::stream<T2> &in_stream) {
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				T2 in = in_stream.read();
				T1 out;
				for (int j = 0; j < 3; j++) {
					change_type(out.data[j], in.data[j]);
				}
				out_stream.write(out);
			}
		}

		template<typename T1, typename T2>
		void change_type_loop(hls::stream<T1> &out_stream, hls::stream<T2> &in_stream) {
			for (int i = 0; i < 1; i++) {
				#pragma HLS PIPELINE II=1 rewind
				T2 in = in_stream.read();
				T1 out;
				change_type(out, in);
				out_stream.write(out);
			}
		}

		template<typename T1, typename T2>
		void change_type(T1 &out, const T2 &in) {
			out = T1(in);
		}

		void min_p_max_p_loop( hls::stream<float_array> &max_p_stream,  
								hls::stream<float_array> &min_p_stream,
								hls::stream<float_array> &rgb_stream,
								hls::stream<float_array> &dkl_stream,
								hls::stream<float_array> &inv_square_abc_stream)
		{
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				float_array rgb_centers_i = rgb_stream.read();
				float_array dkl_centers_i = dkl_stream.read();
				float_array inv_square_abcs_i = inv_square_abc_stream.read();
				float_array min_p_i, max_p_i;
				#pragma HLS ARRAY_PARTITION variable=min_p_i.data dim=0 complete
				#pragma HLS ARRAY_PARTITION variable=max_p_i.data dim=0 complete
				line_ell_inter(min_p_i.data, dkl_centers_i.data, min_vec_dkl, inv_square_abcs_i.data);
				line_ell_inter(max_p_i.data, dkl_centers_i.data, max_vec_dkl, inv_square_abcs_i.data);
				fix_bounds(max_p_i.data, rgb_centers_i.data);
				fix_bounds(min_p_i.data, rgb_centers_i.data);
				max_p_stream.write(max_p_i);
				min_p_stream.write(min_p_i);
			}
		}

		void line_ell_inter( float inter_points[3],  const float in_points[3],  
											const float _vec[3], const float inv_square_abc_i[3]){
			 #pragma HLS PIPELINE II=1  
			float sum = 0;
			float _inter_points[3];
			#pragma HLS ARRAY_PARTITION variable=_inter_points dim=0 complete
			for (int j = 0; j < 3; j++) {
				sum += _vec[j] * _vec[j] * inv_square_abc_i[j];
			}
			// gate(sum, line_ell_inter_sum_t(1e4), line_ell_inter_sum_t(1)); 
			float t = hls::rsqrt(sum);
			for (int j = 0; j < 3; j++) {
				_inter_points[j] = in_points[j] + t * _vec[j];
			}
			mm_3x3to3(inter_points, DKL2RGB, _inter_points);
		}

		void fix_bounds(float in_point[3], const float rgb_center[3]){
			#pragma HLS PIPELINE II=1    
			correct_bounds<0, color1, true>(in_point, rgb_center);
			correct_bounds<0, color2, true>(in_point, rgb_center);
			correct_bounds<1, color1, false>(in_point, rgb_center);
			correct_bounds<1, color2, false>(in_point, rgb_center);
		}

		template<int bound = 0, int col = 0, bool floor = true>
		void correct_bounds(float in_point[3], const float rgb_center[3])
		{
			 #pragma HLS PIPELINE   II=1 
			const float _bound = float(bound);
			float t;
			if (floor == true) {
				if (in_point[col] < _bound ) {
					t = (_bound - rgb_center[col]) / min_vec_rgb[col];
					for (int j = 0; j < 3; j++) {
						in_point[j] = rgb_center[j] +  t * min_vec_rgb[j];
					}
				}
			}
			else {
				if (in_point[col] > _bound ) {
					t = (_bound - rgb_center[col]) / min_vec_rgb[col];
					for (int j = 0; j < 3; j++) {
						in_point[j] = rgb_center[j] + t * min_vec_rgb[j];
					}
				}
			}
		}

		void converge_plane_loop( hls::stream<float_array> &opt_points_stream, hls::stream<float> &max_min_stream, 
											hls::stream<float> &min_max_stream, 
											hls::stream<float_array> &min_p_stream, 
											hls::stream<float_array> &max_p_stream,  
											hls::stream<float_array> &rgb_centers_stream)
		{
			float max_min, min_max;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				float_array min_p_i = min_p_stream.read();
				float_array max_p_i = max_p_stream.read();
				float_array rgb_centers_i = rgb_centers_stream.read();
				if (i==0) {
					max_min = max_min_stream.read();
					min_max = min_max_stream.read();
				}
				float col_plane = (max_min + min_max) / 2;
				gate(col_plane, float(1), float(0));
				float_array opt_points_i;
				#pragma HLS ARRAY_PARTITION variable=opt_points_i.data dim=0 complete
				converge_plane( opt_points_i.data, col_plane, min_p_i.data, max_p_i.data, rgb_centers_i.data);
				opt_points_stream.write(opt_points_i);
			}
		}

		void converge_plane(float opt_points_i[3], const float &col_plane, const float min_p_i[3], 
									const float max_p_i[3], const float rgb_centers_i[3]){
			 #pragma HLS PIPELINE II=1   
			if (hls::isless(col_plane, min_p_i[opt_channel])) {
					opt_points_i[0] = min_p_i[0];
					opt_points_i[1] = min_p_i[1];
					opt_points_i[2] = min_p_i[2];
			}
			else if (hls::isgreater(col_plane, max_p_i[opt_channel])) {
				opt_points_i[0] = max_p_i[0];
				opt_points_i[1] = max_p_i[1];
				opt_points_i[2] = max_p_i[2];
			}
			else {
				// converged on a plane
				float t = (col_plane - rgb_centers_i[opt_channel]) / min_vec_rgb[opt_channel];
				opt_points_i[opt_channel] = col_plane;
				opt_points_i[color1] = rgb_centers_i[color1] + t * min_vec_rgb[color1];
				opt_points_i[color2] = rgb_centers_i[color2] + t * min_vec_rgb[color2];
			}
		}

		template<typename T1, typename T2>
		void loopMax_16_3(hls::stream<T1> &max_stream, hls::stream<T2> &in_stream) {
			T1 max;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1   rewind
				T2 in = in_stream.read();
				#pragma HLS ARRAY_PARTITION variable=in.data dim=0 complete
				if (i == 0) {
					max = in.data[opt_channel];
				}
				else {
					my_max(max, max, in.data[opt_channel]);
				}
				if (i == 15) {
					max_stream.write(max);
				}
			}
		}

		template<typename T1, typename T2>
		void loopMin_16_3(hls::stream<T1> &min_stream, hls::stream<T2> &in_stream) {
			T1 min;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1  rewind
				T2 in = in_stream.read();
				#pragma HLS ARRAY_PARTITION variable=in.data dim=0 complete

				if (i == 0) {
					min = in.data[opt_channel];
				}
				else {
					my_min(min, min, in.data[opt_channel]);
				}
				
				if (i == 15) {
					min_stream.write(min);
				}
			}
		}

		template<typename T>
		void my_min ( T &out, const T in1, const T in2) {
			#pragma HLS INLINE
			// if (in1 < in2) {
			if(hls::isless(in1, in2)){
				out = in1;
			}
			else {
				out = in2;
			}
		}

		template<typename T>
		void my_max ( T &out, const T in1, const T in2) {
			#pragma HLS INLINE
			// if (in1 > in2) {
			if(hls::isgreater(in1, in2)	){
				out = in1;
			}
			else {
				out = in2;
			}
		}


	};
}