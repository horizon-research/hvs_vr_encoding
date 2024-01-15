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

if __name__ == '__main__':
    args = get_args()
    if not os.path.exists(args.out_images_folder):
        os.makedirs(args.out_images_folder)
    file_num = len(os.listdir(args.in_images_folder))
    total_frames = file_num

    all_acc_time = 0
    pp_acc_time = 0

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

    repeat_times = 10

    all_t1 = time.time()
    with tqdm(total=total_frames, desc="Running per frame loop") as pbar:
        for i in range(file_num):
            img = img_list[i]

            pp_t1 = time.time()
            left_corrected_img = perframe_color_optimizer_pipeline(img)
            # right_corrected_img = perframe_color_optimizer_pipeline(img, args)
            right_corrected_img = left_corrected_img
            combined_img = np.concatenate((left_corrected_img, right_corrected_img), axis=1)
            combined_img = combined_img.astype(np.uint8)
            combined_img = combined_img[:,:,::-1]# RGB to BGR
            pp_t2 = time.time()
            
            pp_acc_time += pp_t2 - pp_t1

            if args.display:
                out_img_filename = args.out_images_folder + "/" + "frame" + str(i) + ".jpg"
                cv2.imshow('Current Frame', combined_img)
                cv2.waitKey(1) # 1ms  minimal wait time for imshow to work 
            
            if args.save_imgs:
                cv2.imwrite(out_img_filename, combined_img.astype(np.uint8))

            pbar.update(1)

    all_t2 = time.time()

    print("pp_fps: ", 1/pp_acc_time*total_frames)
    print("all fps: ", 1/(all_t2-all_t1)*total_frames)

