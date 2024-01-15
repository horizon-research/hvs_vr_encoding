import sys
import os

file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname) 

from timeit import default_timer as timer
from util.ecc_map import build_foveated_ecc_map
from PIL import Image
import numpy as np

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ
import cupy as cp


RGB2DKL = cp.asarray(RGB2DKL, dtype=cp.float32)
DKL2RGB = cp.asarray(DKL2RGB, dtype=cp.float32)
from model import base_color_model_gpu as base_color_model

from util.base_delta import base_delta

def sRGB2RGB_cupy(sRGB):
  sRGB = cp.clip(sRGB, 0, 1)
  lo_mask = sRGB <= 0.04045
  out = cp.zeros_like(sRGB)
  out[lo_mask] = sRGB[lo_mask] / 12.92
  out[~lo_mask] = ((sRGB[~lo_mask] + 0.055) / 1.055) ** 2.4
  return out

def RGB2sRGB_cupy(RGB):
  RGB = cp.clip(RGB, 0, 1)
  lo_mask = RGB<= 0.0031308
  out = cp.zeros_like(RGB)
  out[lo_mask] = 12.92 * RGB[lo_mask]
  out[~lo_mask] = 1.055 * RGB[~lo_mask] ** (1/2.4) - 0.055
  return out

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
        max_p = self.line_ell_inter(dkl_centers, self.max_vec_dkl)

        self.fix_bounds(min_p)
        self.fix_bounds(max_p)



        max_min = cp.max(min_p[:, :, self.opt_channel],  axis = 1) # maximum of the minimum points
        min_max = cp.min(max_p[:, :, self.opt_channel], axis = 1) # minimum of the maximum points

        col_plane = (max_min + min_max) / 2
        col_plane[col_plane < 0] = 0
        col_plane[col_plane > 1] = 1


        opt_points = cp.zeros((dkl_centers.shape), dtype=cp.float32)
        # set points with minimum value greater than col_plane to min_p
        # set points with maximum value less than col_plane to max_p

        set_to_min_idx = min_p[:, :, self.opt_channel] >= col_plane[:, cp.newaxis]
        opt_points[set_to_min_idx] = min_p[set_to_min_idx]
        set_to_max_idx = max_p[:, :, self.opt_channel] <= col_plane[:, cp.newaxis]
        opt_points[set_to_max_idx] = max_p[set_to_max_idx]

        # find points that their ellipsode intersect the plane
        intersect_idx = cp.where( cp.logical_not( cp.logical_or(set_to_min_idx, set_to_max_idx) ) )[0:2]
        opt_points[intersect_idx] = self.converge_on_plane(self.rgb_centers[intersect_idx], col_plane[intersect_idx[0]])

        return opt_points
    
    def line_ell_inter(self, ell_center, _vec):
    # find point where line and DKL ellipse intersect
        t = 1 / cp.sqrt( cp.sum(_vec**2 * self.inv_square_abc, axis = 1) )
        p = ell_center + cp.tile( t.reshape(-1, 16, 1), (1, 1, 3) ) * _vec
        rgb_p = (DKL2RGB @ p.transpose(0,2,1)).transpose(0,2,1)
        return rgb_p

    def correct_bound(self, pts, bound, col, floor):
        if (floor):
            oob_is = cp.where(pts[:, :,col] < bound)[0:2]
        else:
            oob_is = cp.where(pts[:, :,col] > bound)[0:2]
        t_correct = cp.array( [ (bound - self.rgb_centers[oob_is][:,col] ) / self.min_vec_rgb[0, col] ], dtype=cp.float32).T
        new_p = self.rgb_centers[oob_is] + t_correct * self.min_vec_rgb
        pts[oob_is] = new_p

    # correct out of bound points in red and green directions
    def fix_bounds(self, pts):
        self.correct_bound(pts, cp.array(0, dtype=cp.float32), self.col1, True)
        self.correct_bound(pts, cp.array(0, dtype=cp.float32), self.col2, True)
        self.correct_bound(pts, cp.array(1, dtype=cp.float32), self.col1, False)
        self.correct_bound(pts, cp.array(1, dtype=cp.float32), self.col2, False)

    # moves color points defined by rgb_centers within their ellipsoids along
    # vector vec until they intersect the given plane, in color direction col
    def converge_on_plane(self, rgb_centers, plane):
        center_line = lambda t: rgb_centers + t * self.min_vec_rgb
        t_plane = cp.array([(plane - rgb_centers[:, self.opt_channel]) / self.min_vec_rgb[0, self.opt_channel]], dtype = cp.float32).T
        plane_centers = center_line(t_plane)
        return plane_centers

class Tile_color_optimizer:
    def __init__(self, color_channel, r_max_vec, b_max_vec, abc_scaler, ecc_no_compress, only_blue = False):
        # init the hardware
        self.hw_tile_optimizer = Tile_color_optimizer_hw_part(color_channel, r_max_vec, b_max_vec)
        self.color_channel = color_channel
        self.dump_id = 0

        self.max_dump_id = 100

        self.abc_scaler = abc_scaler
        self.ecc_no_compress = ecc_no_compress

        self.only_blue = only_blue

        self.color_model = base_color_model.BaseColorModel([])
        self.color_model.initialize()
        file_path = os.path.abspath(__file__)
        dirname = os.path.dirname(file_path)
        self.color_model.load(dirname + "/model/model.pth")
        self.color_model.to_cuda()
        self.color_model.to_eval()

    def optimize_tiles(self, tiles, ecc_tiles):
    #   takes a 4x4 pixel tile from an image and optimizes its colors 
    # along the blue or red direction
    #   ecc_tile given eccentricity
        tiles = tiles.reshape(-1, 16, 3)
        dkl_centers, centers_abc = self.generate_ellipsoids(tiles, ecc_tiles)
            
        ### ========================= Hardware accelerated part Begin ========================= ###
        blue_opt_points = self.hw_tile_optimizer.col_opt(self.color_channel["B"], dkl_centers, centers_abc)
        blue_srgb_pts = (RGB2sRGB_cupy(blue_opt_points)*255).round()

        if self.only_blue:
            return blue_srgb_pts.astype(cp.uint8).reshape(-1,4,4,3)
        
        red_opt_points = self.hw_tile_optimizer.col_opt(self.color_channel["R"], dkl_centers, centers_abc)
        red_srgb_pts = (RGB2sRGB_cupy(red_opt_points)*255).round()
        
        blue_len = self.compute_tile_bitlen(blue_srgb_pts)
        red_len = self.compute_tile_bitlen(red_srgb_pts)

        result_image = cp.zeros((blue_srgb_pts.shape), dtype=cp.float32)
        
        choose_red_idx = red_len <= blue_len
        result_image[choose_red_idx] = red_srgb_pts[choose_red_idx]
        choose_blue_idx = cp.logical_not(choose_red_idx)
        result_image[choose_blue_idx] = blue_srgb_pts[choose_blue_idx]


        return result_image.astype(cp.uint8).reshape(-1,4,4,3)

        # return blue_srgb_pts.reshape(-1,4,4,3).astype(cp.uint8)

        ### ========================= Hardware accelerated part End ========================= ###

    def generate_ellipsoids(self, tiles, ecc_tiles):
        srgb_centers = tiles / 255
        rgb_centers = sRGB2RGB_cupy(srgb_centers)

        # import ipdb; ipdb.set_trace()
        dkl_centers = (RGB2DKL[cp.newaxis, :, :] @ rgb_centers.transpose(0,2,1)).transpose(0,2,1)
        centers_abc = self.color_model.compute_ellipses_gpu(srgb_centers, ecc_tiles)
        # centers_abc[:, 2] = self.fixed_c
        centers_abc[:, 2] = centers_abc[:, 2] * 0.3
        centers_abc = centers_abc * self.abc_scaler

        centers_abc[centers_abc <= 1e-5] = 1e-5 # fix devided by zero error and too large inv_square_abc
        # import ipdb; ipdb.set_trace()
        centers_abc[cp.tile(ecc_tiles.reshape(-1,1) < self.ecc_no_compress, (1,3))] = 1e-5
        
        return dkl_centers, centers_abc
    
    def compute_tile_bitlen(self, srgb_tile):
        maxes = cp.amax(srgb_tile, axis=1)
        mins = cp.amin(srgb_tile, axis=1)
        diffs = (maxes - mins)
        bitlens = cp.ceil(cp.log2(diffs + 1))
        bitlen_sum = cp.sum(bitlens, axis=1)
        return bitlen_sum.astype(cp.int32)

class Image_color_optimizer:
    def __init__(self, foveated = True, max_ecc = 35, h_fov=110, img_height = 0, img_width = 0, tile_size = 4, abc_scaler = 1e-3, ecc_no_compress = 15, only_blue = False):
        self.color_channel = dict()

        self.color_channel["R"] = 0
        self.color_channel["G"] = 1
        self.color_channel["B"] = 2

        self.r_max_vec = cp.array([[0.61894476, -0.24312686,  0.62345751]], dtype=cp.float32)  
        self.b_max_vec = cp.array([[0.14766317, -0.13674196, 0.97936063]], dtype=cp.float32) 

        self.Tile_color_optimizer = Tile_color_optimizer(self.color_channel, self.r_max_vec, self.b_max_vec, abc_scaler, ecc_no_compress, only_blue)

        self.img_height = img_height
        self.img_width =  img_width
        self.tile_size = tile_size
        self.v_h_ratio = self.img_height / self.img_width

        self.foveated = foveated
        self.max_ecc = max_ecc
        self.max_ecc = cp.asarray(self.max_ecc, dtype=cp.float32)
        self.d = 1 / cp.tan( h_fov * cp.pi / 180 / 2)
        self.d = cp.asarray(self.d, dtype=cp.float32)
        self.x = cp.linspace(-1, 1, self.img_width, dtype=cp.float32)
        self.y = cp.linspace(-1, 1, self.img_height, dtype=cp.float32)
        self.xx, self.yy = cp.meshgrid(self.x, self.y)
    
    def set_abc_scaler(self, abc_scaler):
        self.Tile_color_optimizer.abc_scaler = abc_scaler

    def set_ecc_no_compress(self, ecc_no_compress):
        self.Tile_color_optimizer.ecc_no_compress = ecc_no_compress

    def set_ecc_map(self, gaze_x, gaze_y):
        """
        Parameters
        ----------
        gaze_x: float
                x coordinate of center of gaze. normalized between [-1, 1]
        gaze_y: float
                y coordinate of center of gaze. normalized between [-1, 1]
        """
        dist = cp.sqrt( (self.xx - gaze_x) **2  + ((self.yy - gaze_y) * self.v_h_ratio)  ** 2)
        self.ecc_map = cp.arctan(dist / self.d)[..., None] * 180 / cp.pi
        self.ecc_map[self.ecc_map > self.max_ecc] = self.max_ecc

    def color_conversion(self, cpimage, gaze_x = 0, gaze_y = 0):
    # Optimize colors in tiles of given image, and return optimized image
    # Input: npimage: numpy array of image
    # Output: npNewImage: numpy array of optimized image
        # prepare inputs (image, ecc_map)
        if cpimage.dtype != cp.float32:
            cpimage = cpimage.astype(cp.float32)
        if (self.foveated):
            self.set_ecc_map(gaze_x, gaze_y)
        else:
            self.ecc_map = cp.ones((self.img_height, self.img_width, 1), dtype= cp.float32) * self.max_ecc

        # optimize every 4x4 tile in bounds of image
        # import ipdb; ipdb.set_trace()
        tiles = cpimage.reshape( self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(-1, self.tile_size, self.tile_size, 3)
        ecc_tiles = self.ecc_map.reshape(self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size).transpose(0, 2, 1, 3).reshape(-1, self.tile_size, self.tile_size, 3)

        # adjusted output
        cpNewImage = self.Tile_color_optimizer.optimize_tiles(tiles, ecc_tiles).reshape( self.img_height // self.tile_size,  self.img_width // self.tile_size, self.tile_size, self.tile_size, 3) \
                .transpose(0, 2, 1, 3, 4).reshape(self.img_height, self.img_width, 3)

        return cpNewImage
    
    def only_generate_ellipses(self, cpimage, gaze_x = 0, gaze_y = 0):
        if cpimage.dtype != cp.float32:
            cpimage = cpimage.astype(cp.float32)
        if (self.foveated):
            self.set_ecc_map(gaze_x, gaze_y)
        else:
            self.ecc_map = cp.ones((self.img_height, self.img_width, 1), dtype= cp.float32) * self.max_ecc

        tiles = cpimage.reshape( self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(-1, self.tile_size, self.tile_size, 3)
        ecc_tiles = self.ecc_map.reshape(self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size).transpose(0, 2, 1, 3).reshape(-1, self.tile_size, self.tile_size, 3)

        tiles = tiles.reshape(-1, 16, 3)
        dkl_centers, centers_abc = self.Tile_color_optimizer.generate_ellipsoids(tiles, ecc_tiles)

        dkl_centers = dkl_centers.reshape( self.img_height // self.tile_size,  self.img_width // self.tile_size, self.tile_size, self.tile_size, 3) \
                .transpose(0, 2, 1, 3, 4).reshape(self.img_height, self.img_width, 3)

        centers_abc = centers_abc.reshape( self.img_height // self.tile_size,  self.img_width // self.tile_size, self.tile_size, self.tile_size, 3) \
                .transpose(0, 2, 1, 3, 4).reshape(self.img_height, self.img_width, 3)

        return dkl_centers, centers_abc


if __name__ == "__main__":
    image_name = "WaterScape.bmp"
    # load image
    img = Image.open("Images/orig/" + image_name)
    img = np.array(img)
    img = img[:, :, ::-1] # convert to RGB

    # optimize colors
    image_color_optimizer = Image_color_optimizer(img_height = img.shape[0], img_width = img.shape[1], tile_size = 4)
    img = cp.asarray(img, dtype=cp.float32)
    opt_img = image_color_optimizer.color_conversion(img)

    img_for_fps = img[:, :960].copy()
    image_color_optimizer = Image_color_optimizer(img_height = img_for_fps.shape[0], img_width = img_for_fps.shape[1], tile_size = 4)
    test_time = 150
    t1 = timer()
    for i in range(test_time):
        opt_img_fps = image_color_optimizer.color_conversion(img_for_fps)
    t2 = timer()
    fps = 1/(t2-t1)*test_time

    print(" color optimizer fps: ", fps)

    # generate ellipses given gaze
    t1 = timer()
    for i in range(test_time):
        dkl_centers, centers_abc = image_color_optimizer.only_generate_ellipses(img_for_fps)
    t2 = timer()
    fps = 1/(t2-t1)*test_time

    print(" only_generate_ellipses fps: ", fps)


    opt_img = cp.asnumpy(opt_img)
    img = cp.asnumpy(img)




    # save image
    opt_img = Image.fromarray(opt_img[:,:,::-1].astype("uint8")) # convert to RGB
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







