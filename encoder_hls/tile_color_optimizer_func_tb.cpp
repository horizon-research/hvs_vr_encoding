#include "tile_color_optimizer_func.h"
#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>
template<typename T>
void load(hls::stream<T> &s, int load_num, std::string dname)
{
	std::cout << " ...loading: " + dname << std::endl;
    std::ifstream data;
    data.open(dname);
//    assert(!data.fail());

    int loaded_num = 0;
    while(!data.eof())
    {
        std::string line;
        std::getline(data, line);
//        std::cout << line << std::endl;

        // std::cout << "line: " << line << std::endl;
        float d = std::stof(line);
        // std::cout << "d: " << d << std::endl;


        // std::cout << "T(d): " << T(d) << std::endl;


        s.write(T(d));
        loaded_num = loaded_num + 1;
        if(data.eof()) 
            break;
    }


    assert(loaded_num == load_num);
};

int main()
{
    hls::stream<ufixed_16_0_t> as("as"), bs("bs"), cs("cs");
    hls::stream<fixed_16_0_t>  ds("ds"), ks("ks"), ls("ls");
    hls::stream<ufixed_16_0_t> ref("ref");
    // Read input and ref GT
    load(as, 4*4, "dump/a0.txt");
    load(bs, 4*4, "dump/b0.txt");
    load(cs, 4*4, "dump/c0.txt");
    load(ds, 4*4, "dump/d0.txt");
    load(ks, 4*4, "dump/k0.txt");
    load(ls, 4*4, "dump/l0.txt");
    load(ref, 4*4*3, "dump/ref0.txt");

    // Call Hardware
    hls::stream<agg_inputs> is("is");
    hls::stream<agg_outputs> os("os");

    //Construct input
    for(int i = 0; i < 1; i++)
    {
        agg_inputs in;
        for(int j = 0; j < 16; j++)
        {
            in.as[j] = as.read();
            // std::cout << "in.as[j]: " << in.as[j] << std::endl;
            in.bs[j] = bs.read();
            in.cs[j] = cs.read();
            in.ds[j] = ds.read();
            in.ks[j] = ks.read();
            in.ls[j] = ls.read();
        }
        is.write(in);
    } // end


    vr_prototype::tile_color_optimizer_func(os, is);


    // Compare output
    float err = 1000;
    agg_outputs _os;
    while(!os.empty())
    {
        _os = os.read();
        for(int i = 0; i < 16; i++)
        {
            std::cout << "rgb: " << _os.rgb[i][0] << " " << _os.rgb[i][1] << " " << _os.rgb[i][2] << std::endl;
            std::cout << "ref.read(): " << ref.read()  << " " << ref.read() << " " << ref.read() << std::endl;
            // assert( std::fabs( float(_os.rgb[i][0] - ref.read()) < err)   ); //r
            // assert( std::fabs( float(_os.rgb[i][1] - ref.read()) < err)   ); //g
            // assert( std::fabs( float(_os.rgb[i][2] - ref.read()) < err)   ); //b
        }
    }

}
