import numpy as np
import sys
import os
from PIL import Image

import time
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname) 


module_path = os.path.abspath(os.path.join(__file__, '../../../../host/base_delta'))
if module_path not in sys.path:
    sys.path.append(module_path)

from BD_encoder.BD_enc_cpu import *
from BD_decoder.BD_dec_cpu import *

import pickle


def save(result, filename, tile_size=4):
    # write to txt file, every row contain a uint8
    with open(filename, 'w') as file:
        for i in range(result.shape[0]):
            file.write(str(result[i]))
            file.write("\n")


        



if __name__ == "__main__":
    image_name = "middle_perspective_image.png"
    # load image
    img = Image.open(module_path + "/test_data/" + image_name)
    npimage = np.array(img)
    tile_size = 4

    # if TB_data folder does not exist, create it
    if not os.path.exists(module_path + "./TB_data"):
        os.makedirs(module_path + "./TB_data")


    reshaped_image = npimage.reshape(npimage.shape[0] // tile_size, tile_size, npimage.shape[1] // tile_size, tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(-1)
    save(reshaped_image, "./TB_data/input.txt")

    enc_result = bd_encoder(npimage, tile_size=4)

    packed_deltas = enc_result["deltas"]
    bases = enc_result["bases"]
    packed_bitlens = enc_result["bitlens"]

    # Unpack bitlens to get deltas bit lengths
    bitlens = unpack_bits_to_uint8s(packed_bitlens, np.ones(bases.shape[0] * bases.shape[1] * 3, dtype=np.uint8) * 4)
    bitlens = bitlens.reshape(bases.shape[0], bases.shape[1], 3)
    # Unpack deltas
    deltas_lengths = np.tile(bitlens[:, :, np.newaxis, np.newaxis, :], (1, 1, tile_size, tile_size, 1)).astype(np.uint8).reshape(-1)
    unpacked_deltas = unpack_bits_to_uint8s(packed_deltas, deltas_lengths)
    # Compute image
    deltas = unpacked_deltas.reshape(bases.shape[0], bases.shape[1], tile_size, tile_size, 3)

    total_length = bitlens.size + bases.size + deltas.size
    enc_hw_result = np.zeros(total_length, dtype=np.uint8)
    enc_hw_result_lenghts = np.zeros(total_length, dtype=np.uint8)
    count = 0
    for i in range(bases.shape[0]):
        for j in range(bases.shape[1]):
            # save bases
            for k in range(3):
                enc_hw_result_lenghts[count] = 8
                enc_hw_result[count] = bases[i, j, k]
                count += 1

            # save bitlens
            for k in range(3):
                enc_hw_result_lenghts[count] = 4
                enc_hw_result[count] = bitlens[i, j, k]
                count += 1
            
            # save deltas
            for l in range(tile_size):
                for m in range(tile_size):
                    for k in range(3):
                        enc_hw_result_lenghts[count] = bitlens[i, j, k]
                        enc_hw_result[count] = deltas[i, j, l, m, k]
                        count += 1
    import ipdb; ipdb.set_trace()
    packed_enc_hw_result = pack_data_numba_wrapper(enc_hw_result.reshape(-1), enc_hw_result_lenghts.reshape(-1))

    save(packed_enc_hw_result, "./TB_data/enc_hw_result.txt")

