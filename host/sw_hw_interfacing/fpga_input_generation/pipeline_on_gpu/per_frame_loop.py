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

    repeat_times = 10000

    all_t1 = time.time()
    with tqdm(total=total_frames * repeat_times, desc="Running per frame loop") as pbar:
        for i in range(total_frames * repeat_times):
            img_idx = i % total_frames
            img = img_list[img_idx]
            img = cp.asarray(img, dtype=cp.uint8)

            pp_t1 = time.time()
            left_12_channel_img = perframe_FPGA_input_generation_pipeline(img)
            right_12_channel_img = cp.zeros_like(left_12_channel_img)
            combined_img = cp.concatenate((left_12_channel_img, right_12_channel_img), axis=0)
            # import ipdb; ipdb.set_trace()
            combined_img = combined_img.reshape(2160, 3840, 3)
            pp_t2 = time.time()

            combined_img = cp.asnumpy(combined_img)
            pygame_drawer.draw(combined_img)


            draw_acc_time += pp_t2 - pp_t1


            pbar.update(1)


    all_t2 = time.time()

    pygame_drawer.close()

    print("pp_time: ", pp_acc_time)
    print("draw time: ", draw_acc_time)
    print("all time: ", all_t2-all_t1)
    
    print("pp_fps: ", 1/pp_acc_time*total_frames*repeat_times)
    print("draw fps: ", 1/draw_acc_time*total_frames*repeat_times)
    print("overall fps: ", 1/(all_t2-all_t1)*total_frames*repeat_times)



