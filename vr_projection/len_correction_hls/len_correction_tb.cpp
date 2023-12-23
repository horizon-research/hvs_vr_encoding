#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>
#include "len_correction_func.h"


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
    std::ifstream ifin, ifref;
    std::ofstream hw_out;
    ifin.open("dump/ins.txt");
    ifref.open("dump/outs.txt");

    hw_out.open("dump/csim_hw_out.txt", std::ios::out | std::ios::trunc);

    hls::stream<ap_uint<8>> ins("ins");
    hls::stream<ap_uint<8>> refs("refs");
    load(ins, 1920 * 1080 * 3, ifin);
    load(refs, 1920 * 1080 * 3, ifref);

    // construct input
    hls::stream<Agg_in_srgb> is("is");
    for(int i = 0; i < int(1920 * 1080 / 16); i++)
    {
        Agg_in_srgb in;
        for(int j = 0; j < 16; j++)
        {
            for (int k = 0; k < 3; k++) {
                in.rgb[j][k] = ins.read();
            }
        }
        is.write(in);
    } 

    // otput is len
    std::cout << "is size: " << is.size() << std::endl;
    
    // Call the top function
    hls::stream<Pixel> os("os");
    len_correction_func(os, is);

    float acc_err = 0;
    int count = 0;
    float max_err = 0;
    int max_ti = 0;  

    // Check output
    while (!os.empty())
    {
        Pixel o = os.read();
        Pixel r;

        r.r = refs.read();
        r.g = refs.read();
        r.b = refs.read();

        float err = 0;

        err += fabs(float(o.b - r.b));
        err += fabs(float(o.g - r.g));
        err += fabs(float(o.r - r.r));

        err /= 3.0;
        acc_err += err;
        count += 1;

        // std::cout << "err: " << err << std::endl;
        // std::cout << "count: " << count << std::endl;
        // std::cout << "out: " << int(o.r) << " " << int(o.g) << " " << int(o.b) << std::endl;
        // std::cout << "ref: " << int(r.r) << " " << int(r.g) << " " << int(r.b) << std::endl; // 251 , 1180


        if (err > max_err) {
            max_err = err;
            max_ti = count;
        }

        if(hw_out.is_open())
        {
            hw_out << int(o.r) << " " << int(o.g) << " " << int(o.b) << std::endl;
        }
        else
        {
            std::cout << "Unable to open file" << std::endl;
        }
    }

    std::cout << "final avg_err: " << acc_err / float(count) << std::endl;
    std::cout << "final max_err: " << float(max_err) << std::endl;
    std::cout << "final max_ti: " << max_ti << std::endl;

}
