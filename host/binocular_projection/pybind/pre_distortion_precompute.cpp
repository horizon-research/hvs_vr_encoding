#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <utility> // for std::pair

namespace py = pybind11;


class PreDistortion {
public:
    PreDistortion() {}

    void bilinearInterpolate(uint8_t result[3], const uint8_t image[1080][960][3], const int &y, const int &x) {

        float dx = weight[y][x][0];
        float dy = weight[y][x][1];
        int x1 = x;
        int y1 = y;
        int x2 = x1 + 1;
        int y2 = y1 + 1;

        // Interpolate using the four nearest pixels
        for (int channel = 0; channel < 3; channel++) { 
            result[channel] = static_cast<uint8_t>(
                (1 - dx) * (1 - dy) * image[y1][x1][channel] +
                dx * (1 - dy) * image[y1][x2][channel] +
                (1 - dx) * dy * image[y2][x1][channel] +
                dx * dy * image[y2][x2][channel]
            );
        }

        // for (int channel = 0; channel < 3; channel++) { 
        //     image[y1][x1][channel];
        // }
    }

    void preDistortImage(uint8_t* _outputImage, const uint8_t* _inputImage) {
        // duplicate the image
        uint8_t (*outputImage)[960][3] = (uint8_t (*)[960][3]) _outputImage;
        uint8_t (*inputImage)[960][3] = (uint8_t (*)[960][3]) _inputImage;
        for (int i = 0; i < 960; ++i) {
            for (int j = 0; j < 1080; ++j) {
                int xDistorted, yDistorted;
                xDistorted = index[j][i][0];
                yDistorted = index[j][i][1];
                if (0 <= xDistorted && xDistorted < 960 && 0 <= yDistorted && yDistorted < 1080) {
                    // Obtain the interpolated pixel
                    uint8_t interpolatedPixel[3];
                    bilinearInterpolate(interpolatedPixel, inputImage, yDistorted, xDistorted);
                    // Set the pixel in the output image
                    outputImage[j][i][0] = interpolatedPixel[0]; // Red
                    outputImage[j][i][1] = interpolatedPixel[1]; // Green
                    outputImage[j][i][2] = interpolatedPixel[2]; // Blue
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


PYBIND11_MODULE(pre_distortion_precompute, m) {
    m.doc() = "pybind11 pre_distortion plugin"; // Optional module docstring

    py::class_<PreDistortion>(m, "PreDistortion")
        .def(py::init<>())
        .def("preDistortImage", [](PreDistortion& preDistort, py::array_t<uint8_t>& outputImage, const py::array_t<uint8_t>& inputImage) {
            auto outImg = outputImage.mutable_unchecked<3>();
            auto inImg = inputImage.unchecked<3>();
            
            preDistort.preDistortImage(outImg.mutable_data(0, 0, 0), inImg.data(0, 0, 0));
         })
        .def("readFromFile", &PreDistortion::readFromFile);
}