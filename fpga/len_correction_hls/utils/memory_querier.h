#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>
#include "../precomputation/precompute_constant.h"
#include "types.h"
#ifndef MEMORY_Q
#define MEMORY_Q // Prevent duplicate definition
namespace vr_prototype
{
	class Memory_querier
	{
        public:
        // void operator() (hls::stream<Memory_query_t> &memory_query_stream, hls::stream<Bilinear_info_t> &bilinear_info_stream, hls::stream<FourPixel_t> &memory_rdata_stream){
        //     #pragma HLS DATAFLOW
        //     hls::stream<Partial_bilinear_info_t> partial_bilinear_info_stream;
        //     #pragma HLS STREAM variable=partial_bilinear_info_stream depth=32
        //     query_generator(memory_query_stream, partial_bilinear_info_stream);
        //     bilinear_info_generator(bilinear_info_stream, partial_bilinear_info_stream, memory_rdata_stream);
        // }
        void query_generator(hls::stream<Memory_query_t> &memory_query_stream, hls::stream<Partial_bilinear_info_t> &partial_bilinear_info_stream){
            for (int i = 0; i < 1080; i++)
			{
				for (int j=0; j < 960; j++)
				{
					#pragma HLS PIPELINE II=1 rewind
                    float x, y, cor_x, cor_y;
                    x = float(j);
                    y = float(i);
                    // compute address using
					compute_correction_idx(cor_x, cor_y, x, y); 
                    bool valid = true;
                    if (cor_x < 0 || cor_x >= 960 || cor_y < 0 || cor_y >= 1080) {
                        valid = false;
                    }
                    bool last_col = (j == 959);
                    send_query_and_partial_bilinear_info(memory_query_stream, partial_bilinear_info_stream, cor_x, cor_y, valid, last_col);



				}
			}
        }
		void compute_correction_idx(float &cor_x, float &cor_y, const float &x, const float &y)
		{
			#pragma HLS INLINE off // need off or synthesize will fail without error
			// HLS Can not detect no-dependency cross functions?
            const float cx = 480;
            const float cy = 540;
            const float c = 1 / 401. * 25.4 / 39.07; // c = 1 / ppi * 25.4 / z ;
            const float k1 = 0.33582564;
			const float k2 = 0.55348791;
            const float rep_c = 1/c;
            
			float x1 = x - cx;
			float y1 = y - cy;
			x1 = x1 * c;
    		y1 = y1 * c;

			float r2 = x1 * x1 + y1 * y1;
			float r4 = r2 * r2;
			float factor = 1 + k1 * r2 + k2 * r4;

			float cor_x1 = x1 * factor;
			float cor_y1 = y1 * factor;
			cor_x1 = cor_x1 * rep_c;
			cor_y1 = cor_y1 * rep_c;
			cor_x = cor_x1 + cx;
			cor_y = cor_y1 + cy;
		}
        void send_query_and_partial_bilinear_info(hls::stream<Memory_query_t> &memory_query_stream, hls::stream<Partial_bilinear_info_t> &partial_bilinear_info_stream, 
                                                    const float &cor_x, const float &cor_y, const bool &valid, const bool &last_col)
        {
            ap_uint<10> x1 = ap_uint<10>(cor_x);
			ap_uint<10> x2 = x1 + 1;
			ap_uint<11> y1 = ap_uint<11>(cor_y);
			ap_uint<11> y2 = y1 + 1;
            Memory_query_t memory_query;
            memory_query.read = valid;
            memory_query.yield = last_col;
            // x1y1, x1y2, x2y1, x2y2
            memory_query.cols[0] = x1;
            memory_query.cols[1] = x1;
            memory_query.cols[2] = x2;
            memory_query.cols[3] = x2;
            memory_query.rows[0] = y1;
            memory_query.rows[1] = y2;
            memory_query.rows[2] = y1;
            memory_query.rows[3] = y2;

            Partial_bilinear_info_t p_blinear_info;
            float dx = cor_x - x1;
			float dy = cor_y - y1;
			p_blinear_info.dx = dx;
			p_blinear_info.dy = dy;
			p_blinear_info.valid = valid;

            memory_query_stream.write(memory_query);
            partial_bilinear_info_stream.write(p_blinear_info);
        }
        void bilinear_info_generator(hls::stream<Bilinear_info_t> &bilinear_info_stream, hls::stream<Partial_bilinear_info_t> &partial_bilinear_info_stream, hls::stream<FourPixel_t> &memory_rdata_stream){
            for (int i = 0; i < 1080; i++)
            {
                for (int j = 0; j < 960; j++)
                {
                    #pragma HLS PIPELINE II=1 rewind
                    Bilinear_info_t bilinear_info;
                    Partial_bilinear_info_t p_blinear_info;
                    p_blinear_info = partial_bilinear_info_stream.read();
                    bilinear_info.dx = p_blinear_info.dx;
                    bilinear_info.dy = p_blinear_info.dy;
                    bilinear_info.valid = p_blinear_info.valid;
                    if (bilinear_info.valid) {
                        FourPixel_t four_pixel;
                        four_pixel = memory_rdata_stream.read();
                        bilinear_info.data = four_pixel;
                    }
                    bilinear_info_stream.write(bilinear_info);
                }
            }
        }
    };
}
#endif