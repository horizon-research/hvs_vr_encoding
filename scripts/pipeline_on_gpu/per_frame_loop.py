import sys
import cv2
import os
import numpy as np
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + '/..') # for args.py
from args import get_args
from tqdm import tqdm

from per_frame_seq_pipeline import Perframe_color_optimizer_pipeline

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

    pygame_drawer = Pygame_drawer(width = args.perspective_width * 2, height = args.perspective_height, display_port = args.display_port)

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



    perframe_color_optimizer_pipeline = Perframe_color_optimizer_pipeline(args)

    repeat_times = 100

    all_t1 = time.time()
    with tqdm(total=total_frames * repeat_times, desc="Running per frame loop") as pbar:
        for i in range(total_frames * repeat_times):
            img_idx = i % total_frames
            img = img_list[img_idx]
            img = cp.asarray(img, dtype=cp.uint8)

            pp_t1 = time.time()
            left_corrected_img = perframe_color_optimizer_pipeline(img)
            right_corrected_img = left_corrected_img
            combined_img = cp.concatenate((left_corrected_img, right_corrected_img), axis=1)
            pp_t2 = time.time()
            pp_acc_time += pp_t2 - pp_t1

            combined_img = cp.asnumpy(combined_img)

            t1 = time.time()
            if args.display:
                pygame_drawer.draw(combined_img)
            # time.sleep(1/30)
            t2 = time.time()

            draw_acc_time += t2 - t1
            
            if args.save_imgs:
                out_img_filename = args.out_images_folder + "/" + "frame" + str(i) + ".jpg"
                cv2.imwrite(out_img_filename, combined_img.astype(np.uint8))

            pbar.update(1)

            # if i % (10*total_frames) == 0:
            #     import ipdb; ipdb.set_trace()
            #     perframe_color_optimizer_pipeline = Perframe_color_optimizer_pipeline(args)

    all_t2 = time.time()

    pygame_drawer.close()

    print("pp_time: ", pp_acc_time)
    print("draw time: ", draw_acc_time)
    print("all time: ", all_t2-all_t1)
    
    print("pp_fps: ", 1/pp_acc_time*total_frames*repeat_times)
    print("draw fps: ", 1/draw_acc_time*total_frames*repeat_times)
    print("overall fps: ", 1/(all_t2-all_t1)*total_frames*repeat_times)



