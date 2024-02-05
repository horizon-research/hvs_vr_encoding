
#include <hls_stream.h>
#include <ap_int.h>
#include "../dma.h"

void result_checker(  ap_uint<32> &error_num, ap_uint<32> &test_frame_num, hls::stream<dma_t> &axis_mm2s);
