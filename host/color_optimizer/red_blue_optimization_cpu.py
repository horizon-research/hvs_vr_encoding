from timeit import default_timer as timer
from util.ecc_map import build_foveated_ecc_map
from PIL import Image
import numpy as np

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ

RGB2DKL = np.asarray(RGB2DKL, dtype=np.float32)
DKL2RGB = np.asarray(DKL2RGB, dtype=np.float32)
from model import base_color_model as base_color_model


import os

from util.base_delta import base_delta

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



        max_min = np.max(min_p[:, :, self.opt_channel],  axis = 1) # maximum of the minimum points
        min_max = np.min(max_p[:, :, self.opt_channel], axis = 1) # minimum of the maximum points

        col_plane = (max_min + min_max) / 2
        col_plane[col_plane < 0] = 0
        col_plane[col_plane > 1] = 1


        opt_points = np.zeros((dkl_centers.shape), dtype=np.float32)
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
        t_correct = np.array( [ (bound - self.rgb_centers[oob_is][:,col] ) / self.min_vec_rgb[0, col] ], dtype=np.float32).T
        new_p = self.rgb_centers[oob_is] + t_correct * self.min_vec_rgb
        pts[oob_is] = new_p

    # correct out of bound points in red and green directions
    def fix_bounds(self, pts):
        self.correct_bound(pts, np.array(0, dtype=np.float32), self.col1, True)
        self.correct_bound(pts, np.array(0, dtype=np.float32), self.col2, True)
        self.correct_bound(pts, np.array(1, dtype=np.float32), self.col1, False)
        self.correct_bound(pts, np.array(1, dtype=np.float32), self.col2, False)

    # moves color points defined by rgb_centers within their ellipsoids along
    # vector vec until they intersect the given plane, in color direction col
    def converge_on_plane(self, rgb_centers, plane):
        center_line = lambda t: rgb_centers + t * self.min_vec_rgb
        t_plane = np.array([(plane - rgb_centers[:, self.opt_channel]) / self.min_vec_rgb[0, self.opt_channel]], dtype = np.float32).T
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

        self.color_model = base_color_model.BaseColorModel([])
        self.color_model.initialize()
        file_path = os.path.abspath(__file__)
        dirname = os.path.dirname(file_path)
        self.color_model.load(dirname + "/model/model.pth")
        self.color_model.to_eval()

    def optimize_tiles(self, tiles, ecc_tiles):
    #   takes a 4x4 pixel tile from an image and optimizes its colors 
    # along the blue or red direction
    #   ecc_tile given eccentricity
        tiles = tiles.reshape(-1, 16, 3)
        dkl_centers, centers_abc = self.generate_ellipsoids(tiles, ecc_tiles)
            
        ### ========================= Hardware accelerated part Begin ========================= ###
        blue_opt_points = self.hw_tile_optimizer.col_opt(self.color_channel["B"], dkl_centers, centers_abc)
        blue_srgb_pts = (RGB2sRGB(blue_opt_points)*255).round()
        
        red_opt_points = self.hw_tile_optimizer.col_opt(self.color_channel["R"], dkl_centers, centers_abc)
        red_srgb_pts = (RGB2sRGB(red_opt_points)*255).round()
        
        blue_len = self.compute_tile_bitlen(blue_srgb_pts)
        red_len = self.compute_tile_bitlen(red_srgb_pts)

        result_image = np.zeros((blue_srgb_pts.shape), dtype=np.float32)
        
        choose_red_idx = red_len <= blue_len
        result_image[choose_red_idx] = red_srgb_pts[choose_red_idx]
        choose_blue_idx = np.logical_not(choose_red_idx)
        result_image[choose_blue_idx] = blue_srgb_pts[choose_blue_idx]


        return result_image.astype(np.uint8).reshape(-1,4,4,3)

        # return blue_srgb_pts.reshape(-1,4,4,3).astype(cp.uint8)

        ### ========================= Hardware accelerated part End ========================= ###

    def generate_ellipsoids(self, tiles, ecc_tiles):
        srgb_centers = tiles / 255
        rgb_centers = sRGB2RGB(srgb_centers)

        # import ipdb; ipdb.set_trace()
        dkl_centers = (RGB2DKL[np.newaxis, :, :] @ rgb_centers.transpose(0,2,1)).transpose(0,2,1)
        centers_abc = self.color_model.compute_ellipses(srgb_centers, ecc_tiles)

        centers_abc[centers_abc <= 1e-5] = 1e-5  ## fix devided by zero error and too large inv_square
        centers_abc[:, 2] = 1e-3
        centers_abc[np.tile(ecc_tiles.reshape(-1,1) < 15, (1,3))] = 1e-5
        
        return dkl_centers, centers_abc
    
    def compute_tile_bitlen(self, srgb_tile):
        maxes = np.amax(srgb_tile, axis=1)
        mins = np.amin(srgb_tile, axis=1)
        diffs = (maxes - mins)
        bitlens = np.ceil(np.log2(diffs + 1))
        bitlen_sum = np.sum(bitlens, axis=1)
        return bitlen_sum.astype(np.int32)

class Image_color_optimizer:
    def __init__(self, foveated = True, max_ecc = 35, fov=110, dump_io = False, dump_dir = None, img_height = 0, img_width = 0, tile_size = 4):
        self.color_channel = dict()

        self.color_channel["R"] = 0
        self.color_channel["G"] = 1
        self.color_channel["B"] = 2

        self.r_max_vec = np.array([[0.61894476, -0.24312686,  0.62345751]], dtype=np.float32)  
        self.b_max_vec = np.array([[0.14766317, -0.13674196, 0.97936063]], dtype=np.float32) 

        self.Tile_color_optimizer = Tile_color_optimizer(self.color_channel, self.r_max_vec, self.b_max_vec, dump_io, dump_dir)

        self.img_height = img_height
        self.img_width =  img_width
        self.tile_size = tile_size

        self.foveated = foveated
        self.max_ecc = max_ecc
        self.max_ecc = np.asarray(self.max_ecc, dtype=np.float32)
        self.d = 1 / np.tan( fov * np.pi / 180 / 2)
        self.d = np.asarray(self.d, dtype=np.float32)
        self.x = np.linspace(-1, 1, self.img_width, dtype=np.float32)
        self.y = np.linspace(-1, 1, self.img_height, dtype=np.float32)
        self.xx, self.yy = np.meshgrid(self.x, self.y)

    def set_ecc_map(self, gaze_x, gaze_y):
        """
        Parameters
        ----------
        gaze_x: float
                x coordinate of center of gaze. normalized between [-1, 1]
        gaze_y: float
                y coordinate of center of gaze. normalized between [-1, 1]
        """
        dist = np.sqrt( (self.xx - gaze_x) **2  + (self.yy - gaze_y)**2)
        self.ecc_map = np.arctan(dist / self.d)[..., None] * 180 / np.pi
        self.ecc_map[self.ecc_map > self.max_ecc] = self.max_ecc

    def color_conversion(self, npimage, gaze_x = 0, gaze_y = 0):
    # Optimize colors in tiles of given image, and return optimized image
    # Input: npimage: numpy array of image
    # Output: npNewImage: numpy array of optimized image
        # prepare inputs (image, ecc_map)
        if (self.foveated):
            self.set_ecc_map(gaze_x, gaze_y)
        else:
            self.ecc_map = np.ones((self.img_height, self.img_width, 1), dtype= np.float32) * self.max_ecc

        # optimize every 4x4 tile in bounds of image
        # import ipdb; ipdb.set_trace()
        tiles = npimage.reshape( self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(-1, self.tile_size, self.tile_size, 3)
        ecc_tiles = self.ecc_map.reshape(self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size).transpose(0, 2, 1, 3).reshape(-1, self.tile_size, self.tile_size, 3)

        # adjusted output
        npNewImage = self.Tile_color_optimizer.optimize_tiles(tiles, ecc_tiles).reshape( self.img_height // self.tile_size,  self.img_width // self.tile_size, self.tile_size, self.tile_size, 3) \
                .transpose(0, 2, 1, 3, 4).reshape(self.img_height, self.img_width, 3)

        return npNewImage
    
    def only_generate_ellipses(self, npimage, gaze_x = 0, gaze_y = 0):
        if (self.foveated):
            self.set_ecc_map(gaze_x, gaze_y)
        else:
            self.ecc_map = np.ones((self.img_height, self.img_width, 1), dtype= np.float32) * self.max_ecc

        tiles = npimage.reshape( self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(-1, self.tile_size, self.tile_size, 3)
        ecc_tiles = self.ecc_map.reshape(self.img_height // self.tile_size, self.tile_size, self.img_width // self.tile_size, self.tile_size).transpose(0, 2, 1, 3).reshape(-1, self.tile_size, self.tile_size, 3)

        tiles = tiles.reshape(-1, 16, 3)
        dkl_centers, centers_abc = self.Tile_color_optimizer.generate_ellipsoids(tiles, ecc_tiles)

        return dkl_centers, centers_abc


if __name__ == "__main__":
    image_name = "WaterScape.bmp"
    # load image
    img = Image.open("Images/orig/" + image_name)
    img = np.array(img)

    # optimize colors
    image_color_optimizer = Image_color_optimizer(img_height = img.shape[0], img_width = img.shape[1], tile_size = 4)
    img = np.asarray(img, dtype=np.float32)
    opt_img = image_color_optimizer.color_conversion(img)

    img_for_fps = img[:, :960].copy()
    image_color_optimizer = Image_color_optimizer(img_height = img_for_fps.shape[0], img_width = img_for_fps.shape[1], tile_size = 4)
    test_time = 3
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







