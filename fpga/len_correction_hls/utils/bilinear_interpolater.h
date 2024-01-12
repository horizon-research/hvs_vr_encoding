#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>
#include "../precomputation/precompute_constant.h"
#include "types.h"
#ifndef BILINEAR_INTERPOLATER_H
#define BILINEAR_INTERPOLATER_H // Prevent duplicate definition
namespace vr_prototype
{
	namespace bilinear_interpolater
	{       
            void bilinear_interpolation(ap_uint<8> &out, const ap_uint<8> &xy11, const ap_uint<8> & xy12, const ap_uint<8> & xy21, const ap_uint<8> &xy22, const float &dx, const float &dy);
            void bilinear_interpolater (hls::stream<Pixel_t> &dout, hls::stream<Bilinear_info_t> &bilinear_info_stream){
                for (int i = 0; i < 1080; i++)
                {
                    for (int j = 0; j < 960; j++)
                    {
                        #pragma HLS PIPELINE II=1 rewind
                        Bilinear_info_t blinear_info = bilinear_info_stream.read();
                        #pragma HLS ARRAY_PARTITION variable=blinear_info.data.data complete
                        Pixel_t xy11 = blinear_info.data.data[0];
                        Pixel_t xy12 = blinear_info.data.data[1];
                        Pixel_t xy21 = blinear_info.data.data[2];
                        Pixel_t xy22 = blinear_info.data.data[3];
                        float dx = blinear_info.dx;
                        float dy = blinear_info.dy;
                        Pixel_t out; //write codes closer to where it is used?
                        // How to partition and unroll struct ?
                        bilinear_interpolation(out.b, xy11.b, xy12.b, xy21.b, xy22.b, dx, dy);
                        bilinear_interpolation(out.g, xy11.g, xy12.g, xy21.g, xy22.g, dx, dy);
                        bilinear_interpolation(out.r, xy11.r, xy12.r, xy21.r, xy22.r, dx, dy);
                        if (blinear_info.valid) {
                            dout.write(out);
                        }
                        else {
                            out.b = 0;
                            out.g = 0;
                            out.r = 0;
                            dout.write(out);
                        }
                    }
                }
            }
            void bilinear_interpolation(ap_uint<8> &out, const ap_uint<8> &xy11, const ap_uint<8> & xy12, const ap_uint<8> & xy21, const ap_uint<8> &xy22, const float &dx, const float &dy) {
			    #pragma HLS INLINE // solve the timing issue
                float xy11_f = float(xy11);
                float xy12_f = float(xy12);
                float xy21_f = float(xy21);
                float xy22_f = float(xy22);
                float _out = xy11_f * (1 - dx) * (1 - dy) + xy12_f * dy * (1 - dx) + xy21_f * (1 - dy) * dx + xy22_f * dx * dy;

                if (_out > 255) {
                    _out = 255;
                }
                else if (_out < 0) {
                    _out = 0;
                }
                out = ap_uint<8> ( ap_ufixed<8, 8, AP_RND>(_out) );
		    }
    };
}
#endif