#include <cmath>    // 对于 fabs
#include <iostream>
#include <fstream>
#include <string>
#include <hls_stream.h>

#include "rearrangement.h"

int main()
{
    hls::stream<Pixel> os("os"), refs("refs");
    hls::stream<Agg_in_srgb> is("is");

    // input generate
    for (int i = 0; i < 1920 / 4; i++)
    {
        Agg_in_srgb in;
        for (int j = 0; j < 16; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                in.rgb[j][k] = j + i * 16;
            }
        }
        is.write(in);
    }

    // reference generate
    for (int i = 0; i < 1920 * 4; i++)
    {
        int row_idx = i / 1920;
        int col_idx = i % 1920;
        int tile_idx = col_idx / 4;
        Pixel ref;
        ref.r = tile_idx * 16 + row_idx * 4 + col_idx % 4;
        ref.g = tile_idx * 16 + row_idx * 4 + col_idx % 4;
        ref.b = tile_idx * 16 + row_idx * 4 + col_idx % 4;
        refs.write(ref);
    }

    rearrangement_func(
        os,
        is
    );

    // Check output
    int count = 0;
    while (!os.empty())
    {
       auto out = os.read();
       auto ref = refs.read();
      if (out.r != ref.r) {
          std::stringstream ss;
          ss << "Error: out.r (" << out.r << ") != ref.r (" << ref.r << ")" << " at pxixel: " << count << std::endl;
          throw std::runtime_error(ss.str());
      }

      if (out.g != ref.g) {
          std::stringstream ss;
          ss << "Error: out.g (" << out.g << ") != ref.g (" << ref.g << ")" << " at pxixel: " << count << std::endl;
          throw std::runtime_error(ss.str());
      }

      if (out.b != ref.b) {
          std::stringstream ss;
          ss << "Error: out.b (" << out.b << ") != ref.b (" << ref.b << ")" << " at pxixel: " << count << std::endl;
          throw std::runtime_error(ss.str());
      }

        count += 1;

    }

}
