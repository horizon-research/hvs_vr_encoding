#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>


#define HLS_PIPELINE_II_Set _Pragma("HLS PIPELINE II=16")

typedef ap_ufixed<16, 0> abc_t;
typedef ap_ufixed<16, 0, AP_RND, AP_SAT> rgb_t; // prevent SAT in DKL to RGB
typedef ap_ufixed<29, 14> inv_square_t; // 14 for 1e4 int out, 
										// 14 point for 1e-4 in   16 is because type casting default is TRN, we need to preserve the abc
										// add 2 point for precision
typedef ap_ufixed<16, 14, AP_RND, AP_SAT> line_ell_inter_sum_t;
typedef ap_ufixed<23, 14> rsqrt_t; // add 2 point for precision 
typedef ap_ufixed<9, 0, AP_RND, AP_SAT> line_ell_inter_t_t;

typedef ap_fixed<16, 1, AP_TRN, AP_SAT> dkl_t;
typedef ap_fixed<17, 1> vec_t;
typedef ap_fixed<17, 9> dkl2rgb_t;
typedef ap_fixed<16, 5, AP_RND, AP_SAT> converge_t_t; // -10 - 10
typedef ap_fixed<16, 5, AP_RND, AP_SAT> fix_bound_t_t; // -10 - 10
typedef ap_fixed<20, 4, AP_RND, AP_SAT> rgb_not_fixed_t; // prevent SAT in DKL to RGB



struct agg_outputs {
	rgb_t rgb[16][3];
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
		hls::stream< agg_outputs > &dout,
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
	class Tile_color_optimizer
	{
		public:
		vec_t max_vec_rgb[3], min_vec_rgb[3], max_vec_dkl[3], min_vec_dkl[3];
		dkl2rgb_t DKL2RGB[3][3] ;

		rgb_t rgb_centers [16][3];
		dkl_t dkl_centers [16][3];
		inv_square_t inv_square_abc [16][3];


		void operator()(agg_outputs &out , agg_inputs &in) {
		HLS_PIPELINE_II_Set
		// #pragma HLS INLINE
		// #pragma HLS INLINE recursive

		#pragma HLS ARRAY_PARTITION variable=max_vec_rgb dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=min_vec_rgb dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=max_vec_dkl dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=min_vec_dkl dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=rgb_centers dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=dkl_centers dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=inv_square_abc dim=0 complete

		#pragma HLS ARRAY_PARTITION variable=out.rgb dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=in.as dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=in.bs dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=in.cs dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=in.ds dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=in.ks dim=0 complete
		#pragma HLS ARRAY_PARTITION variable=in.ls dim=0 complete

		DKL2RGB[0][0] = 10.60864043;  DKL2RGB[0][1] = 23.50260678;  DKL2RGB[0][2] = 21.01613594;
        DKL2RGB[1][0] = -3.17452434;  DKL2RGB[1][1] = -22.53568763; DKL2RGB[1][2] = -20.37323115;
        DKL2RGB[2][0] = 0.20807273;   DKL2RGB[2][1] = 154.02866473; DKL2RGB[2][2] = 153.78039361;
		

		// Initialization
		if (opt_channel == Color::BLUE) {
			max_vec_rgb[0] = 0.14766317;  max_vec_rgb[1] = -0.13674196; max_vec_rgb[2] = 0.97936063;
			min_vec_rgb[0] = -0.14766317; min_vec_rgb[1] = 0.13674196;  min_vec_rgb[2] = -0.97936063;
			max_vec_dkl[0] = 0.082895;    max_vec_dkl[1] = 0.205911;    max_vec_dkl[2] = 0.430501;
			min_vec_dkl[0] = -0.082895;   min_vec_dkl[1] = -0.205911;   min_vec_dkl[2] = -0.430501;
		}
		else {
			max_vec_rgb[0] = 0.61894476;  max_vec_rgb[1] = -0.24312686; max_vec_rgb[2] = 0.62345751;
            min_vec_rgb[0] = -0.61894476; min_vec_rgb[1] = 0.24312686;  min_vec_rgb[2] = -0.62345751;
            max_vec_dkl[0] = 0.5015333;   max_vec_dkl[1] = 0.0126745;   max_vec_dkl[2] = 0.0271685;
			min_vec_dkl[0] = -0.5015333;  min_vec_dkl[1] = -0.0126745;  min_vec_dkl[2] = -0.0271685;
		}

			// Read dkl from input
			for(int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				dkl_centers[i][0] = in.ds[i];
				dkl_centers[i][1] = in.ks[i];
				dkl_centers[i][2] = in.ls[i];
			}

			// DKL to RGB - done
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL factor=1
				// #pragma HLS allocation function instances=mm_3x3to3<T1, T2, T3> limit=1
				mm_3x3to3(rgb_centers[i], DKL2RGB, dkl_centers[i]);
			}

			// compute inv_square_abc - done
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL factor=1
				// HLS_PIPELINE_II_Set
				// #pragma HLS allocation function instances=inv_square<T1, T2> limit=1
				// no need to gate, gate is done in the software
				// in range : 1e-2 ~1
				inv_square(inv_square_abc[i], in.as[i], in.bs[i], in.cs[i]);
				// out range : 1 ~ 1e4, u<16, 14> is enough, log2(1e4) = 13.28
			}

			rgb_t opt_points[16][3] ;
			#pragma HLS ARRAY_PARTITION variable=opt_points dim=0 complete
			adjust_tile(opt_points);

			for(int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				out.rgb[i][0] = opt_points[i][0];
				out.rgb[i][1] = opt_points[i][1];
				out.rgb[i][2] = opt_points[i][2];
			}

		}

		template<typename T1, typename T2>
		void inv_square(T1 inv_square_abc_i[3], T2 &a, T2 &b, T2 &c){
			#pragma HLS PIPELINE II=1
			inv_square_abc_i[0] = hls::recip(inv_square_t(a * a));
			inv_square_abc_i[1] = hls::recip(inv_square_t(b * b));
			inv_square_abc_i[2] = hls::recip(inv_square_t(c * c));
		}

		template<typename T1, typename T2, typename T3>
		void mm_3x3to3 (  T1 m3_out[3], T2 m33[3][3], T3 m3_in[3]) {
			// //HLS_PIPELINE_II_Set
			#pragma HLS ARRAY_PARTITION variable=m3_out dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=m33 dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=m3_in dim=0 complete
			for (int i = 0; i < 3; i++) {
				#pragma HLS UNROLL
				m3_out[i] = m33[i][0] * m3_in[0] + m33[i][1] * m3_in[1] + m33[i][2] * m3_in[2];
			}
		}

		template<typename T>
		void gate( T &out, T max, T min) {
			#pragma HLS PIPELINE II=1
			if (out > max) {
				out = max;
			}
			else if (out < min) {
				out = min;
			}
		}

		void adjust_tile( rgb_t opt_points[16][3] ){
			#pragma HLS ARRAY_PARTITION variable=opt_points dim=0 complete
			HLS_PIPELINE_II_Set
			rgb_not_fixed_t min_p[16][3], max_p[16][3];
			#pragma HLS ARRAY_PARTITION variable=min_p dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=max_p dim=0 complete

			for (int i=0; i < 16; i++)
			{	
				// #pragma HLS allocation function instances=line_ell_inter limit=1
				// #pragma HLS allocation function instances=fix_bounds limit=1
				#pragma HLS UNROLL factor=1
				line_ell_inter(min_p[i], dkl_centers[i], min_vec_dkl, inv_square_abc[i]); 
				fix_bounds(min_p[i], rgb_centers[i]);
			}

			for (int i=0; i < 16; i++)
			{	
				// #pragma HLS allocation function instances=line_ell_inter limit=1
				// #pragma HLS allocation function instances=fix_bounds limit=1
				#pragma HLS UNROLL factor=1
				line_ell_inter(max_p[i], dkl_centers[i], max_vec_dkl, inv_square_abc[i]);
				fix_bounds(max_p[i], rgb_centers[i]);
			}

			rgb_not_fixed_t min_max, max_min;
			treeMaxOpt_16_3(max_min, min_p);
			treeMinOpt_16_3(min_max, max_p);

			rgb_not_fixed_t col_plane = (max_min + min_max) / 2;
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL factor=1
				converge_plane(col_plane, min_p[i], max_p[i], opt_points[i], rgb_centers[i]);
			}
		}

		void converge_plane(rgb_not_fixed_t &col_plane, rgb_not_fixed_t min_p_i[3], 
									rgb_not_fixed_t max_p_i[3], rgb_t opt_points_i[3], rgb_t rgb_centers_i[3]){
			#pragma HLS PIPELINE II=1
			if (col_plane < min_p_i[opt_channel]) {
					opt_points_i[0] = min_p_i[0];
					opt_points_i[1] = min_p_i[1];
					opt_points_i[2] = min_p_i[2];
				}
				else if (col_plane > max_p_i[opt_channel]) {
					opt_points_i[0] = max_p_i[0];
					opt_points_i[1] = max_p_i[1];
					opt_points_i[2] = max_p_i[2];
				}
				else {
					// converged on a plane
					converge_t_t t = (col_plane - rgb_centers_i[opt_channel]) / min_vec_rgb[opt_channel];
					for (int j = 0; j < 3; j++) {
						#pragma HLS UNROLL
						// rgb has SAT
						opt_points_i[j] = rgb_centers_i[j]  + t * min_vec_rgb[j];
					}
				}
		}

		void line_ell_inter( rgb_not_fixed_t inter_points[3],  dkl_t in_points[3],  vec_t _vec[3], inv_square_t inv_square_abc_i[3]){
			#pragma HLS PIPELINE II=1
			#pragma HLS ARRAY_PARTITION variable=inter_points dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=in_points dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=_vec dim=0 complete
			line_ell_inter_sum_t sum = 0;
			line_ell_inter_t_t t;
			dkl_t _inter_points[3];

			for (int j = 0; j < 3; j++) {
				// //HLS_PIPELINE_II_Set
				#pragma HLS UNROLL
				sum += _vec[j] * _vec[j] * inv_square_abc_i[j];
			}

			gate(sum, line_ell_inter_sum_t(1e4), line_ell_inter_sum_t(1)); // gate sum to 1-1e4

			rsqrt_t _t;
			rsqrt:{
				_t = hls::rsqrt<23, 14>(rsqrt_t(sum)); // rsqrt: In   1-1e4: 14 int,   out: 1e-2 - 1: 7 point
			}
			
			t = line_ell_inter_t_t(_t) ;// leave only 9 point

			// dkl inter, dkl has SAT
			for (int j = 0; j < 3; j++) {
				#pragma HLS UNROLL
				_inter_points[j] = in_points[j] + t * _vec[j];
			}
			// DKL2RGB, rgb has SAT
			// #pragma HLS allocation function instances=mm_3x3to3 limit=1
			mm_3x3to3(inter_points, DKL2RGB, _inter_points);
		}

		void fix_bounds(rgb_not_fixed_t in_point[3], rgb_t rgb_center[3]){
			#pragma HLS ARRAY_PARTITION variable=in_point dim=0 complete
			#pragma HLS PIPELINE II=1
			correct_bounds<0, color1, true>(in_point, rgb_center);
			correct_bounds<0, color2, true>(in_point, rgb_center);
			correct_bounds<1, color1, false>(in_point, rgb_center);
			correct_bounds<1, color2, false>(in_point, rgb_center);
		}

		

		template<int bound = 0, int col = 0, bool floor = true>
		void correct_bounds(rgb_not_fixed_t in_point[3], rgb_t rgb_center[3])
		{
			#pragma HLS PIPELINE II=1
			#pragma HLS ARRAY_PARTITION variable=in_point dim=0 complete
			const auto _bound = rgb_not_fixed_t(bound);
			fix_bound_t_t t;
			if (floor == true) {
				if (in_point[col] < _bound ) {
					t = (_bound - rgb_center[col]) / min_vec_rgb[col];
					for (int j = 0; j < 3; j++) {
						#pragma HLS UNROLL
						in_point[j] = rgb_center[j] +  t * min_vec_rgb[j];
					}
				}
			}
			else {
				if (in_point[col] > _bound ) {
					t = (_bound - rgb_center[col]) / min_vec_rgb[col];
					for (int j = 0; j < 3; j++) {
						#pragma HLS UNROLL
						in_point[j] = rgb_center[j] + t * min_vec_rgb[j];
					}
				}
			}
		}

		template<typename T>
		void treeMinOpt_16_3(T &min, T array[16][3]) {
			#pragma HLS ARRAY_PARTITION variable=array dim=0 complete
			T temp1[8], temp2[4], temp3[2];
			#pragma HLS ARRAY_PARTITION variable=temp1 dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=temp2 dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=temp3 dim=0 complete
			for (int i = 0; i < 8; ++i) {
				my_min(temp1[i], array[2 * i][opt_channel], array[2 * i + 1][opt_channel]);
			}

			// Second level of comparisons
			for (int i = 0; i < 4; ++i) {
				my_min(temp2[i], temp1[2 * i], temp1[2 * i + 1] );
			}

			// Third level of comparisons
			for (int i = 0; i < 2; ++i) {
				my_min(temp3[i] , temp2[2 * i], temp2[2 * i + 1]);
			}

			// Final comparison
			my_min(min, temp3[0], temp3[1]);
		}

		template<typename T>
		void my_min ( T &out, T &in1, T &in2) {
			//HLS_PIPELINE_II_Set
			if (in1 < in2) {
				out = in1;
			}
			else {
				out = in2;
			}
		}

		template<typename T>
		void my_max ( T &out, T &in1, T &in2) {
			//HLS_PIPELINE_II_Set
			if (in1 > in2) {
				out = in1;
			}
			else {
				out = in2;
			}
		}

		template<typename T>
		void treeMaxOpt_16_3(T &max, T array[16][3]) {
			//HLS_PIPELINE_II_Set
			#pragma HLS ARRAY_PARTITION variable=array dim=0 complete
			
			T temp1[8], temp2[4], temp3[2];
						#pragma HLS ARRAY_PARTITION variable=temp1 dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=temp2 dim=0 complete
			#pragma HLS ARRAY_PARTITION variable=temp3 dim=0 complete

			// Initial comparisons (level 1)
			for (int i = 0; i < 8; ++i) {
				my_max( temp1[i], array[2 * i][opt_channel], array[2 * i + 1][opt_channel] );
			}

			// Second level of comparisons
			for (int i = 0; i < 4; ++i) {
				my_max( temp2[i], temp1[2 * i], temp1[2 * i + 1] );
			}

			// Third level of comparisons
			for (int i = 0; i < 2; ++i) {
				my_max(temp3[i], temp2[2 * i], temp2[2 * i + 1]);
			}

			// Final comparison
			my_max(max, temp3[0], temp3[1]);
		}




	};
}
