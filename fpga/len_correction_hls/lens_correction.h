#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>

void len_correction_func(
	hls::stream< Pixel > &dout,
	hls::stream< Agg_in_srgb > &din
	);