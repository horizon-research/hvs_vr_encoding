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
    bases = np.ceil((tiles_max + tiles_min) / 2)

    # Compute bit lengths, output shape (height // 4, width // 4, 3), uint8
    v_range = tiles_max - tiles_min + 1
    bitlens = np.zeros_like(v_range, dtype=np.float32)
    bitlens[v_range > 0] = np.ceil(np.log2( v_range[v_range > 0])) 

    # Compute deltas, output shape (height // 4, width // 4, 4, 4, 3), int8
    deltas = (npimage - bases[:, :, np.newaxis, np.newaxis, :])

    # Update tags, bases, and deltas arrays, output shape (height // 4, width // 4, 3), int32
    tags = np.cumsum(bitlens, axis=1)

    # Calculate tag bit lengths
    # records the bit length necessary to hold the tags for every color channel
    # based on the row with the highest scan sum 
    # int 32
    max_tags = np.max(tags[:, -1, :], axis=0)
    tag_bitlens = np.zeros(3, dtype=np.float32) # r, g, b
    tag_bitlens[max_tags > 0] = np.ceil(np.log2(max_tags[max_tags > 0]))

    # Pack tags
    tag_bitlens_8 = np.zeros((3,4), dtype=np.float32)
    tag_bitlens_8[:, 0] = tag_bitlens[:]
    tag_bitlens_8[:, 1] = tag_bitlens[:] - 8
    tag_bitlens_8[:, 2] = tag_bitlens[:] - 16
    tag_bitlens_8[:, 3] = tag_bitlens[:] - 24
    tag_bitlens_8 = tag_bitlens_8.clip(min=0, max=8)

    tag_bitlens_8 = tag_bitlens_8.astype(np.uint8)

    tags = tags.astype(np.uint32).view(np.uint8).reshape(tags.shape[0], tags.shape[1], tags.shape[2], 4)

    tags_lengths_8 = np.tile(tag_bitlens_8[np.newaxis, np.newaxis, :, :], (tags.shape[0], tags.shape[1], 1, 1))
    packed_tags = pack_data_numba_wrapper(tags.reshape(-1), tags_lengths_8.reshape(-1))

    # Pack deltas
    deltas = deltas.reshape(-1).astype(np.int8).view(np.uint8)
    deltas_lengths = np.tile(bitlens[:, :, np.newaxis, np.newaxis, :], (1, 1, tile_size, tile_size, 1)).astype(np.uint8).reshape(-1)
    packed_deltas = pack_data_numba_wrapper(deltas, deltas_lengths)

    enc_result = dict()
    enc_result["tags"] = packed_tags
    enc_result["tags_bitlens"] = tag_bitlens.astype(np.uint8)
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
    compressed_size = enc_result["tags"].nbytes + enc_result["deltas"].nbytes + enc_result["bases"].nbytes + enc_result["tags_bitlens"].nbytes


    compress_rate = 1 - compressed_size / npimage.nbytes
    print ("Compression rate: " + str(compress_rate))
    print ("FPS: " + str(test_time / (t2 - t1)))


    with open('../test_data/enc_result.pkl', 'wb') as pickle_file:
        pickle.dump(enc_result, pickle_file)