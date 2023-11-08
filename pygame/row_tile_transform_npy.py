import cv2
import numpy as np
import ipdb

def tile_to_row(img_path, tile_size, output_size):
# img: image to be converted
# tile_size: size of the tile (8x8)
# output_size: size of the output image (2160x3840x3) HxW
# transforme image tile_order to row_direction
    # read image
    # img = cv2.imread(img_path)
    # img = cv2.resize(img, (output_size[1], output_size[0]) ) # W and H   not   H and W

    img = np.load(img_path)

    # split image into tiles
    tiles = []
    for x in range(0, img.shape[0], tile_size[0]):
        for y in range(0, img.shape[1], tile_size[1]):# traverse 3840 side first
            tiles.append(img[x:x+tile_size[0], y:y+tile_size[1]])
    
    # tiles into row-order
    new_img = np.zeros(output_size, dtype=np.uint8)
    row_counter = 0
    col_counter = 0
    for i in range(0, len(tiles)):
        for j in range(0, tile_size[0]):
            for k in range(0, tile_size[1]): # traverse 3840 side first
                new_img[row_counter][col_counter] = tiles[i][j][k]
                if col_counter == output_size[1] -1:
                    col_counter = 0
                    row_counter = row_counter + 1
                else:
                    col_counter = col_counter + 1

    return new_img, img

def row_to_tile(img_path, tile_size, output_size):
# img: image to be converted
# tile_size: size of the tile (8x8)
# output_size: size of the output image (2160x3840x3)
# transforme image row_direction to tile_order
    # read image
    img = cv2.imread(img_path)

    new_img = np.zeros(output_size, dtype=np.uint8)



    tile_row_offset = 0
    tile_col_offset = 0
    tile_row_start = 0
    tile_col_start = 0
    for i in range(0, output_size[0]):
        for j in range(0, output_size[1]): # first traverse 3840 side
            # print(tile_row_start)
            # print(tile_row_offset)
            try:
                new_img[tile_row_start + tile_row_offset][tile_col_start + tile_col_offset] = img[i][j]
            except:
                ipdb.set_trace()
            if tile_col_offset == tile_size[1] -1:
                tile_col_offset = 0
                tile_row_offset = tile_row_offset + 1
            else:
                tile_col_offset = tile_col_offset + 1
            
            if tile_row_offset == tile_size[0]:
                tile_row_offset = 0
                tile_col_offset = 0
                tile_col_start = tile_col_start + tile_size[1]

                if tile_col_start == output_size[1]:
                    tile_row_start = tile_row_start + tile_size[0]
                    tile_col_start = 0
    
    # import ipdb; ipdb.set_trace()

    return new_img

import os

if __name__ == "__main__":
    in_dir = "./hogrider_20s_4k_out"
    out_dir = "./hogrider_20s_4k_out_t2r"
    if not os.path.exists(out_dir):
        os.mkdir(out_dir)
    for i in range(1, 10):
        print(i)
        img_t2r, img_raw = tile_to_row(in_dir + "/"+ str(i) + ".npy", (8,8), (2160, 3840, 3))
        np.save(out_dir + "/"+ str(i) + ".npy", img_t2r)
        # img_r2t = row_to_tile("./Image_Set/"+ str(i) + "_tile_to_row.jpg", (8,8), (2160, 3840, 3))
        # cv2.imwrite("./Image_Set/"+ str(i) + "_row_to_tile.jpg", img_r2t)
        # import ipdb; ipdb.set_trace()
    #     cv2.imwrite("./Image_Set/"+ str(i) + "_tile_to_row.jpg", img)
    # for i in range(1):
    #     print(i)
    #     img = row_to_tile("./Image_Set/"+ str(i) + "_tile_to_row.jpg", (4,4), (2160, 3840, 3))
    #     cv2.imwrite("./Image_Set/"+ str(i) + "_row_to_tile.jpg", img)