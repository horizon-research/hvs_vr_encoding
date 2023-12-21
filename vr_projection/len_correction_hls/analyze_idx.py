import numpy as np
import math

# get bounding box for every row
def get_bounding_box(distort_idx):
    bb = []
    for i in range(distort_idx.shape[0]):
        min_y = None
        max_y = None
        for j in range(distort_idx.shape[1]):
            if  min_y is None or min_y > distort_idx[i][j][0]:
                min_y = distort_idx[i][j][0]
            if max_y is None or max_y < distort_idx[i][j][0]:
                max_y = distort_idx[i][j][0]

        if min_y <= 0:
            min_y = 0
        elif min_y >= distort_idx.shape[0]:
            min_y = distort_idx.shape[0] - 1

        if max_y >= distort_idx.shape[0]:
            max_y = distort_idx.shape[0] - 1
        elif max_y <= 0:
            max_y = 0

        min_y = math.floor(min_y)
        max_y = math.ceil(max_y)

        min_y = min_y + 1
        max_y = max_y + 1

        bb.append([min_y, max_y, i])

    return bb


def get_shift_nums(bb):
    shift_nums = []
    buffer_size = 0
    img_shift = 0
    for i in range(len(bb)):
        min_y = bb[i][0]
        if min_y > img_shift:
            shift = min_y - img_shift
            shift = math.floor(shift / 4) * 4
            shift_nums.append(shift)
            img_shift = img_shift + shift
            buffer_size_needed = bb[i][1] - img_shift + 1 # make sure buffersize is enough
            if buffer_size_needed > buffer_size:
                buffer_size = buffer_size_needed
        else:
            shift_nums.append(0)
    
    if img_shift < 1080:
        shift_nums.append(1080 - img_shift)
    
    buffer_size = math.ceil(buffer_size / 4) * 4
    print("buffer_size: ", buffer_size)

    return shift_nums, buffer_size

def post_process(shift_nums, buffer_size):
    shifts = []
    for shift_num in shift_nums:
        if(buffer_size > 0):
            buffer_size -= shift_num
            shift_num = 0
        assert buffer_size >= 0

        shifts.append(shift_num)
    return shifts





if __name__ == "__main__":  
    distort_idx = np.load("distort_idx.npy")
    print(distort_idx.shape)
    bounding_box = get_bounding_box(distort_idx)
    shift_nums, buffer_size = get_shift_nums(bounding_box)

    shifts = post_process(shift_nums, buffer_size)
    print(shifts)

    shifts = np.array(shifts)
    print(shifts.shape)
    print(shifts.sum() + buffer_size)

    # shift_nums = np.array(shift_nums)
    # print(shift_nums.shape)
    # print(shift_nums.sum())
