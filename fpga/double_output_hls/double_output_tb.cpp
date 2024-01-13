#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>
#include "double_output_func.h"


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
    std::ifstream ifin, ifref;
    std::ofstream hw_out;
    ifin.open("gold_sequence/TB_data/input.txt");
    ifref.open("gold_sequence/TB_data/gold_seq.txt");
    hw_out.open("gold_sequence/TB_data/csim_hw_out.txt", std::ios::out | std::ios::trunc);

    hls::stream<ap_uint<8>> fins("ins");
    hls::stream<ap_uint<8>> frefs("refs");

    load(fins, 1080 * 960 * 3, ifin);
    load(frefs, 1080 * 1920 * 3, ifref);

    // construct input
    hls::stream<Pixel_t> ins("ins");
    hls::stream<Pixel_t> refs("refs");
    for (int i = 0; i < 1080 * 960 ; i++) {
        Pixel_t p;
        p.r = fins.read();
        p.g = fins.read();
        p.b = fins.read();
        ins.write(p);

        p.r = frefs.read();
        p.g = frefs.read();
        p.b = frefs.read();
        refs.write(p);

        p.r = frefs.read();
        p.g = frefs.read();
        p.b = frefs.read();
        refs.write(p);
    }

    std::cout << "Input Pixel size: " << ins.size() << std::endl;
    std::cout << "Ref Pixel size: " << refs.size() << std::endl;
    
    // Call the top function
    hls::stream<Axis_pixel_t> os("os");
    double_output_func(os, ins);

    // Check Left Input
    std::cout << "Left Input size: " << ins.size() << std::endl;

    //Check output
    std::cout << "Output size: " << os.size() << std::endl;

    float acc_err = 0;
    int count = 0;
    float max_err = 0;
    int max_ti = 0;  

    // Check output
    while (!os.empty())
    {
        Axis_pixel_t o = os.read();
        Pixel_t r = refs.read();

        float err = 0;

        err += fabs(float(o.data.b - r.b));
        err += fabs(float(o.data.g - r.g));
        err += fabs(float(o.data.r - r.r));

        err /= 3.0;
        acc_err += err;
        count += 1;


        if (err > max_err) {
            max_err = err;
            max_ti = count;
        }

        if(hw_out.is_open())
        {
            hw_out << int(o.data.r) << " " << int(o.data.g) << " " << int(o.data.b) << std::endl;
        }
        else
        {
            std::cout << "Unable to open hw_out file" << std::endl;
        }
    }

    std::cout << "final avg_err: " << acc_err / float(count) << std::endl;
    std::cout << "final max_err: " << float(max_err) << std::endl;
    std::cout << "final max_ti: " << max_ti << std::endl;

}
