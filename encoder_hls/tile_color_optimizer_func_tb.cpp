#include "tile_color_optimizer_func.h"

template<int N>
void load(hls::stream<ap_uint<N>> &s, int nb, std::string dname, int dnum)
{
	std::cout << " ...loading: " + dname << std::endl;
    std::ifstream data;
    int nbuf = 0;
    ap_uint<N> buf = 0;
    int _rd;
    ap_int<N> rd;
    int dcount = 0;
    data.open(dname);  // TODO: How to Check it ?
    assert(!data.fail());
    int i = 0;
    while(!data.eof())
    {
        data >> _rd; // give value to int first, then ap_uint,  or it will report error
//        std::cout << _rd << std::endl;
		rd = _rd;
        rd.range(N-1, nb) = 0;
//         std::cout << rd << std::endl;
        if(nbuf + nb < N)
        {
           buf = ap_uint<N>(rd << nbuf) | buf;
           nbuf += nb;
//           for(int i = N-1; i >= 0; i--)
//           {
//        	   std::cout << buf[i];
//           }
//           std::cout << std::endl;
        }
        else
        {
           buf = ap_uint<N>(rd << nbuf) | buf;
           s << buf;
           nbuf = nbuf + nb - N;
           buf = rd >> (nb - nbuf);
           dcount += N;
        }
    }
    if(dcount < dnum)
    	s << buf;
};

int main()
{
    hls::stream<ap_uint<W1>> is("is");
    hls::stream<ap_uint<W2>> os("os"), ref("ref");
    // Read input and ref GT
    for(int i = 0; i < n_frame; i++)
    {
        load(is, 8, in_name, mcp.fec_param.n_ldpc * 8);  //no padding
        load(ref, mcp.mod_nb, ref_name, mcp.fec_param.k_ldpc); //padding - what about 128qam ?
    }

    // Call Hardware
    vr_prototype::tile_color_optimizer_func(os, is);


    // Compare output
    while(!os.empty())
    {
    	tn ++;
    	auto _os  = os.read();
    	auto _ref = ref.read();
    	CAPTURE(tn);
		CAPTURE(fidx);
    	CHECK(_os == _ref);
    	if(tn == trans_per_frame)
    	{
    		tn = 0;
    		fidx ++;
    	}
    }

}