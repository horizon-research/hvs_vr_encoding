#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>
#include "precomputation/precompute_constant.h"
#include "utils/types.h"
#include "utils/memory_writer.h"
#include "utils/memory_manager.h"
#include "utils/memory_querier.h"
#include "utils/bilinear_interpolater.h"

#ifndef LEN_CORRECTION_H
#define LEN_CORRECTION_H // Prevent duplicate definition

namespace vr_prototype
{
	namespace lens_corrector
	{
			void lens_corrector(hls::stream<Pixel_t> &dout, hls::stream<Pixel_t> &din) {
				#pragma HLS DATAFLOW
				hls::stream<Memory_write_t> memory_write_stream;
				memory_writer::memory_writer(memory_write_stream, din);

				hls::stream<Memory_query_t> memory_query_stream;
				hls::stream<Partial_bilinear_info_t> partial_bilinear_info_stream;
            	#pragma HLS STREAM variable=partial_bilinear_info_stream depth=32
				//  need to seperate the query_generator and bilinear_info_generator in memory_querier
				//since csim will operate sequentially
				memory_querier::query_generator(memory_query_stream, partial_bilinear_info_stream);

				hls::stream<FourPixel_t> memory_rdata_stream;
				memory_manager::memory_manager(memory_rdata_stream, memory_write_stream, memory_query_stream);
				
				hls::stream<Bilinear_info_t> bilinear_info_stream;
				memory_querier::bilinear_info_generator(bilinear_info_stream, partial_bilinear_info_stream, memory_rdata_stream);

				bilinear_interpolater::bilinear_interpolater(dout, bilinear_info_stream);
			}
	}

}

#endif
