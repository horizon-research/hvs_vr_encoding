template<int TB_TIMES>
void dkl2rgb_loop(hls::stream<rgb_t_array> &rgb_stream, hls::stream<dkl_t_array> &dkl_stream) {
    dkl_t_array dkl_centers_i;
    while (true) 
    {
        #pragma HLS PIPELINE II=1 
        #pragma HLS ARRAY_PARTITION variable=dkl_centers_i.data dim=0 complete
        if (dkl_stream.read_nb(dkl_centers_i)){
            rgb_t_array rgb_centers_i;
            #pragma HLS ARRAY_PARTITION variable=rgb_centers_i.data dim=0 complete
            mm_3x3to3(rgb_centers_i.data, DKL2RGB, dkl_centers_i.data);
            rgb_stream.write(rgb_centers_i);
        }
        else {
            // pass
        }
    }
}