#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>

typedef ap_ufixed<16, 0> ufixed_16_0_t;
typedef ap_ufixed<16, 16> ufixed_16_16_t;
typedef ap_ufixed<16, 16> ufixed_17_16_t;
typedef ap_ufixed<16, 6> ufixed_16_6_t;
typedef ap_fixed<16, 0> fixed_16_0_t;
typedef ap_fixed<16, 8> fixed_16_8_t;
typedef ap_fixed<16, 9> fixed_16_9_t;
typedef ap_ufixed<33, 16> ufixed_33_16_t;
typedef ap_uint<8> ap_uint8_t;


struct agg_outputs {
	ufixed_16_0_t rgb[16][3];
};

struct agg_inputs {
	ufixed_16_0_t as[16];
	ufixed_16_0_t bs[16];
	ufixed_16_0_t cs[16];
	fixed_16_0_t ds[16];
	fixed_16_0_t ks[16];
	fixed_16_0_t ls[16];
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
		fixed_16_0_t max_vec_rgb[3], min_vec_rgb[3], max_vec_dkl[3], min_vec_dkl[3];
		fixed_16_9_t DKL2RGB[3][3] ;

		ufixed_16_0_t rgb_centers [16][3];
		fixed_16_0_t dkl_centers [16][3];
		ufixed_17_16_t inv_square_abc [16][3];



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

			// DKL to RGB
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				mm_3x3to3(rgb_centers[i], DKL2RGB, dkl_centers[i]);
			}

			// compute inv_square_abc
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				// divide 0 check is needed
				inv_square_abc[i][0] = hls::recip(ufixed_33_16_t(in.as[i] * in.as[i]));
				inv_square_abc[i][1] = hls::recip(ufixed_33_16_t(in.bs[i] * in.bs[i]));
				inv_square_abc[i][2] = hls::recip(ufixed_33_16_t(in.cs[i] * in.cs[i]));
			}

			// print dkl, rgb, inv_square_abc for debugging
			// 打印 DKL 数据
// for(int i = 0; i < 16; i++) {
//     std::cout << "DKL[" << i << "] = {" << dkl_centers[i][0] << ", " << dkl_centers[i][1] << ", " << dkl_centers[i][2] << "}\n";
// }

// // ...DKL 到 RGB 的转换代码...

// // 打印 RGB 数据
// for(int i = 0; i < 16; i++) {
//     std::cout << "RGB[" << i << "] = {" << rgb_centers[i][0] << ", " << rgb_centers[i][1] << ", " << rgb_centers[i][2] << "}\n";
// }

// // print abc 
// for(int i = 0; i < 16; i++) {
// 	std::cout << "abc[" << i << "] = {" << in.as[i] << ", " << in.bs[i] << ", " << in.cs[i] << "}\n";
// }

// ...计算 inv_square_abc 代码...

// 打印 inv_square_abc 数据
// for(int i = 0; i < 16; i++) {
//     std::cout << "inv_square_abc[" << i << "] = {" << inv_square_abc[i][0] << ", " << inv_square_abc[i][1] << ", " << inv_square_abc[i][2] << "}\n";
// }


			ufixed_16_0_t opt_points[16][3] ;
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

		void adjust_tile( ufixed_16_0_t opt_points[16][3] ){
			fixed_16_0_t min_p[16][3], max_p[16][3];
			line_ell_inter(min_p, dkl_centers, min_vec_dkl);
			fix_bounds(min_p);

// for(int i = 0; i < 16; i++) {
//     std::cout << "min_p[" << i << "] = {" << min_p[i][0] << ", " << min_p[i][1] << ", " << min_p[i][2] << "}\n";
// }


			line_ell_inter(max_p, dkl_centers, max_vec_dkl);
			fix_bounds(max_p);

			fixed_16_0_t min_max, max_min;
			treeMaxOpt_16_3(max_min, min_p);
			treeMinOpt_16_3(min_max, max_p);

			ufixed_16_0_t col_plane = (max_min + min_max) << 1;
			

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
					ufixed_16_6_t t = col_plane - rgb_centers[i][opt_channel] / min_vec_rgb[opt_channel];
					for (int j = 0; j < 3; j++) {
						#pragma HLS UNROLL
						opt_points[i][j] = rgb_centers[i][j]  + t * min_vec_dkl[j];
					}
				}
			}
		}

		void line_ell_inter( fixed_16_0_t inter_points[16][3], fixed_16_0_t in_points[16][3],  fixed_16_0_t _vec[3]){
			ufixed_16_16_t sum[16];
			ufixed_16_0_t t[16];

			for (int i = 0; i < 3; i++) {
				std::cout << "_vec[" << i << "] = {" << _vec[i] << "}\n";
			}
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				for (int j = 0; j < 3; j++) {
					#pragma HLS UNROLL
					sum[i] += _vec[j] * _vec[j] * inv_square_abc[i][j];
				}

				t[i] = hls::rsqrt<16,16>(sum[i]);

				ufixed_16_0_t _inter_points[16][3];
				for (int j = 0; j < 3; j++) {
					#pragma HLS UNROLL
					_inter_points[i][j] = in_points[i][j] + t[i] * _vec[j];
				}
				// DKL2RGB
				mm_3x3to3(inter_points[i], DKL2RGB, _inter_points[i]);

				for(int i = 0; i < 16; i++) {
    						std::cout << "sum[" << i << "] = {" << sum[i] << "}\n";
							std::cout << "t[" << i << "] = {" << t[i] << "}\n";
							std::cout << "inter_points[" << i << "] = {" << _inter_points[i][0] << ", " << _inter_points[i][1] << ", " << _inter_points[i][2] << "}\n";

				}
			}
		}

		void fix_bounds(fixed_16_0_t in_points[16][3]){
			correct_bounds<0, color1, true>(in_points);
			correct_bounds<0, color2, true>(in_points);
			correct_bounds<1, color1, false>(in_points);
			correct_bounds<1, color2, false>(in_points);
		}

		template<int bound = 0, int col = 0, bool floor = true>
		void correct_bounds(fixed_16_0_t in_points[16][3])
		{
			const auto _bound = ap_uint<1>(bound);
			for (int i = 0; i < 16; i++) {
				#pragma HLS UNROLL
				if (floor == true) {
					if (in_points[i][col] < _bound ) {
						ufixed_16_6_t t = _bound - in_points[i][col] / min_vec_dkl[col];
						for (int j = 0; j < 3; j++) {
							#pragma HLS UNROLL
							in_points[i][j] += t * min_vec_dkl[j];
						}
					}
				}
				else {
					if (in_points[i][col] > _bound ) {
						ufixed_16_6_t t = _bound - in_points[i][col], min_vec_dkl[col];
						for (int j = 0; j < 3; j++) {
							#pragma HLS UNROLL
							in_points[i][j] += t * min_vec_dkl[j];
						}
					}
				}
			}
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
