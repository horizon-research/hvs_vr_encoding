#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>
#include "BD_dec.h"


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
    if(loaded_num != load_num) {
        std::cout << "loaded_num: " << loaded_num << " != " << "load_num: " << load_num << std::endl;
    }
};

int main()
{
    std::ifstream ifin, ifref;
    std::ofstream hw_out;
    ifref.open("TB_data/input.txt");

    // get the line count of the file
    ifin.open("TB_data/enc_hw_result.txt");
    int lineCount = 0;
    std::string line;
    while (getline(ifin, line)) {
        ++lineCount;
    }
    std::cout << "lineCount: " << lineCount << std::endl;
    ifin.close();

    ifin.open("TB_data/enc_hw_result.txt");

    hw_out.open("TB_data/csim_hw_out.txt", std::ios::out | std::ios::trunc);

    hls::stream<ap_uint<8>> fins("ins");
    hls::stream<ap_uint<8>> frefs("refs");

    load(fins, lineCount, ifin);
    load(frefs, 1080 * 960 * 3, ifref);

    // construct refs
    hls::stream<dma_t> ins("ins");
    hls::stream<SixteenPixel_t> refs("refs");

    for (int i = 0; i < 1080 * 960 /16 ; i++) {
        SixteenPixel_t in;
        for (int j = 0; j < 16; j++) {
            Pixel_t p;
            p.r = frefs.read();
            p.g = frefs.read();
            p.b = frefs.read();
            in.data[j] = p;
        }
        refs.write(in);
    }


    //padding ins to mutiplied of datasize / 8
    int in_size = fins.size();
    if (in_size % (data_size / 8) != 0) {
        int pad_size = (in_size / (data_size / 8) + 1) * (data_size / 8) - in_size;
        for (int i = 0; i < pad_size; i++) {
            fins.write(0);
        }
    }
    // construct ins
    int ins_size = fins.size();
    std::cout << "ins_size: " << ins_size << std::endl;
    for (int i = 0; i < ins_size / (data_size / 8) ; i++) {
        dma_t p;
        for (int j = 0; j < (data_size / 8); j++) {
            ap_uint<8> in = fins.read();
            p.data.range(8*(j+1)-1, 8*j) = in;
        }
        p.last = (i == ins_size / (data_size / 8) - (data_size / 8));
        ins.write(p);
    }

    std::cout << "Input num: " << ins.size() << std::endl;
    std::cout << "Ref pixel num: " << refs.size() << std::endl;
    
    // Call the top function
    hls::stream<SixteenPixel_t> os("os");
    bd_dec(os, ins);

    // Check Left Input
    std::cout << "Left Input size: " << ins.size() << std::endl;

    //Check output
    std::cout << "Output size: " << os.size() << std::endl;
    // Check output
    SixteenPixel_t o;
    SixteenPixel_t ref;
    int count = 0;
    while (!os.empty())
    {
        o = os.read();
        ref = refs.read();
        for (int i = 0; i < 16 ; i++) {
            Pixel_t r = ref.data[i];
            Pixel_t p = o.data[i];
            // std::cout << " count: " << count << ", o: " << p.r << " " << p.g << " " << p.b << " , r: " << r.r << " " << r.g << " " << r.b << std::endl;
            if (p.r != r.r || p.g != r.g || p.b != r.b) {
                std::cout << " count: " << count << ", Error: " << p.r << " " << p.g << " " << p.b << " != " << r.r << " " << r.g << " " << r.b << std::endl;
                return 1;
            }
            count += 1;
        }
    }
}
