import sys
import os

import cupy as cp

from PIL import Image

import time
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname) 

from pack_gpu import pack_data_cuda_wrapper

import pickle


def bd_encoder(cpimage, tile_size=4):
    # Assuming npimage is your input image with shape (height, width, 3)
    height, width, _ = cpimage.shape

    # Reshape npimage to process 4x4 tiles, output shape (height // 4, width // 4, 4, 4, 3), uint8
    cpimage = cpimage.reshape(height // tile_size, tile_size, width // tile_size, tile_size, 3).transpose(0, 2, 1, 3, 4).astype(cp.float32)

    # Compute min, max, and base for each tile and each color channel, output shape (height // 4, width // 4, 3), uint8
    tiles_min = cpimage.min(axis=(2, 3))
    tiles_max = cpimage.max(axis=(2, 3))
    bases = tiles_min

    # Compute bit lengths, output shape (height // 4, width // 4, 3), uint8
    v_range = tiles_max - tiles_min + 1
    bitlens = cp.zeros_like(v_range, dtype=cp.float32)
    bitlens[v_range > 0] = cp.ceil(cp.log2( v_range[v_range > 0]))
    bitlens = bitlens.astype(cp.uint8)

    # Compute deltas, output shape (height // 4, width // 4, 4, 4, 3), uint8
    deltas = (cpimage - bases[:, :, cp.newaxis, cp.newaxis, :])

    # Pack bitlens
    bitlens_lengths = cp.ones_like(bitlens, dtype=cp.uint8) * 4
    packed_bitlens = pack_data_cuda_wrapper(bitlens.reshape(-1), bitlens_lengths.reshape(-1))


    # Pack deltas
    deltas = deltas.reshape(-1).astype(cp.uint8)
    deltas_lengths = cp.tile(bitlens[:, :, cp.newaxis, cp.newaxis, :], (1, 1, tile_size, tile_size, 1)).astype(cp.uint8).reshape(-1)
    packed_deltas = pack_data_cuda_wrapper(deltas, deltas_lengths)

    enc_result = dict()
    enc_result["bitlens"] = packed_bitlens
    enc_result["bases"] = bases.astype(cp.uint8)
    enc_result["deltas"] = packed_deltas

    return enc_result




if __name__ == "__main__":
    image_name = "middle_perspective_image.png"
    # load image
    img = Image.open("../test_data/" + image_name)
    cpimage = cp.array(img)
    # import ipdb; ipdb.set_trace()

    # warm up numba JIT
    enc_result = bd_encoder(cpimage, tile_size=4)

    test_time = 1000

    # t1 = time.time()
    # for i in range(test_time):
    #     enc_result = bd_encoder(cpimage, tile_size=4)
    # t2 = time.time()

    t1 = time.time()
    for i in range(test_time):
        enc_result = bd_encoder(cpimage, tile_size=4)
    t2 = time.time()
    compressed_size = enc_result["bitlens"].nbytes + enc_result["deltas"].nbytes + enc_result["bases"].nbytes

    compress_rate = 1 - compressed_size / cpimage.nbytes
    print ("Compression rate: " + str(compress_rate))
    print ("FPS: " + str(test_time / (t2 - t1)))

    enc_result["bitlens"] = cp.asnumpy(enc_result["bitlens"])
    enc_result["bases"] = cp.asnumpy(enc_result["bases"])
    enc_result["deltas"] = cp.asnumpy(enc_result["deltas"])

    with open('../test_data/enc_result.pkl', 'wb') as pickle_file:
        pickle.dump(enc_result, pickle_file)


