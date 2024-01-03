from timeit import default_timer as timer
from util.ecc_map import build_foveated_ecc_map
from PIL import Image
import numpy as np

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ

from model import base_color_model


import os

from util.base_delta import base_delta


color_model = base_color_model.BaseColorModel([])
color_model.initialize()
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
color_model.load(dirname + "/model/model.pth")

class Tile_color_optimizer_hw_part:
    def __init__(self, color_channel, r_max_vec, b_max_vec):
        self.color_channel = color_channel
        self.r_max_vec = r_max_vec
        self.b_max_vec = b_max_vec
        self.r_min_vec = -self.r_max_vec
        self.b_min_vec = -self.b_max_vec


        self.max_vec_rgb = None
        self.max_vec_rgb = None

        self.min_vec_dkl = None
        self.max_vec_dkl = None

        self.rgb_centers = None
        self.opt_channel = None
        self.inv_square_abc = None

    # optimizes the colors in the tile along red or blue direction
    def col_opt(self, opt_channel, dkl_centers, centers_abc):
        self.opt_channel = opt_channel
        if (opt_channel == self.color_channel["R"]):
            self.max_vec_rgb = self.r_max_vec
            self.min_vec_rgb = self.r_min_vec
            self.col1 = self.color_channel["G"]
            self.col2 = self.color_channel["B"]
        elif (opt_channel == self.color_channel["B"]):
            self.max_vec_rgb = self.b_max_vec
            self.min_vec_rgb = self.b_min_vec
            self.col1 = self.color_channel["R"]
            self.col2 = self.color_channel["G"]

        self.min_vec_dkl = (RGB2DKL @ (self.min_vec_rgb).T).T * 100
        self.max_vec_dkl = (RGB2DKL @ (self.max_vec_rgb).T).T * 100

        self.rgb_centers = (DKL2RGB @ dkl_centers.transpose(0,2,1)).transpose(0,2,1)
        self.inv_square_abc = 1 / centers_abc**2
        opt_points = self.adjust_tile(dkl_centers)

        return opt_points
    
    def adjust_tile(self, dkl_centers):

        # find minimum points in the colors' ellipsoids along min_vec
        min_p = self.line_ell_inter(dkl_centers, self.min_vec_dkl)
        self.fix_bounds(min_p)
        
        # find maximum points in the colors' ellipsoids along max_vec
        max_p = self.line_ell_inter(dkl_centers, self.max_vec_dkl)
        self.fix_bounds(max_p)

        max_min = np.max(min_p[:, :, self.opt_channel],  axis = 1) # maximum of the minimum points
        min_max = np.min(max_p[:, :, self.opt_channel], axis = 1) # minimum of the maximum points

        col_plane = (max_min + min_max) / 2
        col_plane[col_plane < 0] = 0
        col_plane[col_plane > 1] = 1

        opt_points = np.zeros((dkl_centers.shape))
        # set points with minimum value greater than col_plane to min_p
        # set points with maximum value less than col_plane to max_p

        set_to_min_idx = min_p[:, :, self.opt_channel] >= col_plane[:, np.newaxis]
        opt_points[set_to_min_idx] = min_p[set_to_min_idx]
        set_to_max_idx = max_p[:, :, self.opt_channel] <= col_plane[:, np.newaxis]
        opt_points[set_to_max_idx] = max_p[set_to_max_idx]

        # find points that their ellipsode intersect the plane
        intersect_idx = np.where( np.logical_not( np.logical_or(set_to_min_idx, set_to_max_idx) ) )[0:2]
        opt_points[intersect_idx] = self.converge_on_plane(self.rgb_centers[intersect_idx], col_plane[intersect_idx[0]])

        return opt_points
    
    def line_ell_inter(self, ell_center, _vec):
    # find point where line and DKL ellipse intersect
        t = 1 / np.sqrt( np.sum(_vec**2 * self.inv_square_abc, axis = 1) )
        p = ell_center + np.tile( t.reshape(-1, 16, 1), (1, 1, 3) ) * _vec
        rgb_p = (DKL2RGB @ p.transpose(0,2,1)).transpose(0,2,1)
        return rgb_p
    
    def correct_bound(self, pts, bound, col, floor):
        if (floor):
            oob_is = np.where(pts[:, :,col] < bound)[0:2]
        else:
            oob_is = np.where(pts[:, :,col] > bound)[0:2]
        t_correct = np.array( [ (bound - self.rgb_centers[oob_is][:,col] ) / self.min_vec_rgb[0, col] ] ).T
        new_p = self.rgb_centers[oob_is] + t_correct * self.min_vec_rgb
        pts[oob_is] = new_p

    # correct out of bound points in red and green directions
    def fix_bounds(self, pts):
        self.correct_bound(pts, 0, self.col1, True)
        self.correct_bound(pts, 0, self.col2, True)
        self.correct_bound(pts, 1, self.col1, False)
        self.correct_bound(pts, 1, self.col2, False)

    # moves color points defined by rgb_centers within their ellipsoids along
    # vector vec until they intersect the given plane, in color direction col
    def converge_on_plane(self, rgb_centers, plane):
        center_line = lambda t: rgb_centers + t * self.min_vec_rgb
        t_plane = np.array([(plane - rgb_centers[:, self.opt_channel]) / self.min_vec_rgb[0, self.opt_channel]]).T
        plane_centers = center_line(t_plane)
        return plane_centers

class Tile_color_optimizer:
    def __init__(self, color_channel, r_max_vec, b_max_vec, dump_io = False, dump_dir = None):
        # init the hardware
        self.hw_tile_optimizer = Tile_color_optimizer_hw_part(color_channel, r_max_vec, b_max_vec)
        self.color_channel = color_channel
        self.dump_io = dump_io
        self.dump_dir = dump_dir
        self.dump_id = 0

        self.max_dump_id = 100

    def optimize_tiles(self, tile, ecc_tile):
    #   takes a 4x4 pixel tile from an image and optimizes its colors 
    # along the blue or red direction
    #   ecc_tile given eccentricity
    
        ## prepare FPGA hardware inputs
        tile = tile.astype("int16")
        tile = tile.reshape(-1, 16,3)
        dkl_centers, centers_abc = self.generate_ellipsoids(tile, ecc_tile)
            
        ### ========================= Hardware accelerated part Begin ========================= ###
        blue_opt_points = self.hw_tile_optimizer.col_opt(self.color_channel["B"], dkl_centers, centers_abc)

        blue_srgb_pts = (RGB2sRGB(blue_opt_points)*255).round().astype("uint8")
        
        red_opt_points = self.hw_tile_optimizer.col_opt(self.color_channel["R"], dkl_centers, centers_abc)
        red_srgb_pts = (RGB2sRGB(red_opt_points)*255).round().astype("uint8")
        
        blue_len = self.compute_tile_bitlen(blue_srgb_pts)
        red_len = self.compute_tile_bitlen(red_srgb_pts)

        result_image = np.zeros((blue_srgb_pts.shape))
        
        choose_red_idx = red_len <= blue_len
        result_image[choose_red_idx] = red_srgb_pts[choose_red_idx]
        choose_blue_idx = np.logical_not(choose_red_idx)
        result_image[choose_blue_idx] = blue_srgb_pts[choose_blue_idx]


        return result_image.reshape(-1,4,4,3)

        ### ========================= Hardware accelerated part End ========================= ###

    def generate_ellipsoids(self, tile, ecc_tile):
        srgb_centers = tile / 255
        rgb_centers = sRGB2RGB(srgb_centers)
        # import ipdb; ipdb.set_trace()
        dkl_centers = (RGB2DKL[np.newaxis, :, :] @ rgb_centers.transpose(0,2,1)).transpose(0,2,1)
        centers_abc = color_model.compute_ellipses(srgb_centers, ecc_tile)

        centers_abc[centers_abc <= 1e-5] = 1e-5  ## fix devided by zero error and too large inv_square
        centers_abc[:, 2] = 1e-3

        # centers_abc *= 2

        if ecc_tile.mean() < 15:
            centers_abc[:, :] = 1e-5

        return dkl_centers, centers_abc
    
    def compute_tile_bitlen(self, srgb_tile):
        maxes = np.amax(srgb_tile, axis=1)
        mins = np.amin(srgb_tile, axis=1)
        diffs = (maxes - mins).astype(int)
        bitlens = np.ceil(np.log2(diffs + 1)).astype(int)
        bitlen_sum = np.sum(bitlens, axis=1)
        return bitlen_sum

class Image_color_optimizer:
    def __init__(self, foveated = True, max_ecc = 35, dump_io = False, dump_dir = None):
        self.color_channel = dict()

        self.color_channel["R"] = 0
        self.color_channel["G"] = 1
        self.color_channel["B"] = 2

        self.r_max_vec = np.array([[0.61894476, -0.24312686,  0.62345751]])  
        self.b_max_vec = np.array([[0.14766317, -0.13674196, 0.97936063]]) 

        self.Tile_color_optimizer = Tile_color_optimizer(self.color_channel, self.r_max_vec, self.b_max_vec, dump_io, dump_dir)

        self.img_height = 0
        self.img_width = 0

        self.foveated = foveated
        self.max_ecc = max_ecc

    def set_ecc_map(self):
        if (self.foveated):
            self.ecc_map = build_foveated_ecc_map(110, 0, 0, self.max_ecc, self.img_height, self.img_width)
        else:
            self.ecc_map = np.ones((self.img_height, self.img_width, 1)) * self.max_ecc

    def color_conversion(self, npimage):
    # Optimize colors in tiles of given image, and return optimized image
    # Input: npimage: numpy array of image
    # Output: npNewImage: numpy array of optimized image
        # prepare inputs (image, ecc_map)
        height, width, _ = npimage.shape
        self.img_height = height
        self.img_width = width
        self.set_ecc_map()

        # optimize every 4x4 tile in bounds of image
        np.seterr(all='raise')
        npNewImage = np.empty_like(npimage)

        tile_size = 4


        tiles = npimage.reshape( height // tile_size, tile_size, -1, tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(-1, tile_size, tile_size, 3)
        ecc_tiles = self.ecc_map.reshape(height // tile_size, tile_size, -1, tile_size).transpose(0, 2, 1, 3).reshape(-1, tile_size, tile_size, 3)

        # adjusted output
        # import ipdb; ipdb.set_trace()
        npNewImage = self.Tile_color_optimizer.optimize_tiles(tiles, ecc_tiles).reshape( height // tile_size,  width // tile_size, tile_size, tile_size, 3) \
            .transpose(0, 2, 1, 3, 4).reshape(height, width, 3)

        return npNewImage

if __name__ == "__main__":
    image_name = "WaterScape.bmp"
    # load image
    img = Image.open("Images/orig/" + image_name)
    img = np.array(img)

    # optimize colors
    start = timer()
    image_color_optimizer = Image_color_optimizer()
    opt_img = image_color_optimizer.color_conversion(img)

    end = timer()
    print("Time taken: ", end-start)

    # save image
    opt_img = Image.fromarray(opt_img.astype("uint8"))
    opt_img.save("Images/opt/" + image_name)

    orig_sz = os.stat("Images/orig/" + image_name).st_size
    img = Image.fromarray(img.astype("uint8"))
    origBD_sz = base_delta(img, False, False)
    optBD_sz = base_delta(opt_img, False, False)
    
    origBD_compression = (1 - origBD_sz/orig_sz) * 100
    txt = "{cr:.2f}%"
    print("original base delta compression rate:", txt.format(cr=origBD_compression))

    optBD_compression = (1 - optBD_sz/orig_sz) * 100
    txt = "{cr:.2f}%"
    print("optimized base delta compression rate:", txt.format(cr=optBD_compression))

    compression_improve = optBD_compression - origBD_compression
    txt = "{cr:.2f}%"
    print("compression improvement:", txt.format(cr=compression_improve))


    # np.save("dump/abc_dkls.npy", abc_dkls)







