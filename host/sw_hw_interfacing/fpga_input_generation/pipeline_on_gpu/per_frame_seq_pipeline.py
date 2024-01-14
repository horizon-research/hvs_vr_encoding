
import sys
import os
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + '/../../..') 
from color_optimizer.red_blue_optimization_cuda import Image_color_optimizer
from projection.equirect_to_pespective_cuda import Equirectangular_to_perspective
from len_correction.len_correction_cuda import Len_correction
from math import radians
import cupy as cp


class Perframe_FPGA_input_generation_pipeline():
    def __init__(self, args):
        self.args = args
        self.projector = Equirectangular_to_perspective(args.h_fov, args.equi_height, args.equi_width, args.perspective_height, args.perspective_width)
        self.image_color_optimizer = Image_color_optimizer(img_height = args.perspective_height, img_width = args.perspective_width, tile_size = args.tile_size,\
                                                            foveated = args.foveated, max_ecc = args.max_ecc, h_fov = args.h_fov, fixed_c = args.fixed_c, ecc_no_compress = args.ecc_no_compress)
        self.tile_size = args.tile_size

        self.img_12_channels = cp.empty((1080, 960, 12), dtype=cp.uint8)

    def __call__(self, equirect_img):

        # step 1: Projection
        perspective_img = self.projector.project(equirect_img = equirect_img, roll =  self.args.roll, pitch =  self.args.pitch, yaw =  self.args.yaw)

        # step 2: get dkl and abc
        dkl_centers, centers_abc = self.image_color_optimizer.only_generate_ellipses(perspective_img)
        # change to 4x4 tile order
        dkl_centers = dkl_centers.reshape(1080 // 4, self.tile_size, 960 // self.tile_size, self.tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(1080, 960, 3)
        centers_abc = centers_abc.reshape(1080 // 4, self.tile_size, 960 // self.tile_size, self.tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(1080, 960, 3)
        

        # step 3: Generate 2K output for FPGA sent through HDMI
        img_6_channels = cp.concatenate((dkl_centers, centers_abc), axis=-1).astype(cp.float16)
        img_int = img_6_channels.view(cp.uint16)

        high_bits = (img_int >> 8).astype(cp.uint8)
        low_bits = (img_int & 0xFF).astype(cp.uint8)

        # import ipdb; ipdb.set_trace()


        self.img_12_channels[:, :, 0::2] = low_bits
        self.img_12_channels[:, :, 1::2] = high_bits


        return self.img_12_channels