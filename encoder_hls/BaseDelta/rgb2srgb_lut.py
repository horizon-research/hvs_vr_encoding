import numpy as np
import ipdb 

def RGB2sRGB(RGB):
    RGB = np.clip(RGB, 0, 1)
    print(RGB)
    if RGB <= 0.0031308:
        return int(12.92 * RGB*255)
    else:
        return int((1.055 * RGB ** (1/2.4) - 0.055)*255)

# Generate the LUT
lut_size = 256
lut = [RGB2sRGB(float(i) / 255.0) for i in range(lut_size)]

print(lut)

ipdb.set_trace()

# Print the LUT in C++ array format
print("std::array<double, {}> RGB2sRGB_LUT = {{".format(lut_size))
for value in lut:
    print("    {},".format(value))
print("};")