#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Customizing-AXI4-Stream-Interfaces
// I can not find axis with 6 template inputs as shown in above link, but only four template inputs as shown below
// template <typename T,
//           std::size_t WUser = 0,
//           std::size_t WId = 0,
//           std::size_t WDest = 0,
//           uint8_t EnableSignals = (AXIS_ENABLE_KEEP |
//                                    AXIS_ENABLE_LAST |
//                                    AXIS_ENABLE_STRB),
//           bool StrictEnablement = false>

struct Pixel_t { // Pynq is BGR format
	ap_uint<8> b;
	ap_uint<8> g;
	ap_uint<8> r;
};

typedef hls::axis<Pixel_t, 0, 0, 0>  Axis_pixel_t;

void double_output_func(hls::stream<Axis_pixel_t> &os, hls::stream<Pixel_t> &is);