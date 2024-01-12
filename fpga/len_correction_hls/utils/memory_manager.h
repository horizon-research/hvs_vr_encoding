#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>
#include "../precomputation/precompute_constant.h"
#include "types.h"
#ifndef MEMORY_M
#define MEMORY_M // Prevent duplicate definition
namespace vr_prototype
{
	namespace memory_manager
	{       void img_cood_to_buffer_cood(ap_uint<2> &buffer_idx, ap_uint<7> &buffer_row, ap_uint<9> &buffer_col, const ap_uint<11> &image_row, const ap_uint<10> &image_col);
            void memory_manager(hls::stream<FourPixel_t> &memory_read_stream, hls::stream<Memory_write_t>  &memory_write_stream, hls::stream<Memory_query_t> &memory_query_stream) {
                Pixel_t pixel_buffers0[ per_buffer_row_size ][480];
                Pixel_t pixel_buffers1[ per_buffer_row_size ][480];
                Pixel_t pixel_buffers2[ per_buffer_row_size ][480];
                Pixel_t pixel_buffers3[ per_buffer_row_size ][480];

                #pragma HLS AGGREGATE compact=bit variable=pixel_buffers0// need to aggregate to prevent 3x resource usage, it will use ram in the way
                                                                         // 24xd  instead 8xd 8xd 8xd (3x memory usage)
                #pragma HLS AGGREGATE compact=bit variable=pixel_buffers1
                #pragma HLS AGGREGATE compact=bit variable=pixel_buffers2
                #pragma HLS AGGREGATE compact=bit variable=pixel_buffers3
                #pragma HLS bind_storage variable=pixel_buffers0 type=ram_1p impl=uram
                #pragma HLS bind_storage variable=pixel_buffers1 type=ram_1p impl=uram
                #pragma HLS bind_storage variable=pixel_buffers2 type=ram_1p impl=uram
                #pragma HLS bind_storage variable=pixel_buffers3 type=ram_1p impl=uram

                int writer_yield_num = 0;
                int outputed_rows = 0;
                while(outputed_rows < 1080){
                    #pragma HLS pipeline II=1
                    if (writer_yield_num == 0) {
                        Memory_write_t memory_write;
                        memory_write = memory_write_stream.read();
                        writer_yield_num += memory_write.yield_num;
                        ap_uint<2> buffer_idx;
                        ap_uint<7> buffer_row;
                        ap_uint<9> buffer_col;
                        img_cood_to_buffer_cood(buffer_idx, buffer_row, buffer_col, memory_write.rows, memory_write.cols);
                        if (buffer_idx == 0) {
                            pixel_buffers0[buffer_row][buffer_col] = memory_write.data;
                        }
                        else if (buffer_idx == 1) {
                            pixel_buffers1[buffer_row][buffer_col] = memory_write.data;
                        }
                        else if (buffer_idx == 2) {
                            pixel_buffers2[buffer_row][buffer_col] = memory_write.data;
                        }
                        else if (buffer_idx == 3) {
                            pixel_buffers3[buffer_row][buffer_col] = memory_write.data;
                        }
                    }
                    else {
                        Memory_query_t memory_query;
                        memory_query = memory_query_stream.read();
                        if (memory_query.yield == 1) {
                            outputed_rows ++;
                            writer_yield_num--;
                        }
                        if (memory_query.read == 1) {
                            FourPixel_t four_pixel;
                            #pragma HLS ARRAY_PARTITION variable=four_pixel.data complete
                            for (int i = 0; i < 4; i++) {
                                #pragma HLS unroll
                                ap_uint<2> buffer_idx;
                                ap_uint<7> buffer_row;
                                ap_uint<9> buffer_col;
                                img_cood_to_buffer_cood(buffer_idx, buffer_row, buffer_col, memory_query.rows[i], memory_query.cols[i]);
                                if (buffer_idx == 0) {
                                    four_pixel.data[i] = pixel_buffers0[buffer_row][buffer_col];
                                }
                                else if (buffer_idx == 1) {
                                    four_pixel.data[i] = pixel_buffers1[buffer_row][buffer_col];
                                }
                                else if (buffer_idx == 2) {
                                    four_pixel.data[i] = pixel_buffers2[buffer_row][buffer_col];
                                }
                                else if (buffer_idx == 3) {
                                    four_pixel.data[i] = pixel_buffers3[buffer_row][buffer_col];
                                }
                            }
                            memory_read_stream.write(four_pixel); 
                        }
                    }
                }
            }

            void img_cood_to_buffer_cood(ap_uint<2> &buffer_idx, ap_uint<7> &buffer_row, ap_uint<9> &buffer_col, const ap_uint<11> &image_row, const ap_uint<10> &image_col) {
                ap_uint<10> cyclic_col = image_col;
                ap_uint<8>  cyclic_row = image_row % buffer_row_num;
                // since we have 4 buffers, we need to map the cyclic_row / cyclic_col to 4 buffers' address
                buffer_col = cyclic_col >> 1;
                buffer_row = cyclic_row >> 1;
                // use case
                if (cyclic_row[0] == 0 && cyclic_col[0] == 0) {
                    buffer_idx = 0;
                }
                else if (cyclic_row[0] == 0 && cyclic_col[0] == 1) {
                    buffer_idx = 1;
                }
                else if (cyclic_col[0] == 0 && cyclic_row[0] == 1) {
                    buffer_idx = 2;
                }
                else if (cyclic_col[0] == 1 && cyclic_row[0] == 1) {
                    buffer_idx = 3;
                }

            }
    }
}

#endif