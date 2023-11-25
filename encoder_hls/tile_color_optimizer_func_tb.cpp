#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>

#include "tile_color_optimizer_func.h"

template<typename T>
void load(hls::stream<T> &s, int load_num, std::ifstream &ifdata)
{
	// std::cout << " ...loading: " + dname << std::endl;
//    assert(!data.fail());

    int loaded_num = 0;
    while(!ifdata.eof())
    {
        std::string line;
        std::getline(ifdata, line);
    //    std::cout << line << std::endl;

        // std::cout << "line: " << line << std::endl;
        float d=0;
        if (line == "\n") {
            std::cout << "Meet final \n" << std::endl;
            break;
        } else {
            d = std::stof(line);
        }
        // std::cout << "d: " << d << std::endl;


        // std::cout << "T(d): " << T(d) << std::endl;


        s.write(T(d));
        loaded_num = loaded_num + 1;
        if(loaded_num == load_num) 
            break;
    }


    assert(loaded_num == load_num);
};

int main()
{

    int image_tiles_num = 129600 ;
    int tile_num_one_time = 100;
    std::ifstream ifa, ifb, ifc, ifd, ifk, ifl, ifref;
    ifa.open("dump/a0.txt");
    ifb.open("dump/b0.txt");
    ifc.open("dump/c0.txt");
    ifd.open("dump/d0.txt");
    ifk.open("dump/k0.txt");
    ifl.open("dump/l0.txt");
    ifref.open("dump/ref0.txt");

    std::ofstream hw_out;
    hw_out.open("dump/hw_out.txt", std::ios::out | std::ios::trunc);

    float acc_err = 0;
    int count = 0;
    float max_err = 0;

    for (int ti = 0; ti < image_tiles_num; ti+=tile_num_one_time) {
        std::cout << "progress: " << float(ti) / float(image_tiles_num) << std::endl;
        std::cout << "current max_err: " << float(max_err) << std::endl;
        std::cout << "current avg_err: " << acc_err / float(count) << std::endl;

        hls::stream<abc_t> as("as"), bs("bs"), cs("cs");
        hls::stream<dkl_t>  ds("ds"), ks("ks"), ls("ls");
        hls::stream<rgb_t> ref("ref");
        load(as, tile_num_one_time*4*4, ifa);
        load(bs, tile_num_one_time*4*4, ifb);
        load(cs, tile_num_one_time*4*4, ifc);
        load(ds, tile_num_one_time*4*4, ifd);
        load(ks, tile_num_one_time*4*4, ifk);
        load(ls, tile_num_one_time*4*4, ifl);
        load(ref, tile_num_one_time*4*4*3, ifref);

        // Call Hardware
        hls::stream<agg_inputs> is("is");
        hls::stream<agg_outputs_srgb> os("os");

        //Construct input
        for(int i = 0; i < tile_num_one_time; i++)
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


        // Call the top function
        for (int i = 0; i < tile_num_one_time; i++)
        {
            tile_color_optimizer_func(os, is);
        }
        // std::cout << "os.size(): " << os.size() << std::endl;
        // std::cout << "ref.size(): " << ref.size() << std::endl;


        // Compare output
        for (int i = 0; i < tile_num_one_time; i++)
        {
            agg_outputs_srgb _os;
            _os = os.read();
            for(int i2 = 0; i2 < 16; i2++)
            {
                for (int j = 0; j < 3; j++) { //r,g,b]
                    rgb_t ref_read = ref.read();
                    float ref_read_f = float(ref_read);
                    if (ref_read_f < 0) {
                        ref_read_f = 0;
                    } else if (ref_read_f > 1) {
                        ref_read_f = 1;
                    }
                    if (ref_read_f < 0.0031308) {
                        ref_read_f = ref_read_f * 12.92 * 255;
                    } 
                    else {
                        ref_read_f = (1.055 * std::pow(ref_read_f, 1.0/2.4) - 0.055)*255;
                    }
                    ap_uint<8> ref_read_uint8 = ap_uint<8>(ref_read_f);

                    // std::cout << "ref.read(): " << float(ref_read) << std::endl;
                    // std::cout << "ref_read_f: " << ref_read_f << std::endl;
                    // std::cout << "ref_read_uint8: " << ref_read_uint8 << std::endl;
                    // std::cout << "_os.rgb[i2][j]: " << _os.rgb[i2][j] << std::endl;

                    float err = std::fabs( float(_os.rgb[i2][j] - ref_read_uint8) ); 

                    if(hw_out.is_open())
                    {
                       hw_out << int(_os.rgb[i2][j]) << " ";
                    }
                    else
                    {
                       std::cout << "Unable to open file" << std::endl;
                    }

                    if (err > max_err) {
                        max_err = err;
                    }
                    acc_err += err;
                    count += 1;
                }
            }
        }
    }



    std::cout << "final avg_err: " << acc_err / float(count) << std::endl;

}
