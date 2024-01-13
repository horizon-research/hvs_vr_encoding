#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>


// #define FP32
// #ifdef FP32
#include "tile_color_optimizer_func.h"
// #else
// 	#include "tile_color_optimizer_func_fix.h"
// #endif

template<typename T>
void load(hls::stream<T> &s, int load_num, std::ifstream &ifdata)
{
    int loaded_num = 0;
    while(!ifdata.eof())
    {
        std::string line;
        std::getline(ifdata, line);
        float d=0;
        if (line == "\n") {
            std::cout << "Meet final \n" << std::endl;
            break;
        } else {
            d = std::stof(line);
        }
        s.write(T(d));
        loaded_num = loaded_num + 1;
        if(loaded_num == load_num) 
            break;
    }


    assert(loaded_num == load_num);
};

int main()
{
    int image_tiles_num = 100;
    int tile_num_one_time = 1;
    std::ifstream ifabc, ifdkl, ifref;
    ifabc.open("gold_sequence/TB_data/centers_abc.txt");
    ifdkl.open("gold_sequence/TB_data/dkl_centers.txt");
    ifref.open("gold_sequence/TB_data/gold_sequence.txt");

    std::ofstream hw_out;
    hw_out.open("gold_sequence/csim_hw_out.txt", std::ios::out | std::ios::trunc);

    float acc_err = 0;
    int count = 0;
    float max_err = 0;
    int max_ti = 0;  

    for (int ti = 0; ti < image_tiles_num; ti+=tile_num_one_time) {
        if (ti % 100 == 0)
        {
            std::cout << "progress: " << float(ti) / float(image_tiles_num) << std::endl;
            std::cout << "current max_err: " << float(max_err) << std::endl;
            std::cout << "current avg_err: " << acc_err / float(count) << std::endl;
        }

        hls::stream<abc_t> abcs("abcs");
        hls::stream<dkl_t> dkls("dkls");
        hls::stream<ap_uint<8>> refs("refs"); // SRGB
        load(abcs, tile_num_one_time*4*4*3, ifabc);
        load(dkls, tile_num_one_time*4*4*3, ifdkl);
        load(refs, tile_num_one_time*4*4*3, ifref);

        // Call Hardware
        hls::stream<agg_inputs> is("is");
        hls::stream<agg_outputs_srgb> os("os");

        //Construct input
        for(int i = 0; i < tile_num_one_time; i++)
        {
            agg_inputs in;
            for(int j = 0; j < 16; j++)
            {
                in.as[j] = abcs.read();
                in.bs[j] = abcs.read();
                in.cs[j] = abcs.read();
                in.ds[j] = dkls.read();
                in.ks[j] = dkls.read();
                in.ls[j] = dkls.read();
            }
            is.write(in);
        } // end


        // Call the top function
            for (int i = 0; i < tile_num_one_time; i++)
        {
            tile_color_optimizer_func(os, is);
        }



        // Compare output and reference
        for (int i = 0; i < tile_num_one_time; i++)
        {
            agg_outputs_srgb _os;
            _os = os.read();
            for(int i2 = 0; i2 < 16; i2++)
            {
                for (int j = 0; j < 3; j++) { //r,g,b]
                    ap_uint<8> ref = refs.read();
                    float err = std::fabs( float(_os.rgb[i2][j] - ref) ); 
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
                        max_ti = ti;
                    }
                    acc_err += err;
                    count += 1;
                }
            }
        }
    }



    std::cout << "final avg_err: " << acc_err / float(count) << std::endl;
    std::cout << "final max_err: " << float(max_err) << std::endl;
    std::cout << "final max_ti: " << max_ti << std::endl;

}
