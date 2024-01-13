import numpy as np
import cv2
import time
from PIL import Image
import os
import sys

file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + "/../../../host/color_optimizer")
from red_blue_optimization_cpu import Image_color_optimizer

def save_in_row_order(image, filename):
    height, width = image.shape[:2]
    with open(filename, "w") as f:
        for j in range(0, height, 1):
            for i in range(0, width, 1):
                f.write("{}\n{}\n{}\n".format(image[j, i, 0], image[j, i, 1], image[j, i, 2]))

if __name__ == "__main__":
    # load image
    img = Image.open("../imgs/middle_perspective_image.png")
    img = np.array(img)
    img_height, img_width = img.shape[:2]
    tile_size = 4   
    image_color_optimizer = Image_color_optimizer(img_height = img.shape[0], img_width = img.shape[1], tile_size = tile_size)
    
    dkl_centers, centers_abc = image_color_optimizer.only_generate_ellipses(img)
    dkl_centers = dkl_centers.reshape(img_height // tile_size, tile_size, img_width // tile_size, tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(img_height, img_width, 3)
    centers_abc = centers_abc.reshape(img_height // tile_size, tile_size, img_width // tile_size, tile_size, 3).transpose(0, 2, 1, 3, 4).reshape(img_height, img_width, 3)

    save_in_row_order(dkl_centers, "./TB_data/dkl_centers.txt")
    save_in_row_order(centers_abc, "./TB_data/centers_abc.txt")
    
    opt_img = image_color_optimizer.color_conversion(img)
    save_in_row_order(opt_img, "./TB_data/gold_sequence.txt")






