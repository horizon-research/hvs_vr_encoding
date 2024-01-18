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
    img_heght = 1080
    img_width = 1920
    tile_size = 4
    image = np.random.randint(0, 255, (img_heght, img_width, 3))

    rearranged_image = image.reshape((img_heght//tile_size, tile_size, img_width//tile_size, tile_size, 3)).transpose((0, 2, 1, 3, 4)).reshape((img_heght, img_width, 3))

    
    image = np.asarray(image, dtype=np.uint8)
    save_in_row_order(image, "./TB_data/input.txt")
    save_in_row_order(rearranged_image, "./TB_data/gold_seq.txt")
    