import numpy as np
import cv2
import time

import os
import sys
file_path = os.path.abspath(__file__)
dirname = os.path.dirname(file_path)
sys.path.append(dirname + "/../../../host/len_correction")
from len_correction_cpu import Len_correction

def save_in_row_order(image, filename):
    height, width = image.shape[:2]
    with open(filename, "w") as f:
        for j in range(0, height, 1):
            for i in range(0, width, 1):
                f.write("{}\n{}\n{}\n".format(image[j, i, 0], image[j, i, 1], image[j, i, 2]))
                


if __name__ == '__main__':
    # Example usage:
    image_name =  "../images/middle_perspective_image.png" # Load your image here
    image = cv2.imread(image_name)
    height, width = image.shape[:2]
    k1, k2 = 0.33582564, 0.55348791 # Radial distortion coefficients
    cx, cy = width / 2, height / 2 # Assuming center of the image is the optical center
    len_correction = Len_correction(k1, k2, cx, cy, height, width, ppi=401, inch2mm=25.4, z=39.07)

    image = np.asarray(image, dtype=np.float32)
    len_correction_img = len_correction.correct(image)

    image = np.asarray(image, dtype=np.uint8)
    save_in_row_order(image, dirname + "/TB_data/input.txt")
    save_in_row_order(len_correction_img, dirname + "/TB_data/gold_seq.txt")
    