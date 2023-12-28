import numpy as np
import pandas as pd
import h5py
from PIL import Image


def base_delta(input_image, csv, hdf5):
    # print("base_delta for " + image_name)
    # out.flush()

    # input_image = Image.open(image_dir + image_name + format)

    header = np.zeros((5,), dtype="int32")

    width, height = input_image.size
    header[0] = width
    header[1] = height

    npimage = np.asarray(input_image, dtype="int16")

    data_dir = "./Images/BaseDeltaData/"

    if (csv):
        df = pd.DataFrame([], columns=['i', 'j', 
                                            'r_tag', 'g_tag', 'b_tag',
                                            'r_base', 'g_base', 'b_base',
                                            'r_bitlen', 'g_bitlen', 'b_bitlen',
                                            'r_d0', 'r_d1', 'r_d2', 'r_d3', 'r_d4', 'r_d5', 'r_d6', 'r_d7', 'r_d8', 'r_d9', 'r_d10', 'r_d11', 'r_d12', 'r_d13', 'r_d14', 'r_d15',
                                            'g_d0', 'g_d1', 'g_d2', 'g_d3', 'g_d4', 'g_d5', 'g_d6', 'g_d7', 'g_d8', 'g_d9', 'g_d10', 'g_d11', 'g_d12', 'g_d13', 'g_d14', 'g_d15',
                                            'b_d0', 'b_d1', 'b_d2', 'b_d3', 'b_d4', 'b_d5', 'b_d6', 'b_d7', 'b_d8', 'b_d9', 'b_d10', 'b_d11', 'b_d12', 'b_d13', 'b_d14', 'b_d15'])
        df.to_csv(data_dir + "csv/" + image_name + "_BDdata.csv")

    tags = np.zeros(((int)(height/4), (int)(width/4), 3), dtype="int32")
    bases = np.zeros(((int)(height/4), (int)(width/4), 3, 2), dtype="uint8")
    deltas = np.zeros(((int)(height/4), (int)(width/4), 3, 4, 4), dtype="int8")

    tile_data = np.zeros(((int)(height/4),59)).astype(int)

    # gets tag, base, and delta data for every 4x4 tile in image
    # tags are based on scan sums of delta bit lengths for every row of tiles
    # if they were scan sums for the whole image the tags would be too large
    for i in range(0, height, 4):
        r_scanSum = 0
        g_scanSum = 0
        b_scanSum = 0
        for j in range(0, width, 4):

            if (i+4 <= height and j+4 <= width):
                # print(i,j)

                tile = npimage[i:i+4, j:j+4]
                r = tile[:, :, 0]
                g = tile[:, :, 1]
                b = tile[:, :, 2]    

                r_max = np.max(r)
                r_min = np.min(r)

                g_max = np.max(g)
                g_min = np.min(g)

                b_max = np.max(b)
                b_min = np.min(b)

                r_base = np.floor((r_max + r_min + 1)/2).astype(int)
                g_base = np.floor((g_max + g_min + 1)/2).astype(int)
                b_base = np.floor((b_max + b_min + 1)/2).astype(int)

                r_bitlen = np.ceil(np.log2((r_max - r_base) - (r_min - r_base) + 1)).astype(int)
                g_bitlen = np.ceil(np.log2((g_max - g_base) - (g_min - g_base) + 1)).astype(int)
                b_bitlen = np.ceil(np.log2((b_max - b_base) - (b_min - b_base) + 1)).astype(int) 

                r_deltas = (r - r_base).astype("int8")
                g_deltas = (g - g_base).astype("int8")
                b_deltas = (b - b_base).astype("int8")

                bd_i = (int)(i/4)
                bd_j = (int)(j/4)

                r_scanSum += r_bitlen
                g_scanSum += g_bitlen
                b_scanSum += b_bitlen

                tags[bd_i, bd_j, 0] = r_scanSum
                tags[bd_i, bd_j, 1] = g_scanSum
                tags[bd_i, bd_j, 2] = b_scanSum

                bases[bd_i, bd_j, 0, 0] = r_base.astype("uint8")
                bases[bd_i, bd_j, 1, 0] = g_base.astype("uint8")
                bases[bd_i, bd_j, 2, 0] = b_base.astype("uint8")

                bases[bd_i, bd_j, 0, 1] = r_bitlen.astype("uint8")
                bases[bd_i, bd_j, 1, 1] = g_bitlen.astype("uint8")
                bases[bd_i, bd_j, 2, 1] = b_bitlen.astype("uint8")

                deltas[bd_i, bd_j, 0] = r_deltas
                deltas[bd_i, bd_j, 1] = g_deltas
                deltas[bd_i, bd_j, 2] = b_deltas

                # saves data for each row to csv file if specified
                if (csv):
                    index = (int)(i/4)

                    tile_data[index,0] = i
                    tile_data[index,1] = j

                    tile_data[index,2] = r_scanSum
                    tile_data[index,3] = g_scanSum
                    tile_data[index,4] = b_scanSum

                    tile_data[index,5] = r_base
                    tile_data[index,6] = g_base
                    tile_data[index,7] = b_base

                    tile_data[index,8] = r_bitlen
                    tile_data[index,9] = g_bitlen
                    tile_data[index,10] = b_bitlen

                    tile_data[index,11:27] = r_deltas.flatten()
                    tile_data[index,27:43] = g_deltas.flatten()
                    tile_data[index,43:59] = b_deltas.flatten()

        if (csv):
            df = pd.DataFrame(tile_data)
            df.to_csv(data_dir + "csv/" + image_name + "_BDdata.csv", mode='a', index=False, header=False)
    
    # records the bit length necessary to hold the tags for every color channel
    # based on the row with the highest scan sum 
    if (np.max(tags[:,-1,0]) != 0):
        r_tag_bitlen = np.ceil(np.log2(np.max(tags[:,-1,0])))
    else:
        r_tag_bitlen = 0
    if (np.max(tags[:,-1,1]) != 0):
        g_tag_bitlen = np.ceil(np.log2(np.max(tags[:,-1,1])))
    else:
        g_tag_bitlen = 0
    if (np.max(tags[:,-1,2]) != 0):
        b_tag_bitlen = np.ceil(np.log2(np.max(tags[:,-1,2])))
    else:
        b_tag_bitlen = 0

    header[2] = r_tag_bitlen
    header[3] = g_tag_bitlen
    header[4] = b_tag_bitlen

    # computes total compressed image size based on tags, bases, and deltas
    tag_sum = ((r_tag_bitlen + g_tag_bitlen + b_tag_bitlen) * (int)(height/4) * (int)(width/4))/8
    base_sum = (int)(height/4) * (int)(width/4) * 3
    delta_sum = np.sum(bases[:,:,:,1]*16)/8
    bd_sz = base_sum + tag_sum + delta_sum

    # saves base delta data to hdf5 file if specified
    if (hdf5):
        with h5py.File(data_dir + "hdf5/" + image_name + "_BDdata.hdf5", "w") as f:
            header_dset = f.require_dataset("header", (5,), dtype="int32")
            tags_dset = f.require_dataset("tags", ((int)(height/4), (int)(width/4), 3), dtype="int32")
            bases_dset = f.require_dataset("bases", ((int)(height/4), (int)(width/4), 3, 2), dtype="uint8")
            deltas_dset = f.require_dataset("deltas", ((int)(height/4), (int)(width/4), 3, 4, 4), dtype="int8")

            header_dset[:] = header[:]
            tags_dset[:] = tags[:]
            bases_dset[:] = bases[:]
            deltas_dset[:] = deltas[:]

    return bd_sz