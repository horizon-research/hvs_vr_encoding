#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>

typedef ap_ufixed<16, 0> abc_t;
typedef ap_ufixed<16, 0, AP_RND, AP_SAT> rgb_t; // prevent SAT in DKL to RGB
typedef ap_ufixed<32, 14> inv_square_t; // 14 for 1e4 int out, 
										// 14 point for 1e-4 in   16 is because type casting default is TRN, we need to preserve the abc
										// add 2 point for precision
typedef ap_ufixed<16, 14, AP_RND, AP_SAT> line_ell_inter_sum_t;
typedef ap_ufixed<28, 14> rsqrt_t; // add 2 point for precision 
typedef ap_ufixed<9, 0, AP_RND, AP_SAT> line_ell_inter_t_t;

typedef ap_fixed<16, 1, AP_TRN, AP_SAT> dkl_t;
typedef ap_fixed<17, 1> vec_t;
typedef ap_fixed<20, 10> dkl2rgb_t;
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

namespace vr_prototype
{

void tile_color_optimizer_func(
		hls::stream< agg_outputs > &dout,
		hls::stream< agg_inputs > &din
        );
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
			#pragma HLS PIPELINE II=1
			#pragma HLS INLINE recursive

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
				#pragma HLS UNROLL
				mm_3x3to3(rgb_centers[i], DKL2RGB, dkl_centers[i]);
			}

			// compute inv_square_abc - done
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				// no need to gate, gate is done in the software
				// in range : 1e-2 ~1
				inv_square_abc[i][0] = hls::recip(inv_square_t(in.as[i] * in.as[i]));
				inv_square_abc[i][1] = hls::recip(inv_square_t(in.bs[i] * in.bs[i]));
				inv_square_abc[i][2] = hls::recip(inv_square_t(in.cs[i] * in.cs[i]));
				// out range : 1 ~ 1e4, u<16, 14> is enough, log2(1e4) = 13.28
			}



			// print dkl, rgb, inv_square_abc for debugging
			// 打印 DKL 数据
// for(int i = 0; i < 16; i++) {
//    std::cout << "DKL[" << i << "] = {" << dkl_centers[i][0] << ", " << dkl_centers[i][1] << ", " << dkl_centers[i][2] << "}\n";
// }


// for(int i = 0; i < 16; i++) {
//    std::cout << "RGB[" << i << "] = {" << rgb_centers[i][0] << ", " << rgb_centers[i][1] << ", " << rgb_centers[i][2] << "}\n";
// }

// for(int i = 0; i < 16; i++) {
// 	std::cout << "abc[" << i << "] = {" << in.as[i] << ", " << in.bs[i] << ", " << in.cs[i] << "}\n";
// }

// for(int i = 0; i < 16; i++) {
//    std::cout << "abc**2[" << i << "] = {" << in.as[i] * in.as[i] << ", " << in.bs[i] * in.bs[i] << ", " <<  in.cs[i] * in.cs[i] << "}\n";
// }

// for(int i = 0; i < 16; i++) {
//    std::cout << "abc**2[" << i << "] = {" <<inv_square_t(in.as[i] * in.as[i]) << ", " << inv_square_t(in.bs[i] * in.bs[i]) << ", " << inv_square_t(in.cs[i] * in.cs[i]) << "}\n";
// }


// for(int i = 0; i < 16; i++) {
//    std::cout << "inv_square_abc[" << i << "] = {"  << inv_square_abc[i][0] << ", " << inv_square_abc[i][1] << ", " << inv_square_abc[i][2] << "}\n";
// }


			rgb_t opt_points[16][3] ;
			adjust_tile(opt_points);

			for(int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				out.rgb[i][0] = opt_points[i][0];
				out.rgb[i][1] = opt_points[i][1];
				out.rgb[i][2] = opt_points[i][2];
			}

		}

		template<typename T1, typename T2, typename T3>
		void mm_3x3to3 (  T1 m3_out, T2 m33, T3 m3_in) {
			for (int i = 0; i < 3; i++) {
				#pragma HLS UNROLL
				m3_out[i] = m33[i][0] * m3_in[0] + m33[i][1] * m3_in[1] + m33[i][2] * m3_in[2];
			}
		}

		template<typename T>
		void gate( T &out, T max, T min) {
			if (out > max) {
				out = max;
			}
			else if (out < min) {
				out = min;
			}
		}

		void adjust_tile( rgb_t opt_points[16][3] ){
			rgb_not_fixed_t min_p[16][3], max_p[16][3];
			line_ell_inter(min_p, dkl_centers, min_vec_dkl); // done
			fix_bounds(min_p);

// 			for(int i = 0; i < 16; i++) {
//    std::cout << "min_p[" << i << "] = {" << min_p[i][0] << ", " << min_p[i][1] << ", " << min_p[i][2] << "}\n";
// }

			line_ell_inter(max_p, dkl_centers, max_vec_dkl); // done
			fix_bounds(max_p);    // No fix bound for now


// 			for(int i = 0; i < 16; i++) {
//    std::cout << "max_p[" << i << "] = {" << max_p[i][0] << ", " << max_p[i][1] << ", " << max_p[i][2] << "}\n";
// }

			rgb_not_fixed_t min_max, max_min;
			treeMaxOpt_16_3(max_min, min_p);
			treeMinOpt_16_3(min_max, max_p);

			// std::cout << "max_min = {" << max_min << "}\n";
			// std::cout << "min_max = {" << min_max << "}\n";

			rgb_not_fixed_t col_plane = (max_min + min_max) / 2;
			// std::cout << "col_plane = {" << col_plane << "}\n";

			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				if (col_plane < min_p[i][opt_channel]) {
					opt_points[i][0] = min_p[i][0];
					opt_points[i][1] = min_p[i][1];
					opt_points[i][2] = min_p[i][2];
				}
				else if (col_plane > max_p[i][opt_channel]) {
					opt_points[i][0] = max_p[i][0];
					opt_points[i][1] = max_p[i][1];
					opt_points[i][2] = max_p[i][2];
				}
				else {
					// converged on a plane
					converge_t_t t = (col_plane - rgb_centers[i][opt_channel]) / min_vec_rgb[opt_channel];
					for (int j = 0; j < 3; j++) {
						#pragma HLS UNROLL
						// rgb has SAT
						opt_points[i][j] = rgb_centers[i][j]  + t * min_vec_rgb[j];
						// std::cout << "t = {" << t << "}\n";
						// std::cout << "rgb_centers = {" << rgb_centers[i][j] << "}\n";
						// std::cout << "min_vec_rgb = {" << min_vec_rgb[j] << "}\n";
					}
				}
			}
		}

		void line_ell_inter( rgb_not_fixed_t inter_points[16][3],  dkl_t in_points[16][3],  vec_t _vec[3]){
			line_ell_inter_sum_t sum[16];
			line_ell_inter_t_t t[16];
			dkl_t _inter_points[16][3];

			// for (int i = 0; i < 3; i++) {
			// 	std::cout << "_vec[" << i << "] = {" << _vec[i] << "}\n";
			// }

			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				sum[i] = 0;
				for (int j = 0; j < 3; j++) {
					#pragma HLS UNROLL
					sum[i] += _vec[j] * _vec[j] * inv_square_abc[i][j];
				}


				

				gate(sum[i], line_ell_inter_sum_t(1e4), line_ell_inter_sum_t(1)); // gate sum to 1-1e4

				rsqrt_t _t = hls::rsqrt<28, 14>(rsqrt_t(sum[i])); // rsqrt: In   1-1e4: 14 int,   out: 1e-2 - 1: 7 point


				t[i] = line_ell_inter_t_t(_t) ;// leave only 9 point


				// dkl inter, dkl has SAT
				for (int j = 0; j < 3; j++) {
					#pragma HLS UNROLL
					_inter_points[i][j] = in_points[i][j] + t[i] * _vec[j];
				}
				// DKL2RGB, rgb has SAT
				mm_3x3to3(inter_points[i], DKL2RGB, _inter_points[i]);
			}
			// for(int i = 0; i < 16; i++) {
			// 		std::cout << "inv_square_abc [" << i << "] = {" << inv_square_abc[i][0] << ", " << inv_square_abc[i][1] << ", " << inv_square_abc[i][2] << "}\n";
   			// 			std::cout << "sum[" << i << "] = {" << sum[i] << "}\n";
			// 				std::cout << "t[" << i << "] = {" << t[i] << "}\n";
			// 				std::cout << "_inter_points[" << i << "] = {" << _inter_points[i][0] << ", " << _inter_points[i][1] << ", " << _inter_points[i][2] << "}\n";
			// 				std::cout << "inter_points[" << i << "] = {" << inter_points[i][0] << ", " << inter_points[i][1] << ", " << inter_points[i][2] << "}\n";
			// 	}
		}

		void fix_bounds(rgb_not_fixed_t in_points[16][3]){
			correct_bounds<0, color1, true>(in_points);
			correct_bounds<0, color2, true>(in_points);
			correct_bounds<1, color1, false>(in_points);
			correct_bounds<1, color2, false>(in_points);
		}

		template<int bound = 0, int col = 0, bool floor = true>
		void correct_bounds(rgb_not_fixed_t in_points[16][3])
		{

			// 			for(int i = 0; i < 16; i++) {
			// 	std::cout << "in_points[" << i << "] = {" << in_points[i][0] << ", " << in_points[i][1] << ", " << in_points[i][2] << "}\n";
			// }
			const auto _bound = rgb_not_fixed_t(bound);
			fix_bound_t_t t[16];
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				if (floor == true) {
					if (in_points[i][col] < _bound ) {
						t[i] = (_bound - rgb_centers[i][col]) / min_vec_rgb[col];
						// std::cout << "_bound[" << i << "][" << "] = {" << _bound << "}\n";
						// std::cout << "in_points[" << i << "][" << "col] = {" << in_points[i][col] << "}\n";
						// std::cout << "min_vec_rgb[" << col << "] = {" << min_vec_rgb[col] << "}\n";

						for (int j = 0; j < 3; j++) {
							#pragma HLS UNROLL
							in_points[i][j] = rgb_centers[i][j] +  t[i] * min_vec_rgb[j];
							// std::cout << "rgb_centers[" << i << "][" << j << "] = {" << rgb_centers[i][j] << "}\n";
							// std::cout << "t[" << i << "] = {" << t[i] << "}\n";
							// std::cout << "min_vec_rgb[" << j << "] = {" << min_vec_rgb[j] << "}\n";
							// std::cout << "in_points[" << i << "][" << j << "] = {" << in_points[i][j] << "}\n";
						}
					}
				}
				else {
					if (in_points[i][col] > _bound ) {
						t[i] = (_bound - rgb_centers[i][col]) / min_vec_rgb[col];
						for (int j = 0; j < 3; j++) {
							#pragma HLS UNROLL
							in_points[i][j] = rgb_centers[i][j] + t[i] * min_vec_rgb[j];
						}
					}
				}
			}
			// print ti and in_points for debugging
			
			// for(int i = 0; i < 16; i++) {
			// 	std::cout << "t[" << i << "] = {" << t[i] << "}\n";
			// }
			// for(int i = 0; i < 16; i++) {
			// 	std::cout << "in_points[" << i << "] = {" << in_points[i][0] << ", " << in_points[i][1] << ", " << in_points[i][2] << "}\n";
			// }
		}

		template<typename T>
		void treeMinOpt_16_3(T &min, T array[16][3]) {
			T temp1[8], temp2[4], temp3[2];

			// Initial comparisons (level 1)
			for (int i = 0; i < 8; ++i) {
				#pragma HLS UNROLL
				my_min(temp1[i], array[2 * i][opt_channel], array[2 * i + 1][opt_channel]);
			}

			// Second level of comparisons
			for (int i = 0; i < 4; ++i) {
				#pragma HLS UNROLL
				my_min(temp2[i], temp1[2 * i], temp1[2 * i + 1] );
			}

			// Third level of comparisons
			for (int i = 0; i < 2; ++i) {
				#pragma HLS UNROLL
				my_min(temp3[i] , temp2[2 * i], temp2[2 * i + 1]);
			}

			// Final comparison
			my_min(min, temp3[0], temp3[1]);
		}

		template<typename T>
		void my_min ( T &out, T &in1, T &in2) {
			if (in1 < in2) {
				out = in1;
			}
			else {
				out = in2;
			}
		}

		template<typename T>
		void my_max ( T &out, T &in1, T &in2) {
			if (in1 > in2) {
				out = in1;
			}
			else {
				out = in2;
			}
		}

		template<typename T>
		void treeMaxOpt_16_3(T &max, T array[16][3]) {
			T temp1[8], temp2[4], temp3[2];

			// Initial comparisons (level 1)
			for (int i = 0; i < 8; ++i) {
				#pragma HLS UNROLL
				my_max( temp1[i], array[2 * i][opt_channel], array[2 * i + 1][opt_channel] );
			}

			// Second level of comparisons
			for (int i = 0; i < 4; ++i) {
				#pragma HLS UNROLL
				my_max( temp2[i], temp1[2 * i], temp1[2 * i + 1] );
			}

			// Third level of comparisons
			for (int i = 0; i < 2; ++i) {
				#pragma HLS UNROLL
				my_max(temp3[i], temp2[2 * i], temp2[2 * i + 1]);
			}

			// Final comparison
			my_max(max, temp3[0], temp3[1]);
		}




	};
}
