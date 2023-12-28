#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <utility> // for std::pair

namespace py = pybind11;


class RearrangeForFPGA {
public:
    RearrangeForFPGA() {}

    void rearrangeForFPGA(uint8_t* _outputImage_tiles, const uint8_t* _inputImage) {
        // duplicate the image
        uint8_t (*outputImage_tiles)[960][5][4] = (uint8_t (*)[960][5][4]) _outputImage_tiles;
        uint8_t (*inputImage)[960][3] = (uint8_t (*)[960][3]) _inputImage;
        for (int i = 0; i < 960; ++i) {
            for (int j = 0; j < 1080; ++j) {
                int xDistorted, yDistorted;
                xDistorted = index[j][i][0];
                yDistorted = index[j][i][1];


                for (int k = 0; k < 3; k++) {
                    outputImage_tiles[j][i][k][0] = inputImage[yDistorted][xDistorted][k];
                    outputImage_tiles[j][i][k][1] = inputImage[yDistorted][xDistorted+1][k];
                    outputImage_tiles[j][i][k][2] = inputImage[yDistorted+1][xDistorted][k];
                    outputImage_tiles[j][i][k][3] = inputImage[yDistorted+1][xDistorted+1][k];
                }
                
                for (int k = 3; k < 5; k++) {
                    union {
                            float floatValue;
                            uint8_t intValues[4];
                    } data;
                    data.floatValue = weight[j][i][k-3]; // 0:dx, 1:dy
                    
                    outputImage_tiles[j][i][k][0] = data.intValues[0];
                    outputImage_tiles[j][i][k][1] = data.intValues[1];
                    outputImage_tiles[j][i][k][2] = data.intValues[2];
                    outputImage_tiles[j][i][k][3] = data.intValues[3];
                }
            }
        }
    }



    void readFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Fail to open file. " << filename << std::endl;
            return;
        }

        for (int i = 0; i < 1080; ++i) {
            for (int j = 0; j < 960; ++j) {
                for (int k = 0; k < 2; k++) {
                    if (!(file >> index[i][j][k])) {
                        std::cerr << "Error while reading index" << std::endl;
                        return;
                    }
                }
                for (int k = 0; k < 2; k++) {
                    if (!(file >> weight[i][j][k])) {
                        std::cerr << "Error while reading weight" << std::endl;
                        return;
                    }
                }
            }
        }
    }



private:
    int index[1080][960][2];
    float weight[1080][960][2];
};


PYBIND11_MODULE(rearrange_for_fpga, m) {
    m.doc() = "pybind11 pre_distortion plugin"; // Optional module docstring

    py::class_<RearrangeForFPGA>(m, "RearrangeForFPGA")
        .def(py::init<>())
        .def("rearrangeForFPGA", [](RearrangeForFPGA& _RearrangeForFPGA, py::array_t<uint8_t>& outputImage, const py::array_t<uint8_t>& inputImage) {
            auto outImg = outputImage.mutable_unchecked<4>();
            auto inImg = inputImage.unchecked<3>();
            
            _RearrangeForFPGA.rearrangeForFPGA(outImg.mutable_data(0, 0, 0, 0), inImg.data(0, 0, 0));
         })
        .def("readFromFile", &RearrangeForFPGA::readFromFile);
}