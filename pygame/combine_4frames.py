# read a mp4 file

import cv2
import numpy as np

vid = cv2.VideoCapture("hogrider_20s_out.mp4")

length = int(vid.get(cv2.CAP_PROP_FRAME_COUNT))

fps = vid.get(cv2.CAP_PROP_FPS)

width = vid.get(cv2.CAP_PROP_FRAME_WIDTH)

height = vid.get(cv2.CAP_PROP_FRAME_HEIGHT)

print("length: ",length," fps: ",fps," width: ",width," height: ",height)


import os
import psutil

# 获取第一个CPU核心的编号
first_core = 0

# 获取当前进程的PID
pid = os.getpid()

# 设置进程的CPU亲和性
psutil.Process(pid).cpu_affinity([first_core])

def insert_to_8x8_tile(tile, in_data, idxs):
    idx1, idx2, idx3 = idxs
    in_data = in_data.reshape(-1)
    in_num = in_data.shape[0]

    # print("in_num: ",in_num)

    for i in range(in_num):
        tile[idx1][idx2][idx3] = in_data[i]
        if idx3 == 2:
            idx3 = 0
            if idx1 == 7:
                idx1 = 0
                idx2 += 1
            else:
                idx1 += 1
        else:
            idx3 += 1

        # print("idx1: ",idx1," idx2: ",idx2," idx3: ",idx3)

    if idx2 > 8:
        print("Error: idx2 > 8")
        exit()
    
    return tile, (idx1, idx2, idx3)



# arrangement in a 8x8x3 tile for a 4x4x3 tile
# a[k] = s_axis_tdata[0+16(k+1)-1:0+16k]; first 16 pixels * 16 a = 32x8
# b[k] = s_axis_tdata[256+16(k+1)-1:256+16k]; = 32x8
# c[k] = s_axis_tdata[512+16(k+1)-1:512+16k]; = 32x8
# centres[k][0] = s_axis_tdata[768+16(k+1)-1:768+16k]; = 32x8
# centres[k][1] = s_axis_tdata[1024+16(k+1)-1:1024+16k]; = 32x8
# centres[k][2] = s_axis_tdata[1280+16(k+1)-1:1280+16k]; = 32x8
# ofil = "hogrider_20s_4k_out.mp4"
# out = cv2.VideoWriter(ofil,cv2.VideoWriter_fourcc(*'mp4v'),fps=30,frameSize=(3840, 2160))

i_dir = "../vid_convert/hogrider_20s.mp4_out"
# compute file len
file_len = len(os.listdir(i_dir))
print("file_len: ",file_len)
assert file_len%4 == 0


o_dir= "./hogrider_20s_4k_out"
if not os.path.exists(o_dir):
    os.mkdir(o_dir)



collected_frames_count = 0
# dkl1, dkl2, abc1, abc2
collected_frames = np.zeros((4,1920,1080,3),dtype=np.uint8)
out_frame_count = 0 


total_out = file_len//4

while out_frame_count < total_out:

    frame = np.load(i_dir+"/" + str(out_frame_count) + "_" + str(collected_frames_count+1) + ".npy")

    collected_frames[collected_frames_count] =np.transpose(frame, (1, 0, 2))
    collected_frames_count += 1

    if collected_frames_count == 4:
        collected_frames_count = 0

        abc1 = collected_frames[2]
        abc2 = collected_frames[3]
        dkl1 = collected_frames[0]
        dkl2 = collected_frames[1]

        out_frame = np.zeros((3840,2160,3),dtype=np.uint8)

        for i in range(0, 1920, 4):
            for j in range(0, 1080, 4):
                out_tile = np.zeros((8,8,3),dtype=np.uint8)

                # a
                a1 = abc1[i:i+4, j:j+4, 0]
                a2 = abc2[i:i+4, j:j+4, 0]

                out_tile, idxs = insert_to_8x8_tile(out_tile, a1, (0,0,0))
                out_tile, idxs = insert_to_8x8_tile(out_tile, a2, idxs)

                # b
                b1 = abc1[i:i+4, j:j+4, 1]
                b2 = abc2[i:i+4, j:j+4, 1]
                out_tile, idxs = insert_to_8x8_tile(out_tile, b1, idxs)
                out_tile, idxs = insert_to_8x8_tile(out_tile, b2, idxs)

                # c
                c1 = abc1[i:i+4, j:j+4, 2]
                c2 = abc2[i:i+4, j:j+4, 2]
                out_tile, idxs = insert_to_8x8_tile(out_tile, c1, idxs)
                out_tile, idxs = insert_to_8x8_tile(out_tile, c2, idxs)

                # d
                d1 = dkl1[i:i+4, j:j+4, 0]
                d2 = dkl2[i:i+4, j:j+4, 0]
                out_tile, idxs = insert_to_8x8_tile(out_tile, d1, idxs)
                out_tile, idxs = insert_to_8x8_tile(out_tile, d2, idxs)

                # k
                k1 = dkl1[i:i+4, j:j+4, 1]
                k2 = dkl2[i:i+4, j:j+4, 1]

                out_tile, idxs = insert_to_8x8_tile(out_tile, k1, idxs)
                out_tile, idxs = insert_to_8x8_tile(out_tile, k2, idxs)

                # l
                l1 = dkl1[i:i+4, j:j+4, 2]
                l2 = dkl2[i:i+4, j:j+4, 2]
                out_tile, idxs = insert_to_8x8_tile(out_tile, l1, idxs)
                out_tile, idxs = insert_to_8x8_tile(out_tile, l2, idxs)

                out_frame[i*2:i*2+8, j*2:j*2+8, :] = out_tile

        out_frame_count += 1

        ofil = o_dir+"/" + str(out_frame_count) + ".npy"

        np.save(ofil, out_frame)
        
        print("out_frame_count: ",out_frame_count)
    








