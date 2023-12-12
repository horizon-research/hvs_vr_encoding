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

struct Memory_access {
	Pixel data[4];
	ap_uint<7> address1[4];
	ap_uint<9> address2[4];
	ap_uint<1> yield;
};

struct Memory_info {
	ap_uint<11> image_row_shift;
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
			store_address_generator();
			// Memory Management, deal with lock and yield
			memory_manager();
			// Len correction using Biliner interpolation
			len_correction();
		}

		void store_address_generator(hls::stream<Memory_access> &memory_write_stream, hls::stream<Agg_in_srgb> &din) {
			bool mem_info_sent = false;

			int tile_count = 0;
			send_meminfo(memory_write_stream, 0);
			fill_buffer(memory_write_stream, din, tile_count);

			int tile_start_row = (tile_count / (1920/4)) << 2;
			int tile_start_col = (tile_count % (1920/4)) << 2;
			while(tile_start_row < buffer_row_num) {
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
					decode_tile_momory_write(memory_write_stream, in, tmp_tile_start_row, tmp_tile_start_col);
				}
			}
		}



		void update_buffer(hls::stream<Memory_access> &memory_write_stream, hls::stream<Agg_in_srgb> &din, int &update_num) {
			int tile_start_row = 0;
			int tile_start_col = 0;
			while(tile_start_row < buffer_row_num) {
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
					decode_tile_momory_write(memory_write_stream, in, tmp_tile_start_row, tmp_tile_start_col);
				}
			}
		}

		void send_meminfo(hls::stream<Memory_info> &memory_info_stream, ap_uint<11> image_row_shift) {
			Memory_info mem_info;
			mem_info.image_row_shift = 0;
			memory_info_stream.write(mem_info);
		}

		void tile_write(hls::stream<Memory_access> &memory_write_stream, Agg_in_srgb &in, int tile_start_row, int tile_start_col) {
			Memory_access out[4];
			decode_tile(out, in, tile_start_row, tile_start_col);
			for (int i = 0; i < 4; i++)
			{
				memory_write_stream.write(out[i]);
			}
		}

		void decode_tile ( Memory_access out[4], Agg_in_srgb &in, int tile_start_row, int tile_start_col){
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
				start_col = (start_col >> 1);
				for (int j = 0; j < 4; j++)
				{
					out[i].address1[j] = start_row + addr_shift[i][0];
					out[i].address2[j] = start_col + addr_shift[i][1];
				}
			}
		}

		void rgb_assign(Pixel &out, ap_uint<8> in[3]) {
			out.b = in[2];
			out.g = in[0];
			out.r = in[1];
		}

		void memory_manager(hls::stream<Memory_access> &read, hls::stream<Memory_access> &write) {
			pixel pixel_buffer_1[92][480];
			pixel pixel_buffer_2[92][480];
			pixel pixel_buffer_3[92][480];
			pixel pixel_buffer_4[92][480];
		}

		void len_correction() {

		}
		
	}

}




const ap_uint<3> len_correction_shifts[1080] = {
    {1, 2}, // 第一个元素
    {3, 4}, // 第二个元素
    // ... (剩余元素)
};

