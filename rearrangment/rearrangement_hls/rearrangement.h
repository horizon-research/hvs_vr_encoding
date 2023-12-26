#include <hls_stream.h>
#include <ap_int.h>


struct Agg_in_srgb {
	ap_uint<8> rgb[16][3];
};

struct Pixel { // Pynq is BGR format
    ap_uint<8> b;
    ap_uint<8> g;
    ap_uint<8> r;
};


namespace vr_prototype
{
    template <int tile_size = 4, int img_width=1920>
    void rearragement(
        hls::stream<Pixel> &out,
        hls::stream<Agg_in_srgb> &in
    )
    {
        #pragma HLS INTERFACE axis port=out
        #pragma HLS INTERFACE axis port=in
        #pragma HLS INTERFACE ap_ctrl_none port=return
        Pixel pixel_buffer_1[tile_size][img_width];
        const int load_num = img_width / tile_size; // load 4x4 pixels at a time
        const int out_num = img_width * tile_size; // output pixel by pixel
        const int total_num = load_num + out_num;

        for(int i = 0; i < total_num; i++)
        {
            #pragma HLS PIPELINE II=1 rewind
            if (i < load_num)
            {
                Agg_in_srgb agg_in = in.read();
                for(int j = 0; j < tile_size * tile_size; j++)
                {
                    int row_idx = j / tile_size;
                    int col_idx = j % tile_size + i * tile_size; // shift + base address
                    pixel_buffer_1[row_idx][col_idx].r = agg_in.rgb[j][0];
                    pixel_buffer_1[row_idx][col_idx].g = agg_in.rgb[j][1];
                    pixel_buffer_1[row_idx][col_idx].b = agg_in.rgb[j][2];
                }
            }
            else
            {
                int row_idx = (i - load_num) / img_width;
                int col_idx = (i - load_num) % img_width;
                out.write(pixel_buffer_1[row_idx][col_idx]);
            }
        }
    }
}