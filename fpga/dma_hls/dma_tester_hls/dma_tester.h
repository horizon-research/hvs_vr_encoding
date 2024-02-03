
#include <hls_stream.h>
#include <ap_int.h>
#include "../dma.h"

void dma_tester (hls::stream<dma_t> &axis_s2mm, hls::stream<ap_uint<32>> &error_nums, hls::stream<dma_t> &axis_mm2s);
void test_pattern_generator(hls::stream<dma_t> &axis_s2mm);
void result_checker(hls::stream<ap_uint<32>> &error_nums, hls::stream<dma_t> &axis_mm2s);