import numpy as np
import math


import os
import sys
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + "/../../../host/len_correction")
from len_correction_cpu import Len_correction


def get_bb(y_distorted):
    min_ys = np.min(y_distorted, axis=1)
    max_ys = np.max(y_distorted, axis=1)
    min_ys = np.floor(min_ys).astype(np.int32)
    max_ys = np.ceil(max_ys).astype(np.int32)
    min_ys[min_ys < 0] = 0
    max_ys[max_ys > 1079] = 1079
    return min_ys, max_ys


def get_max_buffersize(min_ys, max_ys):
    max_buffersize = 0
    max_buffersize = max(max_ys - min_ys)
    return math.ceil(max_buffersize)

def get_insert_nums(min_ys, max_ys):
    insert_nums = []
    max_y_in_buffer = -1
    for i in range(len(min_ys)):
        if max_y_in_buffer < max_ys[i]:
            insert_nums.append(max_ys[i] - max_y_in_buffer)
            max_y_in_buffer = max_ys[i]
        else:
            insert_nums.append(0)
    return insert_nums
    

def generate_header_for_c(max_buffersize, insert_nums):
    header_content = f"""#ifndef PRE_COMPUTE_LEN_CORRECTION_CONSTANT_H
    #define PRE_COMPUTE_LEN_CORRECTION_CONSTANT_H

    #include <ap_int.h>

    const ap_uint<{math.ceil(math.log2(max_buffersize))}> buffer_row_num = {max_buffersize};
    const ap_uint<{math.ceil(math.log2(np.asarray(insert_nums).max()))}> discards[1080] = {{{', '.join(map(str, insert_nums[:1080]))}}};

    #endif // PRE_COMPUTE_LEN_CORRECTION_CONSTANT_H
    """
    with open('precompute_constant.h', 'w') as file:
        file.write(header_content)



if __name__ == "__main__":  
    height, width = 1080, 960
    k1, k2 = 0.33582564, 0.55348791 # Radial distortion coefficients
    cx, cy = width / 2, height / 2 # Assuming center of the image is the optical center
    
    # for dynamic len correction, use  Len_correction.update_display_distance(self, z) to update z
    len_correction = Len_correction(k1, k2, cx, cy, height, width, ppi=401, inch2mm=25.4, z=39.07)
    y_distorted = len_correction.y_distorted

    min_ys, max_ys = get_bb(y_distorted)
    max_buffersize = get_max_buffersize(min_ys, max_ys)
    insert_nums = get_insert_nums(min_ys, max_ys)

    generate_header_for_c(max_buffersize, insert_nums)

    print("Done!")


