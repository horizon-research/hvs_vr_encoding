import numpy as np
import cv2
import time

import os
import sys


def save_in_row_order(image, filename):
    height, width = image.shape[:2]
    with open(filename, "w") as f:
        for j in range(0, height, 1):
            for i in range(0, width, 1):
                f.write("{}\n{}\n{}\n".format(image[j, i, 0], image[j, i, 1], image[j, i, 2]))
                


if __name__ == '__main__':
    # Example usage:
    image_name =  "../images/middle_perspective_image_len_correction.png" # Load your image here
    image = cv2.imread(image_name)
    doubled_image = np.concatenate((image, image), axis=1)

    #save double image 
    doubled_image = doubled_image.astype(np.uint8)
    cv2.imwrite("../images/doubled_image.png", doubled_image)
    
    image = np.asarray(image, dtype=np.uint8)
    save_in_row_order(image, "./TB_data/input.txt")
    save_in_row_order(doubled_image, "./TB_data/gold_seq.txt")
    