import numpy as np
import sys
import os
from PIL import Image

import time
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname) 

from pack_cpu import pack_data_numba_wrapper

import pickle


def bd_encoder(npimage, tile_size=4):
    # Assuming npimage is your input image with shape (height, width, 3)
    height, width, _ = npimage.shape

    # Reshape npimage to process 4x4 tiles, output shape (height // 4, width // 4, 4, 4, 3), uint8
    npimage = npimage.reshape(height // tile_size, tile_size, width // tile_size, tile_size, 3).transpose(0, 2, 1, 3, 4).astype(np.float32)

    # Compute min, max, and base for each tile and each color channel, output shape (height // 4, width // 4, 3), uint8
    tiles_min = npimage.min(axis=(2, 3))
    tiles_max = npimage.max(axis=(2, 3))
    bases = tiles_min

    # Compute bit lengths, output shape (height // 4, width // 4, 3), uint8
    v_range = tiles_max - tiles_min + 1
    bitlens = np.zeros_like(v_range, dtype=np.float32)
    bitlens[v_range > 0] = np.ceil(np.log2( v_range[v_range > 0])) 

    # Compute deltas, output shape (height // 4, width // 4, 4, 4, 3), int8
    deltas = (npimage - bases[:, :, np.newaxis, np.newaxis, :])

    # Pack bitlens
    bitlens_lengths = np.ones_like(bitlens, dtype=np.uint8) * 4
    bitlens = bitlens.astype(np.uint8)
    packed_bitlens = pack_data_numba_wrapper(bitlens.reshape(-1), bitlens_lengths.reshape(-1))

    # Pack deltas
    deltas = deltas.reshape(-1).astype(np.int8).view(np.uint8)
    deltas_lengths = np.tile(bitlens[:, :, np.newaxis, np.newaxis, :], (1, 1, tile_size, tile_size, 1)).astype(np.uint8).reshape(-1)
    packed_deltas = pack_data_numba_wrapper(deltas, deltas_lengths)

    enc_result = dict()
    enc_result["bitlens"] = packed_bitlens
    enc_result["bases"] = bases.astype(np.uint8)
    enc_result["deltas"] = packed_deltas
    

    return enc_result




if __name__ == "__main__":
    image_name = "middle_perspective_image.png"
    # load image
    img = Image.open("../test_data/" + image_name)
    npimage = np.array(img)

    # warm up numba JIT
    enc_result = bd_encoder(npimage, tile_size=4)

    test_time = 10
    t1 = time.time()
    for i in range(test_time):
        enc_result = bd_encoder(npimage, tile_size=4)
    t2 = time.time()
    compressed_size = enc_result["bitlens"].nbytes + enc_result["deltas"].nbytes + enc_result["bases"].nbytes


    compress_rate = 1 - compressed_size / npimage.nbytes
    print ("Compression rate: " + str(compress_rate))
    print ("FPS: " + str(test_time / (t2 - t1)))


    with open('../test_data/enc_result.pkl', 'wb') as pickle_file:
        pickle.dump(enc_result, pickle_file)