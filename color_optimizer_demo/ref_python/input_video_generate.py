from timeit import default_timer as timer
from util.ecc_map import build_foveated_ecc_map
from PIL import Image
import numpy as np

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ

from model import base_color_model
color_model = base_color_model.BaseColorModel([])
color_model.initialize()
color_model.load("./model/model.pth")

import os

from base_delta import base_delta
import cv2

def change_video_to_image(video_path, image_path):
    if not os.path.exists(image_path):
        os.makedirs(image_path)
    cap = cv2.VideoCapture(video_path)
    count = 0
    while cap.isOpened():
        ret, frame = cap.read()
        if ret == True:
            cv2.imwrite(image_path + "/frame%d.jpg" % count, frame)
            count += 1
        else:
            break
    cap.release()
    cv2.destroyAllWindows()

from multiprocessing import Pool
def generate_input_for_images(image_path, input_path, num_workers = 32):
    if not os.path.exists(input_path):
        os.makedirs(input_path)
    # construct list for multi-processing
    filenames = []
    for img_filename in os.listdir(image_path):
        img_filename = image_path + "/" + img_filename
        input_filename = input_path + "/" + img_filename.split("/")[-1][:-4] + ".npy"
        filename = dict()
        filename["img"] = img_filename
        filename["input"] = input_filename
        filenames.append(filename)

    # multi-processing
    # pool = Pool(num_workers)
    # pool.map(generate_input_for_one_image, filenames)

    generate_input_for_one_image(filenames[0])

def generate_input_for_one_image(filename):
    img = Image.open(filename["img"])
    img = np.array(img)

    input_generator = Input_generator()
    input = input_generator.get_input(img)

    np.save(filename["input"], input)

class Input_generator:
    def __init__(self, foveated = True, max_ecc = 100):
        self.img_height = 0
        self.img_width = 0

        self.foveated = foveated
        self.max_ecc = max_ecc

        self.inframe = np.zeros((2160, 3840, 3), dtype=np.uint8)

        self.in_frame_count = 0


    def set_ecc_map(self):
        if (self.foveated):
            self.ecc_map = build_foveated_ecc_map(60, 0, 0, self.max_ecc, self.img_height, self.img_width)
        else:
            self.ecc_map = np.ones((self.img_height, self.img_width, 1)) * self.max_ecc

    def get_input(self, npimage):
        # prepare inputs (image, ecc_map)
        height, width, _ = npimage.shape
        self.img_height = height
        self.img_width = width
        self.set_ecc_map()

        # optimize every 4x4 tile in bounds of image
        for i in range(0, height, 4):
            for j in range(0, width, 4):
                tile = npimage[i:i+4, j:j+4]
                ecc_tile = self.ecc_map[i:i+4,j:j+4]
                self.upate_input(tile, ecc_tile)
        # RGB to BGR
        return self.inframe[:,:,::-1]
    
    def upate_input(self, tile, ecc_tile):
        dkl, abc = self.get_one_tile_parameters(tile, ecc_tile)
        abc = abc.reshape(4,4,3)
        dkl = dkl.reshape(4,4,3)
        # dkl_abs = np.abs(dkl)
        # dkl_sign = np.sign(dkl)

        # dkl_abs *= (1<<15)
        # dkl_abs = np.ceil(dkl_abs)
        # dkl_abs = dkl_abs.astype(np.uint16)
        # dkl = dkl_abs
        # dkl[dkl_sign==-1] = (1<<16) - dkl[dkl_sign==-1]

        # abc *= (1<<16)
        # abc = np.ceil(abc)
        # abc = abc.astype(np.uint16)

        abc = abc.astype(np.float16).view(np.uint16)
        dkl = dkl.astype(np.float16).view(np.uint16)

        seq_abc_dkl = np.zeros((16 * 12), dtype=np.uint8)

        # import ipdb; ipdb.set_trace()

        count = 0
        for i in range(0, 4):
            for j in range(0, 4):
                seq_abc_dkl[count] = abc[i][j][0] & 0b11111111# a1
                count = count + 1
                seq_abc_dkl[count] = abc[i][j][0] >> 8 # a2
                count = count + 1
        for i in range(0, 4):
            for j in range(0, 4):
                seq_abc_dkl[count] = abc[i][j][1] & 0b11111111 # b1
                count = count + 1
                seq_abc_dkl[count] = abc[i][j][1] >> 8  # b2
                count = count + 1

        for i in range(0, 4):
            for j in range(0, 4):
                seq_abc_dkl[count] = abc[i][j][2] & 0b11111111
                count = count + 1
                seq_abc_dkl[count] = abc[i][j][2] >> 8 
                count = count + 1

        for i in range(0, 4):
            for j in range(0, 4):
                seq_abc_dkl[count] = dkl[i][j][0] & 0b11111111
                count = count + 1
                seq_abc_dkl[count] = dkl[i][j][0] >> 8 
                count = count + 1

        for i in range(0, 4):
            for j in range(0, 4):
                seq_abc_dkl[count] = dkl[i][j][1] & 0b11111111
                count = count + 1
                seq_abc_dkl[count] = dkl[i][j][1] >> 8 
                count = count + 1

        #
        for i in range(0, 4):
            for j in range(0, 4):
                seq_abc_dkl[count] = dkl[i][j][2] & 0b11111111
                count = count + 1
                seq_abc_dkl[count] = dkl[i][j][2] >> 8 
                count = count + 1


        for i in range(len(seq_abc_dkl)):
            ii = self.in_frame_count // (3840 * 3)
            jj = (self.in_frame_count % (3840 * 3)) // 3
            kk = (self.in_frame_count % (3840 * 3)) % 3

            self.inframe[ii][jj][kk] = seq_abc_dkl[i]
            self.in_frame_count += 1
    
    def get_one_tile_parameters(self, tile, ecc_tile):
        tile = tile.astype("int16")
        tile = tile.reshape(16,3)
        dkl_centers, centers_abc = self.generate_ellipsoids(tile, ecc_tile)
        return dkl_centers, centers_abc

    def generate_ellipsoids(self, tile, ecc_tile):
        srgb_centers = tile / 255
        rgb_centers = sRGB2RGB(srgb_centers)

        dkl_centers = (RGB2DKL @ rgb_centers.T).T
        centers_abc = color_model.compute_ellipses(srgb_centers, ecc_tile)

        centers_abc *= 5
        centers_abc[centers_abc <= 1e-5] = 1e-5  ## fix devided by zero error and too large inv_square

        centers_abc[:, 2] = 5e-3

        if ecc_tile.mean() < 18:
            centers_abc[:, :] = 1e-5

        return dkl_centers, centers_abc

if __name__ == "__main__":
    # change_video_to_image("./hogrider_20s.mp4", "./images_from_video_1e5")
    generate_input_for_images("./office_img", "./reversed_input_from_office_img", num_workers=128)
    print("done")







