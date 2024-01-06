
import sys
sys.path.append('../') 
sys.path.append('../color_optimizer') 
from color_optimizer.red_blue_optimization import Image_color_optimizer
from host.projection.equirect_to_pespective_cpu import equirectangular_to_perspective
from host.len_correction.len_correction_cpu import len_correction
from math import radians

def perframe_color_optimizer_pipeline(equirect_img, args):
    # step 1: Projection
    perspective_img = equirectangular_to_perspective(equirect_img = equirect_img, fov = args.fov, roll = 0, pitch = radians(90), yaw = radians(90), height = args.perspective_height, width = args.perspective_width)

    # step 2: Color optimizer
    image_color_optimizer = Image_color_optimizer()
    opt_img = image_color_optimizer.color_conversion(perspective_img)

    # step 3: Pre-distortion
    corrected_img = len_correction(opt_img, args.k1, args.k2, args.cx, args.cy)

    return corrected_img