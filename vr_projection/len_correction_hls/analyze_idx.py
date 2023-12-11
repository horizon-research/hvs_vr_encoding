import numpy as np
import math

# get bounding box for every row
def get_bounding_box(distort_idx):
    bb = []
    last_min_y = 0
    last_max_y = 0
    for i in range(distort_idx.shape[0]):
        min_y = None
        max_y = None
        for j in range(distort_idx.shape[1]):
            if distort_idx[i][j][1] >=0 and distort_idx[i][j][1] < 1920 and distort_idx[i][j][0] >=0 and distort_idx[i][j][0] < 1080:
                if  min_y is None or min_y > distort_idx[i][j][0]:
                    min_y = distort_idx[i][j][0]
                if max_y is None or max_y < distort_idx[i][j][0]:
                    max_y = distort_idx[i][j][0]
            if min_y is None or max_y is None:
                continue
            else:
                last_min_y = min_y
                last_max_y = max_y

        if min_y is None or max_y is None:
            bb.append([math.floor(last_min_y), math.ceil(last_max_y), "skip"])
        else:
            bb.append([math.floor(min_y), math.ceil(max_y), "normal"])

    return bb


def get_discard_add_nums(bounding_box):
    discards = []
    adds = []
    for i in range(len(bounding_box)):        
        if i==0:
            discards.append([0, bounding_box[i][2]])
            adds.append([bounding_box[i][1] - bounding_box[i][0] + 1, bounding_box[i][2]])
        else:
            discards.append([bounding_box[i][0] - bounding_box[i-1][0], bounding_box[i][2]])
            adds.append([bounding_box[i][1] - bounding_box[i-1][1], bounding_box[i][2]])

    return discards, adds

def parse_discard_add_nums(discards, adds):
    line_comp = []
    buffersize = 0
    max_buffersize = 0
    for i in range(len(discards)):
        line_info = dict()
        if discards[i][1] == "skip":
            continue
        else:
            buffersize -= discards[i][0]
            buffersize += adds[i][0]
            line_info["discard"] = discards[i][0]
            line_info["add"] = adds[i][0]
            line_info["buffersize"] = buffersize
            line_info["num"] = i
            line_comp.append(line_info)

            if buffersize > max_buffersize:
                max_buffersize = buffersize

    print("max_buffersize: ", max_buffersize)

    return line_comp


if __name__ == "__main__":  
    distort_idx = np.load("distort_idx.npy")
    print(distort_idx.shape)
    bounding_box = get_bounding_box(distort_idx)
    discards, adds = get_discard_add_nums(bounding_box)
    line_comp = parse_discard_add_nums(discards, adds)
    # print(line_comp)