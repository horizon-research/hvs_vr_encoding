import sys
import cv2
import os
import numpy as np
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + '/..') # for args.py
from args import get_args
from tqdm import tqdm

from per_frame_seq_pipeline import Perframe_FPGA_input_generation_pipeline

import time
import cupy as cp
from  pygame_drawer import Pygame_drawer

from gui_trial import *

def update_parameters(pipeline):
    slider_values = get_slider_values()
    running = get_running()
    pipeline.args.roll = slider_values["Roll"]
    pipeline.args.pitch = slider_values["Pitch"]
    pipeline.args.yaw = slider_values["Yaw"] 
    if pipeline.args.h_fov != slider_values["Horizontal FOV"]:
        # import ipdb; ipdb.set_trace()   
        pipeline.projector.update_fov(slider_values["Horizontal FOV"])
        pipeline.args.h_fov = slider_values["Horizontal FOV"]

    pipeline.image_color_optimizer.set_abc_scaler(slider_values["abc_scaler"])
    pipeline.image_color_optimizer.set_ecc_no_compress(slider_values["Center FOV"])

    upadate_gradual_filter()

    orignal_right = get_use_uncompressed_image_on_right_eye()

    return running, orignal_right


if __name__ == '__main__':
    args = get_args()
    if not os.path.exists(args.out_images_folder):
        os.makedirs(args.out_images_folder)
    file_num = len(os.listdir(args.in_images_folder))
    total_frames = file_num

    all_acc_time = 0
    pp_acc_time = 0
    loded_time = 0
    draw_acc_time = 0

    pygame_drawer = Pygame_drawer(width = 3840, height = 2160, display_port = args.display_port)

    # pre-load images
    img_list = list()
    with tqdm(total=file_num, desc="Preloading") as pbar:
        for i in range(total_frames):
            in_img_filename = args.in_images_folder + "/" + "frame" + str(i) + ".jpg"
            img = cv2.imread(in_img_filename)
            img_list.append(img)
            pbar.update(1)
    args.equi_width = img_list[0].shape[1]
    args.equi_height = img_list[0].shape[0]



    perframe_FPGA_input_generation_pipeline = Perframe_FPGA_input_generation_pipeline(args)

    with tqdm(leave=False, mininterval=1, bar_format='{rate_fmt}') as pbar:
        _running = True
        while _running:
            i = i % total_frames
            img_idx = i % total_frames
            img = img_list[img_idx]

            root.update_idletasks()
            root.update()

            running, _ = update_parameters(perframe_FPGA_input_generation_pipeline)


            img = cp.asarray(img, dtype=cp.uint8)
            left_12_channel_img = perframe_FPGA_input_generation_pipeline(img)
            right_12_channel_img = cp.zeros_like(left_12_channel_img)
            combined_img = cp.concatenate((left_12_channel_img, right_12_channel_img), axis=0)
            combined_img = combined_img.reshape(2160, 3840, 3)
            combined_img = cp.asnumpy(combined_img)

            pygame_drawer.draw(combined_img)

            pbar.update(1)
            i=i+1

    pygame_drawer.close()



