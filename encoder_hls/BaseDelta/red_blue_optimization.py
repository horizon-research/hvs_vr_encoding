"""
This script optimizes images for base delta compression by moving the colors within 4x4 tiles of an images closer together
in either the red or blue direction, as constrained by the colors' perceptually similar ellipsoids. Every tile is optimized 
in the red and blue direction, and the output tile with the most improved delta bitlengths gets put in the new image.
This script also includes a base delta compressor that spits out the compression rate before and after optimization. Base
delta data from this compressor can be saved to csv or hdf5 files (hdf5 files can be decoder with base_delta_decoder.py).
"""

import os
import sys
import importlib
from pathlib import Path

sys.path.append('..')

from PIL import Image
import numpy as np
from timeit import default_timer as timer
import pandas as pd
from util.ecc_map import *
import h5py

# for virtual machines
# import base_color_model
# color_model = base_color_model.BaseColorModel()
# color_model.initialize()

# for local machines
bcm = importlib.import_module("color_model").get_class("base")
color_model = bcm({})

path = "../io/color_model"
model_fn = "model.pth"
model_root = Path(path)
color_model.load(model_root / model_fn)

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ

# ids for color directions
red_i = 0
green_i = 1
blue_i = 2

# hardcoded constants for eccentricity and ellipsoid c value
ECC = 25        # could be 10, 25, or 35  # used for constant eccentricity, not foveated 
C_VALUE = 1e-3  # adjustable, original: 1e-5

# vectors traversed in process of optimization
# found using optimal_vector_finder.py
# r_max_vec = np.array([[0.8288408400752694, -0.284202178730536, 0.3747391117846977]]) # for c value of 1e-5
# r_max_vec = np.array([[0.8154233, -0.28184743, 0.39376541]])    # for c value of 1e-4
r_max_vec = np.array([[0.61894476, -0.24312686,  0.62345751]])    # for c value of 1e-3
# r_max_vec = np.array([[0.47060249, -0.21153693, 0.76828353]])    # for c value of .002
# r_max_vec = np.array([[0.37671306, -0.19051786, 0.84767728]])   # for c value of .003
r_min_vec = -r_max_vec

# b_max_vec = np.array([[0.19429344437082802, -0.15495947014530864, 0.9632058561267168]]) # for c value of 1e-5
# b_max_vec = np.array([[0.18740741, -0.15250238, 0.96625566]])   # for c value of 1e-4
b_max_vec = np.array([[0.14766317, -0.13674196, 0.97936063]])   # for c value of 1e-3
# b_max_vec = np.array([[0.13928518, -0.13280315, 0.98128449]])   # for c value of .002
# b_max_vec = np.array([[0.13681531, -0.13153875, 0.9818185 ]])   # for c value of .003
b_min_vec = -b_max_vec


dump_count = 0
dump_abc_dkl = np.zeros([129600, 2, 16, 3]).astype(np.float64)
dump_blue_srgb = np.zeros([129600, 16, 3]).astype(np.uint8)

# takes a 4x4 pixel tile from an image and optimizes its colors either along the 
# red direction or the blue direction
# ecc_tile given for foveated eccentricity
def optimize_tile(tile, ecc_tile, out):

    tile = tile.astype("int16")
    tile = tile.reshape(16,3)

    # orig tile properties #

    # min and max values for each color channel
    maxes = np.amax(tile, axis=0)
    mins = np.amin(tile, axis=0)

    diffs = maxes - mins

    global dump_abc_dkl
    global dump_count
    global dump_blue_srgb
    
    # generate information for dkl ellipsoids - rgb_centers, dkl_centers, and defining A matrices
    def generate_ellipsoids():
        srgb_centers = tile/255
        rgb_centers = sRGB2RGB(srgb_centers)
        dkl_centers = (RGB2DKL @ rgb_centers.T).T

        # ecc_tile = np.full((tile.shape[0],1),ECC, dtype=float)    # uncomment to use constant eccentricity
        centers_abc = color_model.compute_ellipses(srgb_centers, ecc_tile)
        zero_pad = np.zeros_like(centers_abc[:,0])
        a_lens = centers_abc[:,0]
        b_lens = centers_abc[:,1]
        # c_lens = centers_abc[:,2]*5
        c_lens = np.ones_like(centers_abc[:,2]) * C_VALUE

        a_inv_squ = np.divide(np.ones_like(a_lens), a_lens**2, out=np.zeros_like(a_lens), where=a_lens!=0)
        b_inv_squ = np.divide(np.ones_like(b_lens), b_lens**2, out=np.zeros_like(b_lens), where=b_lens!=0) 
        c_inv_squ = np.divide(np.ones_like(c_lens), c_lens**2, out=np.zeros_like(c_lens), where=c_lens!=0)

        a_vec = np.stack([a_inv_squ, zero_pad, zero_pad]).T
        b_vec = np.stack([zero_pad, b_inv_squ, zero_pad]).T
        c_vec = np.stack([zero_pad, zero_pad, c_inv_squ]).T

        As = np.stack([a_vec, b_vec, c_vec], axis=1)
        
        return rgb_centers, dkl_centers, centers_abc, As
    
    # moves point on line to a certain bound (col x,y,z) 
    # floor: if true correct points below bound, if false correct points above bound
    def correct_bound(pts, col, bound, floor, centers, lmn):
        if (floor):
            oob_is = np.where(pts[:,col] < bound)[0]
        else:
            oob_is = np.where(pts[:,col] > bound)[0]

        if (len(oob_is) > 0):
            correct_line = lambda t: centers[oob_is] + t[:,0]*lmn
            t_correct = np.array([[(bound - centers[oob_is][:,col])/lmn[0,col]]]).T
            new_p = correct_line(t_correct)
            where_NaN = np.any(np.isnan(new_p), axis=1)
            new_p[where_NaN] = centers[oob_is][where_NaN]
            temp_p = new_p[~where_NaN]
            temp_p[:,col] = bound
            new_p[~where_NaN] = temp_p
            pts[oob_is] = new_p

    # correct out of bound points in red and green directions
    def fix_bounds(pts, col1, col2, centers, lmn):
        correct_bound(pts, col1, 0, True, centers, lmn)
        correct_bound(pts, col2, 0, True, centers, lmn)
        correct_bound(pts, col1, 1, False, centers, lmn)
        correct_bound(pts, col2, 1, False, centers, lmn)

    # find point where line and DKL ellipse intersect
    # rgb: if true return point in rgb
    def line_ell_inter(ell_center, lmn, A):

        line = lambda t: ell_center + t[:,0]*lmn

        t = np.zeros((A.shape[0],1,1))
        roots = np.sqrt(lmn @ A @ lmn.T)
        zero_root = np.where(roots == 0, True, False)
        t[zero_root] = 0
        t[~zero_root] = 1/roots[~zero_root]
        p = line(t)
        
        rgb_p = (DKL2RGB @ p.T).T
        return rgb_p

    rgb_centers, dkl_centers, centers_abc, As = generate_ellipsoids()

    dump_abc_dkl[dump_count][0] = centers_abc
    dump_abc_dkl[dump_count][1]  = dkl_centers

    # necessary bit lengths for deltas in each color channel
    # if (dkl_centers<0).any():
    #     import ipdb;ipdb.set_trace();
    bitlens = np.ceil(np.log2(diffs + 1)).astype(int)
    if(np.all(bitlens == 0)):
        # print("dump_count"+str(dump_count))
        # import ipdb;ipdb.set_trace();
        dump_blue_srgb[dump_count] = tile
        dump_count = dump_count + 1
        return tile.reshape(4,4,3)

    # moves color points defined by rgb_centers within their ellipsoids along
    # vector vec until they intersect the given plane, in color direction col
    def converge_on_plane(rgb_centers, plane, col, vec):
        center_line = lambda t: rgb_centers + t[:,0]*vec
        t_plane = np.array([[(plane - rgb_centers[:,col])/vec[0,col]]]).T
        plane_centers = center_line(t_plane)
        plane_centers[:,col] = plane
        
        return plane_centers
    
    # optimizes the colors in the tile along a given color direction col_i
    def col_opt(col_i):

        if (col_i == red_i):
            max_vec = r_max_vec
            min_vec = r_min_vec
            col1 = green_i
            col2 = blue_i
        elif (col_i == blue_i):
            max_vec = b_max_vec
            min_vec = b_min_vec
            col1 = red_i
            col2 = green_i

        opt_points = np.zeros((tile.shape))

        # only optimize points if they don't already have the same value in the given color channel 
        if (bitlens[col_i] > 0):

            # find minimum points in the colors' ellipsoids along min_vec
            min_lmn = (RGB2DKL @ (min_vec).T).T
            min_p = line_ell_inter(dkl_centers, min_lmn, As)

            # import ipdb; ipdb.set_trace()
            
            fix_bounds(min_p, col1, col2, rgb_centers, min_vec)
            
            # find maximum points in the colors' ellipsoids along max_vec
            max_lmn = (RGB2DKL @ (max_vec).T).T
            max_p = line_ell_inter(dkl_centers, max_lmn, As)

            fix_bounds(max_p, col1, col2, rgb_centers, max_vec)
            
            min_col = max(min_p[:,col_i]) # maximum of the minimum points
            max_col = min(max_p[:,col_i]) # minimum of the maximum points
            
            # if all the mimimums are below 0, converge to the 0 plane
            if (min_col < 0):
                opt_points = converge_on_plane(rgb_centers, 0, col_i, min_vec)
                
            # if all the maximums are above 1, converge to the 1 plane
            elif (max_col > 1):
                opt_points = converge_on_plane(rgb_centers, 1, col_i, min_vec)
                
            else:

                # plane between max min and min max
                col_plane = (min_col + max_col) / 2 
                
                # if max min is less than min max, then col_plane intersects every ellipsoid
                # converge points on col_plane
                if (min_col < max_col):
                    opt_points = converge_on_plane(rgb_centers, col_plane, col_i, min_vec)

                # if man min is less than min max
                # ellipsoids above col_plane move points to their minimums
                # ellipsoids below col_plabe move points to their maximums
                # ellipsoids that intersect col_plane converge on col_plane  
                else:
                    unmod = np.full((tile.shape[0]), True)
                    
                    other_mins = np.where(min_p[:,col_i] > col_plane)
                    opt_points[other_mins] = min_p[other_mins]
                    unmod[other_mins] = False
                    
                    other_maxes = np.where(max_p[:,col_i] < col_plane)
                    opt_points[other_maxes] = max_p[other_maxes]
                    unmod[other_maxes] = False
                    
                    opt_points[unmod] = converge_on_plane(rgb_centers[unmod], col_plane, col_i, min_vec)

        # if colors already have the same value in the given color channel
        else:
            opt_points = rgb_centers

        return opt_points
    
    np.seterr(all='raise')

    # optimize tile in red direction
    red_opt_points = col_opt(red_i)
    red_srgb_pts = (RGB2sRGB(red_opt_points)*255).round().astype("uint8")
    
    # new tile data from red optimization
    r_maxes = np.amax(red_srgb_pts, axis=0)
    r_mins = np.amin(red_srgb_pts, axis=0)
    r_diffs = (r_maxes - r_mins).astype(int)
    try:
        r_bitlens = np.ceil(np.log2(r_diffs + 1)).astype(int)
    except:
        for diff in r_diffs:
            print(np.ceil(np.log2(diff + 1)).astype(int))
        print(r_diffs + 1)
    r_bitlen_sum = sum(r_bitlens)

    # optimize tile in blue direction
    blue_opt_points = col_opt(blue_i)
    blue_srgb_pts = (RGB2sRGB(blue_opt_points)*255).round().astype("uint8")

    # new tile data in blue direction
    b_maxes = np.amax(blue_srgb_pts, axis=0)
    b_mins = np.amin(blue_srgb_pts, axis=0)
    b_diffs = (b_maxes - b_mins).astype(int)
    try:
        b_bitlens = np.ceil(np.log2(b_diffs + 1)).astype(int)
    except:
        for diff in b_diffs:
            print(np.log2(diff + 1))
        print(b_diffs + 1)
    b_bitlen_sum = sum(b_bitlens)

    # return red or blue optimized colors depending on which direction acheived smaller
    # total delta bit lengths

    # import ipdb; ipdb.set_trace()
    
    # np.save("blue_srgb_pts.npy", blue_srgb_pts)
    

    # import ipdb; ipdb.set_trace()


    # print("dump_count"+str(dump_count))
    if (r_bitlen_sum < b_bitlen_sum):
        if (np.any(red_opt_points < 0)):
            print("neg")
            out.flush()

        dump_blue_srgb[dump_count] = red_srgb_pts.copy()
        # print(dump_count)
        dump_count = dump_count + 1
        return red_srgb_pts.reshape(4,4,3)
    else:
        if (np.any(blue_opt_points < 0)):
            print("neg")
            out.flush()
        # print(dump_count)
        dump_blue_srgb[dump_count] = blue_srgb_pts.copy()
        dump_count = dump_count + 1
        return blue_srgb_pts.reshape(4,4,3)
    
    

# Optimize colors in tiles of given image, and save new optimized image
def color_conversion(image_dir, opt_dir, image_name, format, out):

    out.flush()

    input_image = Image.open(image_dir + image_name + format)
    width, height = input_image.size

    npimage = np.asarray(input_image)
    npNewImage = np.empty_like(npimage)

    print(npNewImage.shape)

    # exit(0)
    ecc_map = build_foveated_ecc_map(60,0,0,35,height,width)

    # optimize every 4x4 tile in bounds of image
    totStart = timer()

    global dump_blue_srgb
    count = 0
    raw22 = npNewImage[0:0+4, 88:88+4].copy().reshape(16,3)
    for i in range(0, height, 4):
        for j in range(0, width, 4):
            # print(i, j)
            # out.flush()
            if i%10 == 0 and j==0:
                print(i, j)

            if (i+4 <= height and j+4 <= width):
                tile = npimage[i:i+4, j:j+4]
                ecc_tile = ecc_map[i:i+4,j:j+4]
                npNewImage[i:i+4, j:j+4] = optimize_tile(tile, ecc_tile, out)

                count = count + 1

    end = timer()
    print(end-totStart)
    out.flush()

    out_frame_i = 0
    out_frame_j = 0
    out_frame = np.empty_like(npimage)


    # import ipdb; ipdb.set_trace()
    for ti in range(0, dump_blue_srgb.shape[0]):
        if ti % 100 == 0:
            print(ti / dump_blue_srgb.shape[0])
        # abc_dkl = dump_abc_dkl[ti]
        blue_srgb = dump_blue_srgb[ti].reshape(4,4,3).copy()
        # print(ti)
        out_frame[out_frame_i:out_frame_i+4, out_frame_j:out_frame_j+4, :] = blue_srgb.copy()
        if not (out_frame[out_frame_i:out_frame_i+4, out_frame_j:out_frame_j+4] == npNewImage[out_frame_i:out_frame_i+4, out_frame_j:out_frame_j+4]).all():
            import ipdb; ipdb.set_trace()
        out_frame_j += 4
        if out_frame_j == 1920:
            out_frame_j = 0
            out_frame_i += 4

    output_image = Image.fromarray(out_frame)
    output_image.save(opt_dir + "opt" + image_name + format)

    import ipdb; ipdb.set_trace()




# finds data for image compressed using base delta
# can save data to csv and/or hdf5 file if specified
def base_delta(image_dir, image_name, format, csv, hdf5, out):
    print("base_delta for " + image_name)
    out.flush()

    input_image = Image.open(image_dir + image_name + format)

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
                out.flush()

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

if __name__ == "__main__":
    
    image_dir = "./Images/orig/"
    opt_dir = "./Images/opt/"
    image_name = "WaterScape"
    format = ".bmp"

    with open("Red_Blue_Opt_Output.txt", 'w') as out:
        # sys.stdout = out

        # import ipdb; ipdb.set_trace()

        color_conversion(image_dir, opt_dir, image_name, format, out)

        orig_sz = os.stat(image_dir + image_name + format).st_size
        origBD_sz = base_delta(image_dir, image_name, format, True, True, out)
        optBD_sz = base_delta(opt_dir, "opt" + image_name, format, True, True, out)

        origBD_compression = (1 - origBD_sz/orig_sz) * 100
        txt = "{cr:.2f}%"
        print("original base delta compression rate:", txt.format(cr=origBD_compression))

        optBD_compression = (1 - optBD_sz/orig_sz) * 100
        txt = "{cr:.2f}%"
        print("optimized base delta compression rate:", txt.format(cr=optBD_compression))

        compression_improve = optBD_compression - origBD_compression
        txt = "{cr:.2f}%"
        print("compression improvement:", txt.format(cr=compression_improve))


    np.save("dump_abc_dkl.npy", dump_abc_dkl)
    np.save("dump_blue_srgb.npy", dump_blue_srgb)


