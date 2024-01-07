import sys
sys.path.append('../') 

from scripts.pipeline_on_cpu.per_frame_seq_pipeline import perframe_color_optimizer_pipeline
import cv2
import os
import numpy as np
from multiprocessing import Pool
from math import radians
from scripts.args import get_args


def muticore_perframe_color_optimizer_pipeline(in_dict):
    in_img_filename = in_dict["input_path"]
    out_img_filename = in_dict["output_path"]
    args = in_dict["args"]

    img = cv2.imread(in_img_filename)
    left_corrected_img = perframe_color_optimizer_pipeline(img, args)
    # right_corrected_img = perframe_color_optimizer_pipeline(img, args)
    right_corrected_img = left_corrected_img
    combined_img = np.concatenate((left_corrected_img, right_corrected_img), axis=1)
    
    cv2.imwrite(out_img_filename, combined_img.astype(np.uint8))


def muticore_multiframes_color_optimizer_pipeline(args):
    in_dicts = []
    file_num = len(os.listdir(args.in_images_folder))
    for i in range(file_num):
        in_img_filename = args.in_images_folder + "/" + "frame" + str(i) + ".jpg"
        out_img_filename = args.out_images_folder + "/" + "frame" + str(i) + ".jpg"
        in_dict = dict()
        in_dict["input_path"] = in_img_filename
        in_dict["output_path"] = out_img_filename
        in_dict["args"] = args
        in_dicts.append(in_dict)

    # multi-processing
    pool = Pool(args.num_workers)
    pool.map(muticore_perframe_color_optimizer_pipeline, in_dicts)


if __name__ == '__main__':
    args = get_args()
    # step 1: make a dir for output images
    if not os.path.exists(args.out_images_folder):
        os.makedirs(args.out_images_folder)

    muticore_multiframes_color_optimizer_pipeline(args)

