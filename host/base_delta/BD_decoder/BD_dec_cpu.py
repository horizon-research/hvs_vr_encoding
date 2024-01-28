import os
import sys

file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname) 

import pickle
from unpack_cpu import unpack_bits_to_int8s, unpack_bits_to_uint8s
import numpy as np

from PIL import Image

def bd_decoder(enc_result, tile_size=4):
    packed_tags = enc_result["tags"]
    packed_deltas = enc_result["deltas"]
    bases = enc_result["bases"]
    tag_bitlens = enc_result["tags_bitlens"]

    img_height = bases.shape[0] * tile_size
    img_width = bases.shape[1] * tile_size

    # Unpack tags to get deltas bit lengths
    tag_bitlens_8 = np.zeros((3,4), dtype=np.float32)
    tag_bitlens = tag_bitlens.astype(np.float32)
    tag_bitlens_8[:, 0] = tag_bitlens[:]
    tag_bitlens_8[:, 1] = tag_bitlens[:] - 8
    tag_bitlens_8[:, 2] = tag_bitlens[:] - 16
    tag_bitlens_8[:, 3] = tag_bitlens[:] - 24
    tag_bitlens_8 = tag_bitlens_8.clip(min=0, max=8)
    tag_bitlens_8 = tag_bitlens_8.astype(np.uint8)
    tags_lengths_8 = np.tile(tag_bitlens_8[np.newaxis, np.newaxis, :, :], (bases.shape[0], bases.shape[1], 1, 1))
    unpacked_tags = unpack_bits_to_uint8s(packed_tags.reshape(-1), tags_lengths_8.reshape(-1))
    unpacked_tags = unpacked_tags.view(np.uint32).reshape(bases.shape[0], bases.shape[1], 3)

    start = np.roll(unpacked_tags, 1, axis=1)
    start[:, 0, :] = 0
    bitlens = unpacked_tags - start

    # Unpack deltas
    deltas_lengths = np.tile(bitlens[:, :, np.newaxis, np.newaxis, :], (1, 1, tile_size, tile_size, 1)).astype(np.uint8).reshape(-1)
    unpacked_deltas = unpack_bits_to_int8s(packed_deltas, deltas_lengths)

    # Compute image
    deltas = unpacked_deltas.reshape(bases.shape[0], bases.shape[1], tile_size, tile_size, 3).view(np.int8) 

    img = deltas + bases[:, :, np.newaxis, np.newaxis, :]
    img = img.transpose(0, 2, 1, 3, 4)
    img = img.reshape(img_height, img_width, 3).astype(np.uint8)

    return img


if __name__ == "__main__":
    filename = 'test_pkl_result/enc_result.pkl'
    with open(filename, 'rb') as file:
        enc_result = pickle.load(file)
    img = bd_decoder(enc_result, tile_size=4)

    img = Image.fromarray(img)
    img.save("test_pkl_result/decoded.png")



    