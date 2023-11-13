"""
Script used to decode an hdf5 base delta data file into the original image that it represents
"""

import numpy as np
from PIL import Image
import h5py

# Takes base delta data from a saved hdf5 file and converts it back into its original image
def base_delta_decode(header, tags, bases, deltas):

    # everything contained within the header
    width = header[0]
    height = header[1]
    r_tag_bitlen = header[2]
    g_tag_bitlen = header[3]
    b_tag_bitlen = header[4]

    decoded_image = np.zeros((height, width, 3), dtype="uint8")
    print(decoded_image.shape)

    # decodes image tile by tile
    for i in range((int)(height/4)):
        for j in range((int)(width/4)):
            # print(i,j)

            # Scan sums used to find location of tile
            r_scanSum = 0
            g_scanSum = 0
            b_scanSum = 0
            for k in range(i):
                r_scanSum += tags[k, (int)(width/4) - 1, 0]
                g_scanSum += tags[k, (int)(width/4) - 1, 1]
                b_scanSum += tags[k, (int)(width/4) - 1, 2]

            r_scanSum += tags[i, j, 0]
            g_scanSum += tags[i, j, 1]
            b_scanSum += tags[i, j, 2]

            # print(r_scanSum, g_scanSum, b_scanSum)

            # bit lengths of deltas within tile
            if j != 0:
                r_bitlen = tags[i, j, 0] - tags[i, j-1, 0]
                g_bitlen = tags[i, j, 1] - tags[i, j-1, 1]
                b_bitlen = tags[i, j, 2] - tags[i, j-1, 2]
            else:
                r_bitlen = tags[i, j, 0]
                g_bitlen = tags[i, j, 1]
                b_bitlen = tags[i, j, 2]

            # print(r_bitlen, g_bitlen, b_bitlen)

            r_scanSum -= r_bitlen
            g_scanSum -= g_bitlen
            b_scanSum -= b_bitlen

            # print(r_scanSum, g_scanSum, b_scanSum)

            # just to show that the individual tile location can be calculated
            n = width/4 * i + j
            tile_loc = (int)(8 * n + 16 * (r_scanSum + g_scanSum + b_scanSum)) # + header + tags
            # print(tile_loc)

            r_base = bases[i, j, 0, 0]
            g_base = bases[i, j, 1, 0]
            b_base = bases[i, j, 2, 0]

            r_deltas = deltas[i, j, 0]
            g_deltas = deltas[i, j, 1]
            b_deltas = deltas[i, j, 2]

            # find the actual color values for every pixel in tile
            rs = r_deltas + r_base
            gs = g_deltas + g_base
            bs = b_deltas + b_base

            # tile = np.stack([rs, gs, bs])
            # print(tile.T)

            # print(tile.shape)
            # print(rs)
            # print(gs)
            # print(bs)

            decoded_image[(i*4):(i*4+4), (j*4):(j*4+4), 0] = rs
            decoded_image[(i*4):(i*4+4), (j*4):(j*4+4), 1] = gs
            decoded_image[(i*4):(i*4+4), (j*4):(j*4+4), 2] = bs

            # print(decoded_image[(i*4):(i*4+4), (j*4):(j*4+4), 0])

    output_image = Image.fromarray(decoded_image, mode="RGB")
    return output_image

def run():

    data_dir = "./Images/BaseDeltaData/"

    image_name = "WaterScape"

    with h5py.File(data_dir + image_name + "_BDdata.hdf5", "r") as f:
        header = f["header"][()]
        tags = f["tags"][()]
        bases = f["bases"][()]
        deltas = f["deltas"][()]

    output_image = base_delta_decode(header, tags, bases, deltas)

    output_image.save("./Images/dec/" + image_name + "_BDdec.bmp")

if __name__ == "__main__":
    run()