#include "types_constants.h"


namespace vr_prototype
{
    class Memory_writer
	{
        public:
        
            void operator()(hls::stream<Memory_access_data> &memory_write_ds, hls::stream<Memory_access_ctrl> &memory_write_cs, hls::stream<Agg_in_srgb> &din) {
                hls::stream<ap_uint<8>> shifts;
                hls::stream<ap_uint<11>> bases;
                row_configer(shifts, bases);
                memory_writer(memory_write_ds, memory_write_cs, din, shifts, bases);
            }
            void memory_writer(hls::stream<Memory_access> &memory_write_ds, hls::stream<Memory_access_ctrl> &memory_write_cs, hls::stream<Agg_in_srgb> &din, hls::stream<ap_uint<8>> &shifts, hls::stream<ap_uint<11>> &bases) {
                ap_uint<2> writer_instr;
                for ( int i = 0; i < 1127; i++) { // yield count / write count
                    for (int j = 0; j < 1920; j+=4) {
                        if (j == 0) {
                            writer_instr = writer_instrs[i];
                        }
                        write = writer_instr.range(1, 1);
                        yield = writer_instr.range(0, 0);
                        #pragma HLS PIPELINE II=1 rewind
                        if (j >= 960) {
                            din.read(); 
                        }
                        else {
                            Agg_in_srgb in = din.read();
                            Memory_access_ctrl ctrls[4];
                            Memory_access_data datas[4];
                            decode_tile_data(datas, in);
                            decode_tile_ctrl(ctrls, in, i, j);
                            for (int k = 0; k < 4; k++) {
                                memory_write_ds.write(datas[k]);
                                memory_write_cs.write(ctrls[k]);
                            }

                        }         
                    }      
                }
            }
            void decode_tile_data(Memory_access_data datas[4], Agg_in_srgb &in) {
                const ap_uint<4> rgb_shift[4] = {0, 2, 8, 10};
                for (int i = 0; i < 4; i++)
                {
                    rgb_assign(datas[i].data[0], in.rgb[rgb_shift[i]]);
                    rgb_assign(datas[i].data[1], in.rgb[rgb_shift[i] + 1]);
                    rgb_assign(datas[i].data[2], in.rgb[rgb_shift[i] + 4]);
                    rgb_assign(datas[i].data[3], in.rgb[rgb_shift[i] + 5]);
                }
            }
            void decode_tile_ctrl(Memory_access_ctrl ctrls[4], Agg_in_srgb &in) {
                const ap_uint<4> rgb_shift[4] = {0, 2, 8, 10};
                const ap_uint<4> addr_shift[4][2] = {{0, 0}, {0, 2}, {2, 0}, {2, 2}};
                for (int i = 0; i < 4; i++)
                {
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
    };


}