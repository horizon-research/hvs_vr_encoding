import sys
sys.path.append('../') 
import cv2
import os
import numpy as np
from pipeline_args import get_args
from tqdm import tqdm

from software_pipeline_per_frame import perframe_color_optimizer_pipeline

if __name__ == '__main__':
    args = get_args()
    if not os.path.exists(args.out_images_folder):
        os.makedirs(args.out_images_folder)
    file_num = len(os.listdir(args.in_images_folder))
    total_frames = file_num
    count = 0
    with tqdm(total=total_frames, desc="Converting Video") as pbar:
        for i in range(file_num):
            in_img_filename = args.in_images_folder + "/" + "frame" + str(i) + ".jpg"
            out_img_filename = args.out_images_folder + "/" + "frame" + str(i) + ".jpg"
            img = cv2.imread(in_img_filename)
            left_corrected_img = perframe_color_optimizer_pipeline(img, args)
            # right_corrected_img = perframe_color_optimizer_pipeline(img, args)
            right_corrected_img = left_corrected_img
            combined_img = np.concatenate((left_corrected_img, right_corrected_img), axis=1)
            pbar.update(1)
            cv2.imwrite(out_img_filename, combined_img.astype(np.uint8))
