import os
import sys

file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname) 

import pickle
from unpack_cpu import unpack_bits_to_int8s, unpack_bits_to_uint8s
import numpy as np

from PIL import Image

import time

def bd_decoder(enc_result, tile_size=4):
    packed_deltas = enc_result["deltas"]
    bases = enc_result["bases"]
    packed_bitlens = enc_result["bitlens"]

    img_height = bases.shape[0] * tile_size
    img_width = bases.shape[1] * tile_size

    # Unpack bitlens to get deltas bit lengths
    bitlens = unpack_bits_to_uint8s(packed_bitlens, np.ones(bases.shape[0] * bases.shape[1] * 3, dtype=np.uint8) * 4)
    bitlens = bitlens.reshape(bases.shape[0], bases.shape[1], 3)


    # Unpack deltas
    deltas_lengths = np.tile(bitlens[:, :, np.newaxis, np.newaxis, :], (1, 1, tile_size, tile_size, 1)).astype(np.uint8).reshape(-1)
    unpacked_deltas = unpack_bits_to_uint8s(packed_deltas, deltas_lengths)

    # Compute image
    deltas = unpacked_deltas.reshape(bases.shape[0], bases.shape[1], tile_size, tile_size, 3)

    img = deltas + bases[:, :, np.newaxis, np.newaxis, :]
    img = img.transpose(0, 2, 1, 3, 4)
    img = img.reshape(img_height, img_width, 3).astype(np.uint8)

    return img


if __name__ == "__main__":
    filename = '../test_data/enc_result.pkl'
    with open(filename, 'rb') as file:
        enc_result = pickle.load(file)

    # warm up numba JIT
    img = bd_decoder(enc_result, tile_size=4)

    test_time = 10
    t1 = time.time()
    for i in range(test_time):
        img = bd_decoder(enc_result, tile_size=4)
    t2 = time.time()

    print ("FPS: " + str(test_time / (t2 - t1)))



    



    