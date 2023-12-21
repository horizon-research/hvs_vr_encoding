#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>


struct Agg_in_srgb {
	ap_uint<8> rgb[16][3];
};

struct Pixel { // Pynq is BGR format
	ap_uint<8> b;
	ap_uint<8> g;
	ap_uint<8> r;
};

struct FourPixel { // Pynq is BGR format
	Pixel data[4];
};

struct Memory_access {
	Pixel data[4];
	ap_uint<7> address1[4];
	ap_uint<9> address2[4];
	ap_uint<1> yield;
};

struct Memory_info {
	ap_uint<11> image_row_shift;
};

struct Bilinear_info {
	ap_uint<2> xy11_idx;
	ap_uint<2> xy12_idx;
	ap_uint<2> xy21_idx;
	ap_uint<2> xy22_idx;
	float dx;
	float dy;
};

namespace vr_prototype
{
	class Len_corrector
	{
		public:
			const int buffer_row_num = 184;
			
		void operator()(hls::stream<Pixel> &dout, hls::stream<Agg_in_srgb> &din) {
			#pragma HLS DATAFLOW disable_start_propagation
			// Read from stream / config, gnerate data to store and its address
			hls::stream<Memory_access> memory_write_stream;
			hls::stream<Memory_info> memory_info_stream;
			store_address_generator(memory_write_stream, memory_info_stream, din);

			// Memory Management, deal with lock and yield
			hls::stream<Memory_access> memory_read_stream;
			hls::stream<FourPixel> read_data_stream;			
			memory_manager(read_data_stream, memory_write_stream, memory_read_stream);

			// Len correction using Biliner interpolation
			hls::stream<Pixel> _dout;
			len_correction(_dout, memory_read_stream, read_data_stream, memory_info_stream);

			// double output
			double_output(dout, _dout);
		}

		void double_output(hls::stream<Pixel> &dout, hls::stream<Pixel> &_dout) {
			Pixel buffer [960];
			for (int i = 0; i < 1080; i++)
			{
				for (int i = 0; i < 960; i++)
				{
					Pixel in = dout.read();
					buffer[i] = in;
					dout.write(in);
				}	
				for (int i = 0; i < 960; i++)
				{
					dout.write(buffer[i]);
				}
			}
		}

		void store_address_generator(hls::stream<Memory_access> &memory_write_stream, hls::stream<Memory_info> &memory_info_stream, hls::stream<Agg_in_srgb> &din) {
			int tile_count = 0;
			for (int i = 0; i < 1080; i++)
			{
				if (i==0) {
					send_meminfo(memory_info_stream, 0);
					update_buffer(memory_write_stream, din, tile_count, buffer_row_num);
				}
				else {
					int tile_start_row = ( tile_count / (1920/4) ) << 2;
					send_meminfo(memory_info_stream, tile_start_row);
					ap_uint<3> shift = len_correction_shifts[i];
					update_buffer(memory_write_stream, din, tile_count, shift);
				}
				Memory_access out;
				out.yield = 1;
				memory_write_stream.write(out);
			}
		}

		void update_buffer(hls::stream<Memory_access> &memory_write_stream, hls::stream<Agg_in_srgb> &din, int &tile_count, const int &update_row_num){
			int tile_start_row = (tile_count / (1920/4)) << 2;
			int tile_start_col = (tile_count % (1920/4)) << 2;
			int target_row_num = tile_start_row + update_row_num;
			while(tile_start_row < target_row_num) {
				int tmp_tile_start_row = tile_start_row;
				int tmp_tile_start_col = tile_start_col;

				tile_count ++;
				tile_start_row = (tile_count / (1920/4)) << 2;
				tile_start_col = (tile_count % (1920/4)) << 2;

				if (tile_start_col >= 960) {
					din.read(); 
				}
				else {
					Agg_in_srgb in = din.read();
					tile_write(memory_write_stream, in, tmp_tile_start_row, tmp_tile_start_col);
				}
			}
		}

		void send_meminfo(hls::stream<Memory_info> &memory_info_stream, ap_uint<11> image_row_shift) {
			Memory_info mem_info;
			mem_info.image_row_shift = 0;
			memory_info_stream.write(mem_info);
		}

		void tile_write(hls::stream<Memory_access> &memory_write_stream, Agg_in_srgb &in, const int &tile_start_row,  const int &tile_start_col) {
			Memory_access out[4];
			decode_tile(out, in, tile_start_row, tile_start_col);
			for (int i = 0; i < 4; i++)
			{
				memory_write_stream.write(out[i]);
			}
		}

		void decode_tile ( Memory_access out[4], Agg_in_srgb &in, const int &tile_start_row,  const int &tile_start_col){
			const ap_uint<4> rgb_shift[4] = {0, 2, 8, 10};
			const ap_uint<4> addr_shift[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
			for (int i = 0; i < 4; i++)
			{
				rgb_assign(out[i].data[0], in.rgb[rgb_shift[i]]);
				rgb_assign(out[i].data[1], in.rgb[rgb_shift[i] + 1]);
				rgb_assign(out[i].data[2], in.rgb[rgb_shift[i] + 4]);
				rgb_assign(out[i].data[3], in.rgb[rgb_shift[i] + 5]);

				ap_uint<7> buffer_start_row = (tile_start_row % buffer_row_num) >> 1;
				ap_uint<9> buffer_start_col = (tile_start_col >> 1);
				for (int j = 0; j < 4; j++)
				{
					out[i].address1[j] = start_row + addr_shift[i][0];
					out[i].address2[j] = start_col + addr_shift[i][1];
				}
				out[i].yield = 0;
			}
		}

		void rgb_assign(Pixel &out, ap_uint<8> in[3]) {
			// rgb to bgr
			out.b = in[2];
			out.g = in[0];
			out.r = in[1];
		}

		void memory_manager(hls::stream<FourPixel> &read_data_stream, hls::stream<Memory_access> &memory_write_stream, hls::stream<Memory_access> &memory_read_stream) {
			Pixel pixel_buffer_1[92][480];
			Pixel pixel_buffer_2[92][480];
			Pixel pixel_buffer_3[92][480];
			Pixel pixel_buffer_4[92][480];
			ap_uint<1> ownership = 0; // 0: write 1: read
			ap_uint<11> line_counter = 0;
			while (line_counter < 1080) {
				if(ownership == 0) {
					Memory_access in = memory_write_stream.read();
					if (in.yield == 1) {
						ownership = 1;
					}
					else {
						write_to_buffer(in, pixel_buffer_1, pixel_buffer_2, pixel_buffer_3, pixel_buffer_4);
					}
				}
				else {
					Memory_access in = memory_read_stream.read();
					if (in.yield == 1) {
						ownership = 0;
						line_counter ++;
					}
					else {
						read_from_buffer(read_data_stream, in, pixel_buffer_1, pixel_buffer_2, pixel_buffer_3, pixel_buffer_4);
					}
				}
			}
		}

		void write_to_buffer(Memory_access &in, Pixel pixel_buffer_1[92][480], Pixel pixel_buffer_2[92][480], Pixel pixel_buffer_3[92][480], Pixel pixel_buffer_4[92][480]) {
			pixel_buffer_1[in.address1[0]][in.address2[0]] = in.data[0];
			pixel_buffer_2[in.address1[1]][in.address2[1]] = in.data[1];
			pixel_buffer_3[in.address1[2]][in.address2[2]] = in.data[2];
			pixel_buffer_4[in.address1[3]][in.address2[3]] = in.data[3];
		}

		void read_from_buffer(hls::stream<FourPixel> &read_data_stream, Memory_access &in, Pixel pixel_buffer_1[92][480], Pixel pixel_buffer_2[92][480], Pixel pixel_buffer_3[92][480], Pixel pixel_buffer_4[92][480]){
			FourPixel out;
			out.data[0] = pixel_buffer_1[in.address1[0]][in.address2[0]];
			out.data[1] = pixel_buffer_2[in.address1[1]][in.address2[1]];
			out.data[2] = pixel_buffer_3[in.address1[2]][in.address2[2]];
			out.data[3] = pixel_buffer_4[in.address1[3]][in.address2[3]];
			read_data_stream.write(out);
		}

		void len_correction( hls::stream<Pixel> &dout, hls::stream<Memory_access> &memory_read_stream, hls::stream<FourPixel> &read_data_stream, hls::stream<Memory_info> &memory_info_stream) {
			#pragma HLS DATAFLOW disable_start_propagation
			// Read memory
			hls::stream<Bilinear_info> bilinear_info_stream; 
			address_query(memory_read_stream, bilinear_info_stream, memory_info_stream);
			// Bilinear interpolation
			bilinear_interpolation_node(dout, read_data_stream, bilinear_info_stream);
		}

		void address_query(hls::stream<Memory_access> &memory_read_stream, hls::stream<Bilinear_info> &bilinear_info_stream, hls::stream<Memory_info> &memory_info_stream) {
			for (int i = 0; i < 1080; i++)
			{
				Memory_info mem_info = memory_info_stream.read();
				ap_uint<11> image_row_shift = mem_info.image_row_shift;
				for (int j=0; j < 960; j++)
				{
					// compute address using
					float x, y, cor_x, cor_y;
					compute_correction_idx(cor_x, cor_y, x, y);
					send_read_query(memory_read_stream, bilinear_info_stream, cor_x, cor_y, image_row_shift);
				}
			}
		}

		void compute_correction_idx(float &cor_x, float &cor_y, float &x, float &y)
		{
			x = x - 480;
			y = y - 540;
			const float c = 1 / 401. * 25.4 / 39.07; // c = 1 / ppi * 25.4 / z ;
			x = x * c;
    		y = y * c;

			float r2 = x**2 + y**2;
			float r4 = r2**2;
			const float k1 = 0.33582564;
			const float k2 = 0.55348791;
			float factor = 1 + k1 * r2 + k2 * r4;

			cor_x = x * factor;
			cor_y = y * factor;

			const float rep_c = 1/c;
			cor_x = cor_x  * rep_c;
			cor_y = cor_y  * rep_c;
			cor_x = cor_x + 480;
			cor_y = cor_y + 540;
		}

		void send_read_query(hls::stream<Memory_access> &memory_read_stream, hls::stream<Bilinear_info> &bilinear_info_stream, const float &cor_x,  const float &cor_y, ap_uint<11> &image_row_shift) {
			ap_uint<11> x1 = ap_uint<11>(cor_x);
			ap_uint<11> x2 = x1 + 1;
			ap_uint<11> y1 = ap_uint<11>(cor_y);
			ap_uint<11> y2 = y1 + 1;
			float dx = cor_x - x1;
			float dy = cor_y - y1;

			Memory_access out;
			Bilinear_info blinear_info;
			blinear_info.dx = dx;
			blinear_info.dy = dy;


			ap_uint<2> xy11_idx, xy12_idx, xy21_idx, xy22_idx;
			if (x1(0,0) == 0 & y1(0,0) == 0) { // if x1 and y1 are even
				xy11_idx = 0;
				xy12_idx = 1;
				xy21_idx = 2;
				xy22_idx = 3;
			}
			else if (x1(0,0) == 0 & y1(0,0) == 1) { // if x1 is even and y1 is odd
				xy11_idx = 3;
				xy12_idx = 4;
				xy21_idx = 1;
				xy22_idx = 2;
			}
			else if (x1(0,0) == 1 & y1(0,0) == 0) {
				xy11_idx = 2;
				xy12_idx = 1;
				xy21_idx = 4;
				xy22_idx = 3;
			}
			else if (x1(0,0) == 1 & y1(0,0) == 1) {
				xy11_idx = 4;
				xy12_idx = 3;
				xy21_idx = 1;
				xy22_idx = 2;
			}

			Memory_access out;
			out.address1[xy11_idx] = y1;
			out.address2[xy11_idx] = x1;
			out.address1[xy12_idx] = y1;
			out.address2[xy12_idx] = x2;
			out.address1[xy21_idx] = y2;
			out.address2[xy21_idx] = x1;
			out.address1[xy22_idx] = y2;
			out.address2[xy22_idx] = x2;
			out.yield = 0;

			blinear_info.xy11_idx = xy11_idx;
			blinear_info.xy12_idx = xy12_idx;
			blinear_info.xy21_idx = xy21_idx;
			blinear_info.xy22_idx = xy22_idx;

			bilinear_info_stream.write(blinear_info);
			memory_read_stream.write(out);
		}

		void bilinear_interpolation_node(hls::stream<Pixel> &dout, hls::stream<FourPixel> &read_data_stream, hls::stream<Bilinear_info> &bilinear_info_stream) {
			for (int i = 0; i < 1080; i++)
			{
				Bilinear_info blinear_info = bilinear_info_stream.read();
				FourPixel in = read_data_stream.read();
				Pixel xy11 = in.data[blinear_info.xy11_idx];
				Pixel xy12 = in.data[blinear_info.xy12_idx];
				Pixel xy21 = in.data[blinear_info.xy21_idx];
				Pixel xy22 = in.data[blinear_info.xy22_idx];
				float dx = blinear_info.dx;
				float dy = blinear_info.dy;
				Pixel out;
				out.b = bilinear_interpolation(xy11.b, xy12.b, xy21.b, xy22.b, dx, dy);
				out.g = bilinear_interpolation(xy11.g, xy12.g, xy21.g, xy22.g, dx, dy);
				out.r = bilinear_interpolation(xy11.r, xy12.r, xy21.r, xy22.r, dx, dy);
				dout.write(out);
			}
		}

		ap_uint<8> bilinear_interpolation(const ap_uint<8> &xy11, const ap_uint<8> & xy12, const ap_uint<8> & xy21, const ap_uint<8> &xy22, const float &dx, const float &dy) {
			float xy11_f = float(xy11);
			float xy12_f = float(xy12);
			float xy21_f = float(xy21);
			float xy22_f = float(xy22);
			float out = xy11_f * (1 - dx) * (1 - dy) + xy12_f * dy * (1 - dx) + xy21_f * (1 - dy) * dx + xy22_f * dx * dy;
			if (out > 255) {
				out = 255;
			}
			else if (out < 0) {
				out = 0;
			}
			return ap_uint<8> ( ap_ufixed<8, 8, AP_RND>(out) );
		}
		
	}

}




const ap_uint<3> len_correction_shifts[1080] = {};

