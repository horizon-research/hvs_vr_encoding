#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>
#include "BD_enc.h"


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
    ifin.open("TB_data/input.txt");

    // get the line count of the file
    ifref.open("TB_data/enc_hw_result.txt");
    int lineCount = 0;
    std::string line;
    while (getline(ifref, line)) {
        ++lineCount;
    }
    std::cout << "lineCount: " << lineCount << std::endl;
    ifref.close();

    ifref.open("TB_data/enc_hw_result.txt");
    hw_out.open("TB_data/csim_hw_out.txt", std::ios::out | std::ios::trunc);

    hls::stream<ap_uint<8>> fins("ins");
    hls::stream<ap_uint<8>> frefs("refs");

    load(fins, 1080 * 960 * 3, ifin);
    load(frefs, lineCount, ifref);

    // construct input
    hls::stream<SixteenPixel_t> ins("ins");
    hls::stream<ap_uint<8>> refs("refs");
    for (int i = 0; i < 1080 * 960 /16 ; i++) {
        SixteenPixel_t in;
        for (int j = 0; j < 16; j++) {
            Pixel_t p;
            p.r = fins.read();
            p.g = fins.read();
            p.b = fins.read();
            in.data[j] = p;
        }
        ins.write(in);
    }

    for (int i = 0; i < lineCount; i++) {
        ap_uint<8> p;
        p = frefs.read();
        refs.write(p);
    }

    //padding ref to mutiplied of datasize / 8
    int ref_size = refs.size();
    if (ref_size % (data_size / 8) != 0) {
        int pad_size = (ref_size / (data_size / 8) + 1) * (data_size / 8) - ref_size;
        for (int i = 0; i < pad_size; i++) {
            refs.write(0);
        }
    }

    if (refs.size() % (data_size / 8) != 0) {
        std::cout << "refs size is not multiple of data_size/8" << std::endl;
        return 1;
    }

    std::cout << "Input tile num: " << ins.size() << std::endl;
    std::cout << "Ref byte num: " << refs.size() << std::endl;
    
    // Call the top function
    hls::stream<dma_t> os("os");
    bd_enc(os, ins);

    // Check Left Input
    std::cout << "Left Input size: " << ins.size() << std::endl;

    //Check output
    std::cout << "Output size: " << os.size() << std::endl;

    float acc_err = 0;
    int count = 0;
    float max_err = 0;
    int max_ti = 0;  

    // Check output
    dma_t o;
    while (!os.empty())
    {
        o = os.read();
        data_t odata = o.data;
        for (int i = 0; i < data_size / 8 ; i++) {
            if (refs.empty()) {
                std::cout << "refs is empty, please check padding" << std::endl;
                return 1;
            }
            ap_uint<8> r = refs.read();
            int _o = (odata.range(8*(i+1)-1, 8*i));
            if  ( _o != int(r)) {
                // error position 
                std::cout << "Error position: " << i << std::endl;
                std::cout << "Error: " << _o << " != " << int(r) << std::endl;
                return 1;
            }
        }
    }
    if (o.last != true){
        std::cout << "Last Signal Error: " << o.last << " != " << true << std::endl;
        return 1;
    }

}
