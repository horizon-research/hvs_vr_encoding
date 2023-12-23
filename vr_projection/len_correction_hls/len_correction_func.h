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
	ap_uint<1> foo; // no use
};

struct Bilinear_info {
	ap_uint<2> xy11_idx;
	ap_uint<2> xy12_idx;
	ap_uint<2> xy21_idx;
	ap_uint<2> xy22_idx;
	float dx;
	float dy;
	bool valid;
};

void len_correction_func(
	hls::stream< Pixel > &dout,
	hls::stream< Agg_in_srgb > &din
	);

namespace vr_prototype
{
	const ap_uint<3> len_correction_shifts[1080] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 4, 0, 4, 0, 4, 4, 0, 4, 0, 4, 4, 0, 4, 0, 4, 0, 4, 0, 4, 4, 0, 4, 0, 4, 0, 4, 0, 4, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 4, 0, 4, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	class Len_corrector
	{
		public:
			const ap_uint<8> buffer_row_num = 188;
			
		void operator()(hls::stream<Pixel> &dout, hls::stream<Agg_in_srgb> &din) {
			#pragma HLS DATAFLOW disable_start_propagation
			
			// Read from stream / config, gnerate data to store and its address
			hls::stream<Memory_access> memory_write_stream("memory_write_stream");
			hls::stream<Memory_info> memory_info_stream("memory_info_stream");
			store_address_generator(memory_write_stream, memory_info_stream, din);

			// Memory Management, deal with lock and yield
			hls::stream<Memory_access> memory_read_stream("memory_read_stream");
			hls::stream<FourPixel> read_data_stream("read_data_stream");			
			hls::stream<Pixel> _dout;
			hls::stream<Bilinear_info> bilinear_info_stream; 
			#pragma HLS STREAM variable=bilinear_info_stream depth=32
			#pragma HLS STREAM variable=memory_read_stream depth=32
			// Len correction using Biliner interpolation, need to sequentialize address_query -> memory_manager -> bilinear_interpolation_node to enable Csim
			address_query(memory_read_stream, bilinear_info_stream, memory_info_stream);

			memory_manager(read_data_stream, memory_write_stream, memory_read_stream);

			bilinear_interpolation_node(_dout, read_data_stream, bilinear_info_stream);

			// double output
			double_output(dout, _dout);
		}

		void double_output(hls::stream<Pixel> &dout, hls::stream<Pixel> &_dout) {
			Pixel buffer [960];
			for (int i = 0; i < 1080; i++)
			{
				for (int j = 0; j < 1920; j++)
				{
					#pragma HLS PIPELINE II=1 rewind
					if (j < 960) {
						Pixel in = _dout.read();
						buffer[j] = in;
						dout.write(in);
					}
					else {
						dout.write(buffer[j-960]);
					}
				}	
			}
		}

		void store_address_generator(hls::stream<Memory_access> &memory_write_stream, hls::stream<Memory_info> &memory_info_stream, hls::stream<Agg_in_srgb> &din) {
			int tile_count = 0;
			for (int i = 0; i < 1080; i++)
			{
				if (i==0) {
					Memory_info mem_info;
					memory_info_stream.write(mem_info);
					update_buffer(memory_write_stream, din, tile_count, buffer_row_num);
				}
				else {
					Memory_info mem_info;
					memory_info_stream.write(mem_info);
					ap_uint<3> shift = len_correction_shifts[i];
					update_buffer(memory_write_stream, din, tile_count, ap_uint<8> (shift) );
				}
				Memory_access out;
				out.yield = 1;
				memory_write_stream.write(out);
			}
		}

		void update_buffer(hls::stream<Memory_access> &memory_write_stream, hls::stream<Agg_in_srgb> &din, int &tile_count, const ap_uint<8>  &update_row_num){
			int tile_start_row = (tile_count / (1920/4)) << 2;
			int tile_start_col = (tile_count % (1920/4)) << 2;
			int target_row_num = tile_start_row + update_row_num;
			while(tile_start_row < target_row_num) {
				#pragma HLS PIPELINE II=4
				int tmp_tile_start_row = tile_start_row;
				int tmp_tile_start_col = tile_start_col;

				tile_count ++;
				tile_start_row = (tile_count / (1920/4)) << 2;
				tile_start_col = (tile_count % (1920/4)) << 2;

				if (tmp_tile_start_col >= 960) {
					din.read(); 
				}
				else {
					Agg_in_srgb in = din.read();
					tile_write(memory_write_stream, in, tmp_tile_start_row, tmp_tile_start_col);
				}
			}
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
			const ap_uint<4> addr_shift[4][2] = {{0, 0}, {0, 2}, {2, 0}, {2, 2}};
			int start_row = tile_start_row;
			int start_col = tile_start_col;

			for (int i = 0; i < 4; i++)
			{
				rgb_assign(out[i].data[0], in.rgb[rgb_shift[i]]);
				rgb_assign(out[i].data[1], in.rgb[rgb_shift[i] + 1]);
				rgb_assign(out[i].data[2], in.rgb[rgb_shift[i] + 4]);
				rgb_assign(out[i].data[3], in.rgb[rgb_shift[i] + 5]);

				ap_uint<2> buffer_idx;
				int row = start_row + addr_shift[i][0];
				int col = start_col + addr_shift[i][1]; //207, 195
				img_cood_to_buffer_cood(buffer_idx, out[i].address1[0], out[i].address2[0], row, col);
				img_cood_to_buffer_cood(buffer_idx, out[i].address1[1], out[i].address2[1], row, col + 1);
				img_cood_to_buffer_cood(buffer_idx, out[i].address1[2], out[i].address2[2], row + 1, col);
				img_cood_to_buffer_cood(buffer_idx, out[i].address1[3], out[i].address2[3], row + 1, col + 1);				
				out[i].yield = 0;
			}
		}

		void rgb_assign(Pixel &out, ap_uint<8> in[3]) {
			// rgb to bgr
			out.b = in[2];
			out.g = in[1];
			out.r = in[0];
		}

		void img_cood_to_buffer_cood(ap_uint<2> &buffer_idx, ap_uint<7> &buffer_row, ap_uint<9> &buffer_col, const ap_uint<11> &image_row, const ap_uint<10> &image_col) {
			// in-buffer col address don't need to be changed
			ap_uint<10> in_buffer_col = image_col;
			// in-buffer row address need to be changed, by mapping it to 0~(B-1) range
			ap_uint<8>  in_buffer_row = image_row % buffer_row_num;
			// Now, we need to map this in_buffer address to certain buffer and itts addr
			buffer_col = in_buffer_col >> 1;
			buffer_row = in_buffer_row >> 1;
			if (in_buffer_col.range(0,0) == 0 && in_buffer_row.range(0,0) == 0)
			{
				buffer_idx = 0;
			}
			else if(in_buffer_col.range(0,0) == 1 && in_buffer_row.range(0,0) == 0)
			{
				buffer_idx = 1;
			}
			else if(in_buffer_col.range(0,0) == 0 && in_buffer_row.range(0,0) == 1)
			{
				buffer_idx = 2;
			}
			else if(in_buffer_col.range(0,0) == 1 && in_buffer_row.range(0,0) == 1)
			{
				buffer_idx = 3;
			}
		}

		void memory_manager(hls::stream<FourPixel> &read_data_stream, hls::stream<Memory_access> &memory_write_stream, hls::stream<Memory_access> &memory_read_stream) {
			Pixel pixel_buffer_1[94][480];
			Pixel pixel_buffer_2[94][480];
			Pixel pixel_buffer_3[94][480];
			Pixel pixel_buffer_4[94][480];
			// #pragma HLS ARRAY_PARTITION variable=pixel_buffer_1 complete dim=2
			// #pragma HLS ARRAY_PARTITION variable=pixel_buffer_2 complete dim=2
			// #pragma HLS ARRAY_PARTITION variable=pixel_buffer_3 complete dim=2
			// #pragma HLS ARRAY_PARTITION variable=pixel_buffer_4 complete dim=2
			ap_uint<1> ownership = 0; // 0: write 1: read
			ap_uint<11> line_counter = 0;
			while (line_counter < 1080) {
				#pragma HLS PIPELINE II=1
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
			// std::cout addresses
			// std::cout << "address1: " << in.address1[0] << " " << in.address1[1] << " " << in.address1[2] << " " << in.address1[3] << std::endl;
			// std::cout << "address2: " << in.address2[0] << " " << in.address2[1] << " " << in.address2[2] << " " << in.address2[3] << std::endl;
			pixel_buffer_1[in.address1[0]][in.address2[0]] = in.data[0];
			pixel_buffer_2[in.address1[1]][in.address2[1]] = in.data[1];
			pixel_buffer_3[in.address1[2]][in.address2[2]] = in.data[2];
			pixel_buffer_4[in.address1[3]][in.address2[3]] = in.data[3];
		}

		void read_from_buffer(hls::stream<FourPixel> &read_data_stream, Memory_access &in, Pixel pixel_buffer_1[92][480], Pixel pixel_buffer_2[92][480], Pixel pixel_buffer_3[92][480], Pixel pixel_buffer_4[92][480]){
			FourPixel out;
			// std::cout << "address1: " << in.address1[0] << " " << in.address1[1] << " " << in.address1[2] << " " << in.address1[3] << std::endl;
			// std::cout << "address2: " << in.address2[0] << " " << in.address2[1] << " " << in.address2[2] << " " << in.address2[3] << std::endl;
			out.data[0] = pixel_buffer_1[in.address1[0]][in.address2[0]];
			out.data[1] = pixel_buffer_2[in.address1[1]][in.address2[1]];
			out.data[2] = pixel_buffer_3[in.address1[2]][in.address2[2]];
			out.data[3] = pixel_buffer_4[in.address1[3]][in.address2[3]];
			read_data_stream.write(out);
		}

		void address_query(hls::stream<Memory_access> &memory_read_stream, hls::stream<Bilinear_info> &bilinear_info_stream, hls::stream<Memory_info> &memory_info_stream) {
			for (int i = 0; i < 1080; i++)
			{
				for (int j=0; j < 961; j++)
				{
					#pragma HLS PIPELINE II=1
					if (j == 960) {
						Memory_access out;
						out.yield = 1;
						memory_read_stream.write(out);
					}
					else {
						float x, y, cor_x, cor_y;
						x = float(j);
						y = float(i);

						if(j == 0) {
							memory_info_stream.read();
						}
						// compute address using
						compute_correction_idx(cor_x, cor_y, x, y); // correct!
						bool valid = true;
						if (cor_x < 0 || cor_x >= 960 || cor_y < 0 || cor_y >= 1080) {
							valid = false;
						}
						send_read_query(memory_read_stream, bilinear_info_stream, cor_x, cor_y, valid);
					}
				}
			}
		}

		void compute_correction_idx(float &cor_x, float &cor_y, const float &x, const float &y)
		{
			#pragma HLS INLINE off // need off or synthesize will fail without error
			// HLS Can not detect no-dependency cross functions?
			float x1 = x - 480;
			float y1 = y - 540;
			const float c = 1 / 401. * 25.4 / 39.07; // c = 1 / ppi * 25.4 / z ;
			x1 = x1 * c;
    		y1 = y1 * c;

			float r2 = x1 * x1 + y1 * y1;
			float r4 = r2 * r2;
			const float k1 = 0.33582564;
			const float k2 = 0.55348791;
			float factor = 1 + k1 * r2 + k2 * r4;

			float cor_x1 = x1 * factor;
			float cor_y1 = y1 * factor;

			const float rep_c = 1/c;
			cor_x1 = cor_x1 * rep_c;
			cor_y1 = cor_y1 * rep_c;
			cor_x = cor_x1 + 480;
			cor_y = cor_y1 + 540;
		}

		void send_read_query(hls::stream<Memory_access> &memory_read_stream, hls::stream<Bilinear_info> &bilinear_info_stream, const float &cor_x,  const float &cor_y, const bool &valid) {
			#pragma HLS INLINE

			// std::cout << "=============== valid: ================" << valid << std::endl;
			// std::cout << "cor_x: " << cor_x << " cor_y: " << cor_y << std::endl;
			ap_uint<10> x1 = ap_uint<10>(cor_x);
			ap_uint<10> x2 = x1 + 1;
			ap_uint<11> y1 = ap_uint<11>(cor_y);
			ap_uint<11> y2 = y1 + 1;

			float dx = cor_x - x1;
			float dy = cor_y - y1;
			// std::cout << "dx: " << dx << " dy: " << dy << std::endl;

			Bilinear_info blinear_info;
			blinear_info.dx = dx;
			blinear_info.dy = dy;
			blinear_info.valid = valid;

			Memory_access out;
			#pragma HLS ARRAY_PARTITION variable=out.data complete dim=0
			#pragma HLS ARRAY_PARTITION variable=out.address1 complete dim=0
			#pragma HLS ARRAY_PARTITION variable=out.address2 complete dim=0

			ap_uint<2> buffer_idx;
			ap_uint<7> buffer_row;
			ap_uint<9> buffer_col;
			img_cood_to_buffer_cood(buffer_idx, buffer_row, buffer_col, y1, x1);
			out.address1[buffer_idx] = buffer_row;
			out.address2[buffer_idx] = buffer_col;
			blinear_info.xy11_idx = buffer_idx;
			img_cood_to_buffer_cood(buffer_idx, buffer_row, buffer_col, y1, x2);
			out.address1[buffer_idx] = buffer_row;
			out.address2[buffer_idx] = buffer_col;
			blinear_info.xy21_idx = buffer_idx;
			img_cood_to_buffer_cood(buffer_idx, buffer_row, buffer_col, y2, x1);
			out.address1[buffer_idx] = buffer_row;
			out.address2[buffer_idx] = buffer_col;
			blinear_info.xy12_idx = buffer_idx;
			img_cood_to_buffer_cood(buffer_idx, buffer_row, buffer_col, y2, x2);
			out.address1[buffer_idx] = buffer_row;
			out.address2[buffer_idx] = buffer_col;
			blinear_info.xy22_idx = buffer_idx;

			out.yield = 0;

			bilinear_info_stream.write(blinear_info);
			if (valid) {
				memory_read_stream.write(out);
			}
		}

		void bilinear_interpolation_node(hls::stream<Pixel> &dout, hls::stream<FourPixel> &read_data_stream, hls::stream<Bilinear_info> &bilinear_info_stream) {
			for (int i = 0; i < 1080; i++)
			{
				for (int j = 0; j < 960; j++)
				{
					#pragma HLS PIPELINE II=1
					Bilinear_info blinear_info = bilinear_info_stream.read();
					// print valid / in_data / out_data
					// std::cout << "blinear_info.valid: " << blinear_info.valid << std::endl;
					if (blinear_info.valid) {
						FourPixel in = read_data_stream.read();
						Pixel xy11 = in.data[blinear_info.xy11_idx];
						Pixel xy12 = in.data[blinear_info.xy12_idx];
						Pixel xy21 = in.data[blinear_info.xy21_idx];
						Pixel xy22 = in.data[blinear_info.xy22_idx];
						float dx = blinear_info.dx;
						float dy = blinear_info.dy;
						Pixel out; //write codes closer to where it is used?
						// How to partition and unroll struct ?
						bilinear_interpolation(out.b, xy11.b, xy12.b, xy21.b, xy22.b, dx, dy);
						bilinear_interpolation(out.g, xy11.g, xy12.g, xy21.g, xy22.g, dx, dy);
						bilinear_interpolation(out.r, xy11.r, xy12.r, xy21.r, xy22.r, dx, dy);
						dout.write(out);
					}
					else {
						Pixel out;
						out.b = 0;
						out.g = 0;
						out.r = 0;
						dout.write(out);
						// std::cout << "out: " << out.b << " " << out.g << " " << out.r << std::endl;
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


