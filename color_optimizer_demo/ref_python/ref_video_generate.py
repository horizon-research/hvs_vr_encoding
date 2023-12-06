import sys
sys.path.append('../../vr_projection') 

from red_blue_optimization import Image_color_optimizer
from equirect_to_fov import equirectangular_to_perspective_with_roll as equirectangular_to_perspective
from pre_distortion import pre_distort_image
from input_video_generate import change_video_to_image

import cv2
import argparse
import os
import numpy as np
from PIL import Image
from multiprocessing import Pool
from math import radians

def ref_image_generate(filename):
    in_img_filename = filename["in"]
    out_img_filename = filename["out"]

    # step 1: Projection
    img = cv2.imread(in_img_filename)
    perspective_img = equirectangular_to_perspective(img, args.fov, 0, radians(90), radians(90), args.perspective_height, args.perspective_width)
    # step 2: Color adjustment
    image_color_optimizer = Image_color_optimizer(dump_io = False, dump_dir = "dump/")
    opt_img = image_color_optimizer.color_conversion(perspective_img)
    # step 3: Pre-distortion
    pre_distorted_img = pre_distort_image(opt_img, args.k1, args.k2, args.cx, args.cy)
    pre_distorted_img = np.tile(pre_distorted_img, (1, 2, 1))
    pre_distorted_img = cv2.resize(pre_distorted_img, (1920, 1080))
    # step 4: Save the perspective image
    cv2.imwrite(out_img_filename, pre_distorted_img.astype(np.uint8))

def ref_images_generate():
    filenames = []
    for img_filename in os.listdir(in_images_path):
        in_img_filename = in_images_path + "/" + img_filename
        out_img_filename = out_images_path + "/" + img_filename
        filename = dict()
        filename["in"] = in_img_filename
        filename["out"] = out_img_filename
        filenames.append(filename)

    # multi-processing
    pool = Pool(args.num_workers)
    pool.map(ref_image_generate, filenames)


def change_image_to_video():
    # step 5: Change the perspective images to video
    fourcc = cv2.VideoWriter_fourcc('m', 'p', '4', 'v')
    out = cv2.VideoWriter(args.out_video_path, fourcc, 60, (1920, 1080))
    file_num = len(os.listdir(out_images_path))
    for i in range(file_num):
        file_name = "frame" + str(i) + ".jpg"
        img = cv2.imread(out_images_path + "/" + file_name)
        out.write(img)
    out.release()

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--in_video_path', type=str, help='path to the 360-video')
    parser.add_argument('--out_video_path', type=str, help='path to the output video')
    parser.add_argument('--k1', type=float, default=0.33582564, help='k1')
    parser.add_argument('--k2', type=float, default=0.55348791, help='k2')
    parser.add_argument('--cx', type=float, default=480, help='cx')
    parser.add_argument('--cy', type=float, default=540, help='cy')
    parser.add_argument('--fov', type=float, default=110.0, help='fov')
    parser.add_argument('--perspective_width', type=int, default=960, help='perspective_width')
    parser.add_argument('--perspective_height', type=int, default=1080, help='perspective_height')
    parser.add_argument('--num_workers', type=int, default=32, help='num_workers')
    args = parser.parse_args()

    # step 1: make a tmp dir for input image
    in_images_path = "./in_images"
    if not os.path.exists(in_images_path):
        os.makedirs(in_images_path)

    # change_video_to_image(args.in_video_path, in_images_path)

    print("Finish change_video_to_image")

    # step 2: make a tmp dir for output image
    out_images_path = "./out_images"
    if not os.path.exists(out_images_path):
        os.makedirs(out_images_path)


    # ref_images_generate()

    change_image_to_video()


    # remove the tmp dir
    os.system("rm -rf " + in_images_path)
    os.system("rm -rf " + out_images_path)

