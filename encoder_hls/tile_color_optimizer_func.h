#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>
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

struct dkl_t_array {
    dkl_t data[3];
};

struct inv_square_t_array {
    inv_square_t data[3];
};

struct rgb_t_array {
    rgb_t data[3];
};

struct rgb_not_fixed_t_array {
    rgb_not_fixed_t data[3];
};

struct agg_outputs_rgb {
	rgb_t rgb[16][3];
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
			const vec_t max_vec_rgb[3] = {0.14766317, -0.13674196, 0.97936063};
			const vec_t min_vec_rgb[3] = {-0.14766317, 0.13674196, -0.97936063};
			const vec_t max_vec_dkl[3] = {0.082895, 0.205911, 0.430501};
			const vec_t min_vec_dkl[3] = {-0.082895, -0.205911, -0.430501};

			//  RED
			// 	const vec_t max_vec_rgb[3] = {0.61894476, -0.24312686, 0.62345751};
			// 	const vec_t min_vec_rgb[3] = {-0.61894476, 0.24312686, -0.62345751};
			// 	const vec_t max_vec_dkl[3] = {0.5015333, 0.0126745, 0.0271685};
			// 	const vec_t min_vec_dkl[3] = {-0.5015333, -0.0126745, -0.0271685};

				
			const dkl2rgb_t DKL2RGB[3][3] = {
				{10.60864043, 23.50260678, 21.01613594},
				{-3.17452434, -22.53568763, -20.37323115},
				{0.20807273, 154.02866473, 153.78039361}
			};

		void operator()(hls::stream<rgb_t_array> &opt_points_stream, hls::stream<agg_inputs> &din) {
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
			hls::stream<dkl_t_array> dkl_stream;
			read_dkl_loop(dkl_stream, din_stream1);

			// Find inv_square of a, b, c, and compress it to 3 * ele_size width
			hls::stream<inv_square_t_array> inv_square_abc_stream;
			inv_square_abc_loop(inv_square_abc_stream, din_stream2);

			// Stage 2

			hls::stream<dkl_t_array> dkl_stream1, dkl_stream2;
			duplicate_stream<dkl_t_array, 16>(dkl_stream1, dkl_stream2, dkl_stream);

			// Stage 3

			hls::stream<rgb_t_array> rgb_stream;
			dkl2rgb_loop(rgb_stream, dkl_stream1);

			// Stage 4

			#pragma HLS STREAM variable=dkl_stream2 depth=16
			#pragma HLS STREAM variable=inv_square_abc_stream depth=16
			adjust_tile(opt_points_stream, rgb_stream, dkl_stream2, inv_square_abc_stream);

			// Stage 5
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

		void read_dkl_loop(hls::stream<dkl_t_array> &dkl_stream, hls::stream<agg_inputs> &din_stream) {
			agg_inputs din;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				if (i == 0){
					din = din_stream.read();
				}
				dkl_t_array dkl_t_array_i;
				#pragma HLS ARRAY_PARTITION variable=dkl_t_array_i.data dim=0 complete
				dkl_t_array_i.data[0] = din.ds[i];
				dkl_t_array_i.data[1] = din.ks[i];
				dkl_t_array_i.data[2] = din.ls[i];
				dkl_stream.write(dkl_t_array_i);
			}
		}

		void inv_square_abc_loop( hls::stream<inv_square_t_array> &inv_square_abc_stream, hls::stream<agg_inputs> &din_stream){
			agg_inputs din;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				if (i == 0){
					din = din_stream.read();
				}
				inv_square_t_array inv_square_abcs_i;
				#pragma HLS ARRAY_PARTITION variable=inv_square_abcs_i.data dim=0 complete
				inv_square(inv_square_abcs_i.data, din.as[i], din.bs[i], din.cs[i]);
				inv_square_abc_stream.write(inv_square_abcs_i);
			}
		}

		void dkl2rgb_loop(hls::stream<rgb_t_array> &rgb_stream, hls::stream<dkl_t_array> &dkl_stream) {
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				dkl_t_array dkl_centers_i = dkl_stream.read();
				rgb_t_array rgb_centers_i;
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
			inv_square_abcs_i[0] = hls::recip(inv_square_t(a * a));
			inv_square_abcs_i[1] = hls::recip(inv_square_t(b * b));
			inv_square_abcs_i[2] = hls::recip(inv_square_t(c * c));
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

		void adjust_tile( hls::stream<rgb_t_array> &opt_points_stream, hls::stream<rgb_t_array> &rgb_stream, 
									hls::stream<dkl_t_array> &dkl_stream, hls::stream<inv_square_t_array> &inv_square_abc_stream){
			
			#pragma HLS DATAFLOW disable_start_propagation

			// stage 0 

			hls::stream<rgb_t_array> rgb_stream1, rgb_stream2;
			duplicate_stream<rgb_t_array, 16>(rgb_stream1, rgb_stream2, rgb_stream);

			// stage 1

			hls::stream<rgb_not_fixed_t_array> min_p_stream, max_p_stream;
			min_p_max_p_loop(max_p_stream, min_p_stream, rgb_stream1, dkl_stream, inv_square_abc_stream);

			// stage 2

			hls::stream<rgb_not_fixed_t_array> min_p_stream1, min_p_stream2, max_p_stream1, max_p_stream2;
			duplicate_stream<rgb_not_fixed_t_array, 16>(min_p_stream1, min_p_stream2, min_p_stream);
			duplicate_stream<rgb_not_fixed_t_array, 16>(max_p_stream1, max_p_stream2, max_p_stream);

			// stage 3

			hls::stream<rgb_not_fixed_t> min_max_stream, max_min_stream;
			loopMax_16_3(max_min_stream, min_p_stream1);
			loopMin_16_3(min_max_stream, max_p_stream1);

			// stage 4
			
			#pragma HLS STREAM variable=min_p_stream2 depth=128
			#pragma HLS STREAM variable=max_p_stream2 depth=128
			#pragma HLS STREAM variable=rgb_stream2 depth=256
			converge_plane_loop(opt_points_stream, max_min_stream, min_max_stream, min_p_stream2, max_p_stream2, rgb_stream2);

			// stage 5
		}

		void min_p_max_p_loop( hls::stream<rgb_not_fixed_t_array> &max_p_stream,  
								hls::stream<rgb_not_fixed_t_array> &min_p_stream,
								hls::stream<rgb_t_array> &rgb_stream,
								hls::stream<dkl_t_array> &dkl_stream,
								hls::stream<inv_square_t_array> &inv_square_abc_stream)
		{
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				rgb_t_array rgb_centers_i = rgb_stream.read();
				dkl_t_array dkl_centers_i = dkl_stream.read();
				inv_square_t_array inv_square_abcs_i = inv_square_abc_stream.read();
				rgb_not_fixed_t_array min_p_i, max_p_i;
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

		void line_ell_inter( rgb_not_fixed_t inter_points[3],  const dkl_t in_points[3],  
											const vec_t _vec[3], const inv_square_t inv_square_abc_i[3]){
			 #pragma HLS PIPELINE II=1  
			line_ell_inter_sum_t sum = 0;
			dkl_t _inter_points[3];
			#pragma HLS ARRAY_PARTITION variable=_inter_points dim=0 complete
			for (int j = 0; j < 3; j++) {
				sum += _vec[j] * _vec[j] * inv_square_abc_i[j];
			}
			gate(sum, line_ell_inter_sum_t(1e4), line_ell_inter_sum_t(1)); 
			rsqrt_t _t;
			_t = hls::rsqrt<23, 14>(rsqrt_t(sum));
			line_ell_inter_t_t t;
			t = line_ell_inter_t_t(_t) ;
			for (int j = 0; j < 3; j++) {
				_inter_points[j] = in_points[j] + t * _vec[j];
			}
			mm_3x3to3(inter_points, DKL2RGB, _inter_points);
		}

		void fix_bounds(rgb_not_fixed_t in_point[3], const rgb_t rgb_center[3]){
			#pragma HLS PIPELINE II=1    
			correct_bounds<0, color1, true>(in_point, rgb_center);
			correct_bounds<0, color2, true>(in_point, rgb_center);
			correct_bounds<1, color1, false>(in_point, rgb_center);
			correct_bounds<1, color2, false>(in_point, rgb_center);
		}

		template<int bound = 0, int col = 0, bool floor = true>
		void correct_bounds(rgb_not_fixed_t in_point[3], const rgb_t rgb_center[3])
		{
			 #pragma HLS PIPELINE   II=1 
			const auto _bound = rgb_not_fixed_t(bound);
			fix_bound_t_t t;
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

		void converge_plane_loop( hls::stream<rgb_t_array> &opt_points_stream, hls::stream<rgb_not_fixed_t> &max_min_stream, 
											hls::stream<rgb_not_fixed_t> &min_max_stream, 
											hls::stream<rgb_not_fixed_t_array> &min_p_stream, 
											hls::stream<rgb_not_fixed_t_array> &max_p_stream,  
											hls::stream<rgb_t_array> &rgb_centers_stream)
		{
			rgb_not_fixed_t max_min, min_max;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1 rewind
				rgb_not_fixed_t_array min_p_i = min_p_stream.read();
				rgb_not_fixed_t_array max_p_i = max_p_stream.read();
				rgb_t_array rgb_centers_i = rgb_centers_stream.read();
				if (i==0) {
					max_min = max_min_stream.read();
					min_max = min_max_stream.read();
				}
				rgb_not_fixed_t col_plane = (max_min + min_max) / 2;
				gate(col_plane, rgb_not_fixed_t(1), rgb_not_fixed_t(0));
				rgb_t_array opt_points_i;
				#pragma HLS ARRAY_PARTITION variable=opt_points_i.data dim=0 complete
				converge_plane( opt_points_i.data, col_plane, min_p_i.data, max_p_i.data, rgb_centers_i.data);
				opt_points_stream.write(opt_points_i);
			}
		}

		void converge_plane(rgb_t opt_points_i[3], const rgb_not_fixed_t &col_plane, const rgb_not_fixed_t min_p_i[3], 
									const rgb_not_fixed_t max_p_i[3], const rgb_t rgb_centers_i[3]){
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
					opt_points_i[j] = rgb_centers_i[j]  + t * min_vec_rgb[j];
				}
			}
		}

		template<typename T1, typename T2>
		void loopMax_16_3(hls::stream<T1> &max_stream, hls::stream<T2> &in_stream) {
			T1 max;
			for (int i = 0; i < 16; i++) {
				#pragma HLS PIPELINE II=1   rewind
				T2 in = in_stream.read();
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
				#pragma HLS PIPELINE II=1   rewind
				T2 in = in_stream.read();
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
			#pragma HLS PIPELINE   II=1 
			if (in1 < in2) {
				out = in1;
			}
			else {
				out = in2;
			}
		}

		template<typename T>
		void my_max ( T &out, const T in1, const T in2) {
			#pragma HLS PIPELINE   II=1 
			if (in1 > in2) {
				out = in1;
			}
			else {
				out = in2;
			}
		}


	};
}

const ap_uint<8> RGB2sRGB_LUT[256] = {
    0,
    12,
    21,
    28,
    33,
    38,
    42,
    46,
    49,
    52,
    55,
    58,
    61,
    63,
    66,
    68,
    70,
    73,
    75,
    77,
    79,
    81,
    82,
    84,
    86,
    88,
    89,
    91,
    93,
    94,
    96,
    97,
    99,
    100,
    102,
    103,
    104,
    106,
    107,
    109,
    110,
    111,
    112,
    114,
    115,
    116,
    117,
    118,
    120,
    121,
    122,
    123,
    124,
    125,
    126,
    127,
    129,
    130,
    131,
    132,
    133,
    134,
    135,
    136,
    137,
    138,
    139,
    140,
    141,
    142,
    142,
    143,
    144,
    145,
    146,
    147,
    148,
    149,
    150,
    151,
    151,
    152,
    153,
    154,
    155,
    156,
    157,
    157,
    158,
    159,
    160,
    161,
    161,
    162,
    163,
    164,
    165,
    165,
    166,
    167,
    168,
    168,
    169,
    170,
    171,
    171,
    172,
    173,
    174,
    174,
    175,
    176,
    176,
    177,
    178,
    179,
    179,
    180,
    181,
    181,
    182,
    183,
    183,
    184,
    185,
    185,
    186,
    187,
    187,
    188,
    189,
    189,
    190,
    191,
    191,
    192,
    193,
    193,
    194,
    194,
    195,
    196,
    196,
    197,
    197,
    198,
    199,
    199,
    200,
    201,
    201,
    202,
    202,
    203,
    204,
    204,
    205,
    205,
    206,
    206,
    207,
    208,
    208,
    209,
    209,
    210,
    210,
    211,
    212,
    212,
    213,
    213,
    214,
    214,
    215,
    215,
    216,
    217,
    217,
    218,
    218,
    219,
    219,
    220,
    220,
    221,
    221,
    222,
    222,
    223,
    223,
    224,
    224,
    225,
    226,
    226,
    227,
    227,
    228,
    228,
    229,
    229,
    230,
    230,
    231,
    231,
    232,
    232,
    233,
    233,
    234,
    234,
    235,
    235,
    236,
    236,
    237,
    237,
    237,
    238,
    238,
    239,
    239,
    240,
    240,
    241,
    241,
    242,
    242,
    243,
    243,
    244,
    244,
    245,
    245,
    245,
    246,
    246,
    247,
    247,
    248,
    248,
    249,
    249,
    250,
    250,
    251,
    251,
    251,
    252,
    252,
    253,
    253,
    254,
    254,
    254,
};
