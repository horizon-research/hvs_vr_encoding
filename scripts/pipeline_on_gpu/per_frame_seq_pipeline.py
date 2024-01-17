
import sys
import os
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + '/../../host') 
from color_optimizer.red_blue_optimization_cuda import Image_color_optimizer
from projection.equirect_to_pespective_cuda import Equirectangular_to_perspective
from len_correction.len_correction_cuda import Len_correction
from math import radians


class Perframe_color_optimizer_pipeline():
    def __init__(self, args):
        self.args = args
        self.projector = Equirectangular_to_perspective(args.h_fov, args.equi_height, args.equi_width, args.perspective_height, args.perspective_width)
        self.len_correction = Len_correction(args.k1, args.k2, args.cx, args.cy, args.perspective_height, args.perspective_width, \
                                             ppi=args.ppi, z=args.display_distance)
        self.image_color_optimizer = Image_color_optimizer(img_height = args.perspective_height, img_width = args.perspective_width, tile_size = args.tile_size,\
                                                            foveated = args.foveated, max_ecc = args.max_ecc, h_fov = args.h_fov, abc_scaler = args.abc_scaler, ecc_no_compress = args.ecc_no_compress)
        
    def __call__(self, equirect_img):

        # step 1: Projection
        perspective_img = self.projector.project(equirect_img = equirect_img, roll =  self.args.roll, pitch =  self.args.pitch, yaw =  self.args.yaw)
        perspective_img = perspective_img[:,:,::-1]# BGR to RGB
        
        # step 2: Color optimizer
        opt_img = self.image_color_optimizer.color_conversion(perspective_img)
        # opt_img = perspective_img

        # step 3: Pre-distortion
        corrected_orig_img = self.len_correction.correct(perspective_img)
        corrected_opt_img = self.len_correction.correct(opt_img)

        return corrected_opt_img, corrected_orig_img
    

class Perframe_compress_rate_pipeline():
    def __init__(self, args):
        self.args = args
        self.projector = Equirectangular_to_perspective(args.h_fov, args.equi_height, args.equi_width, args.perspective_height, args.perspective_width)
        self.image_color_optimizer = Image_color_optimizer(img_height = args.perspective_height, img_width = args.perspective_width, tile_size = args.tile_size,\
                                                            foveated = args.foveated, max_ecc = args.max_ecc, h_fov = args.h_fov, abc_scaler = args.abc_scaler, \ 
                                                            ecc_no_compress = args.ecc_no_compress, only_blue = True)
        self.equirect_img = None
    def __call__(self):

        if self.equirect_img is None:
            return None

        # step 1: Projection
        perspective_img = self.projector.project(equirect_img = self.equirect_img, roll =  self.args.roll, pitch =  self.args.pitch, yaw =  self.args.yaw)
        perspective_img = perspective_img[:,:,::-1]# BGR to RGB
        
        # step 2: Color optimizer
        opt_img = self.image_color_optimizer.color_conversion(perspective_img)

        # step 3: Compute compression rate
        orig_sz = perspective_img.nbytes

        return self.get_compression_rate()  
    