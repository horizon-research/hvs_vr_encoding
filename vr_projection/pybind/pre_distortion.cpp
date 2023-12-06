#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <utility> // for std::pair

namespace py = pybind11;

void bilinearInterpolate(uint8_t result[3], const uint8_t image[1080][960][3], const float &y, const float &x) {
    int x1 = static_cast<int>(x);
    int y1 = static_cast<int>(y);
    int x2 = x1 + 1;
    int y2 = y1 + 1;

    // Calculate differences
    float dx = x - x1;
    float dy = y - y1;

    // Interpolate using the four nearest pixels
    for (int channel = 0; channel < 3; channel++) { 
        result[channel] = static_cast<uint8_t>(
            (1 - dx) * (1 - dy) * image[y1][x1][channel] +
            dx * (1 - dy) * image[y1][x2][channel] +
            (1 - dx) * dy * image[y2][x1][channel] +
            dx * dy * image[y2][x2][channel]
        );
    }
}

void distortPixel(float &xDistorted, float &yDistorted, const float &_x, const float &_y) {
    const float k1 = 0.33582564;
    const float k2 = 0.55348791;
    const float cx = 960 / 2;
    const float cy = 1080 / 2;

    float x = _x - cx;

    float y = _y - cy;

    const float ppi = 401.0f;
    x = x / ppi * 25.4f;
    y = y / ppi * 25.4f;
    const float z = 39.07f;

    x = x / z;
    y = y / z;
    float r2 = x * x + y * y;
    float factor = 1 + k1 * r2 + k2 * r2 * r2;

    xDistorted = x * factor;
    yDistorted = y * factor;

    xDistorted = xDistorted * z;
    yDistorted = yDistorted * z;

    xDistorted = xDistorted / 25.4f * ppi;
    yDistorted = yDistorted / 25.4f * ppi;

    xDistorted += cx;
    yDistorted += cy;
}

void preDistortImage(uint8_t* _outputImage, const uint8_t* _inputImage) {
    // duplicate the image
    uint8_t (*outputImage)[960][3] = (uint8_t (*)[960][3]) _outputImage;
    uint8_t (*inputImage)[960][3] = (uint8_t (*)[960][3]) _inputImage;
    



    for (int i = 0; i < 960; ++i) {
        for (int j = 0; j < 1080; ++j) {
            float xDistorted, yDistorted;
            distortPixel(xDistorted, yDistorted, i, j);
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



PYBIND11_MODULE(pre_distortion, m) {
    m.doc() = "pybind11 pre_distortion plugin"; // Optional module docstring

    m.def("preDistortImage", [](py::array_t<uint8_t>& outputImage, const py::array_t<uint8_t>& inputImage) {
        auto outImg = outputImage.mutable_unchecked<3>();
        auto inImg = inputImage.unchecked<3>();
        preDistortImage(outImg.mutable_data(0, 0, 0), inImg.data(0, 0, 0));
    });
}