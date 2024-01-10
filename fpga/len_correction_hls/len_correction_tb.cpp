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
        // std::cout << line << std::endl;
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
    ifin.open("gold_sequence/input.txt");
    ifref.open("gold_sequence/gold_seq.txt");
    hw_out.open("gold_sequence/csim_hw_out.txt", std::ios::out | std::ios::trunc);

    hls::stream<ap_uint<8>> fins("ins");
    hls::stream<ap_uint<8>> frefs("refs");

    load(fins, 1920 * 1080 * 3, ifin);
    load(frefs, 1920 * 1080 * 3, ifref);

    // construct input
    hls::stream<Pixel> ins("ins");
    hls::stream<Pixel> refs("refs");
    for (int i = 0; i < 1920 * 1080 ; i++) {
        Pixel p;
        p.r = fins.read();
        p.g = fins.read();
        p.b = fins.read();
        ins.write(p);

        p.r = frefs.read();
        p.g = frefs.read();
        p.b = frefs.read();
        refs.write(p);
    }

    std::cout << "Input Pixel size: " << ins.size() << std::endl;
    std::cout << "Ref Pixel size: " << refs.size() << std::endl;
    
    // Call the top function
    hls::stream<Pixel> os("os");
    len_correction_func(os, ins);

    // Check Left Input
    std::cout << "Left Input size: " << ins.size() << std::endl;

    float acc_err = 0;
    int count = 0;
    float max_err = 0;
    int max_ti = 0;  

    // Check output
    while (!os.empty())
    {
        Pixel o = os.read();
        Pixel r = refs.read();

        float err = 0;

        err += fabs(float(o.b - r.b));
        err += fabs(float(o.g - r.g));
        err += fabs(float(o.r - r.r));

        err /= 3.0;
        acc_err += err;
        count += 1;


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
            std::cout << "Unable to open hw_out file" << std::endl;
        }
    }

    std::cout << "final avg_err: " << acc_err / float(count) << std::endl;
    std::cout << "final max_err: " << float(max_err) << std::endl;
    std::cout << "final max_ti: " << max_ti << std::endl;

}
