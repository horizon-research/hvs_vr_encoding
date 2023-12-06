import pre_distortion
import pre_distortion_precompute
import rearrange_for_fpga
import numpy as np
import time


# start = time.time()
# for i in range(100):
#     pre_distortion.preDistortImage(distorted, image)

# end = time.time()

# print("fps: ", 100 / (end - start) )


pre_distortor = pre_distortion_precompute.PreDistortion()


image = np.zeros((1080, 1920, 3), dtype=np.uint8)
distorted = np.zeros((1080, 1920, 3), dtype=np.uint8)

start = time.time()
for i in range(100):
    pre_distortor.preDistortImage(distorted, image)

end = time.time()

print("fps: ", 100 / (end - start) )
pre_distortor.preDistortImage(distorted, image)


# rearrangeForFPGA = rearrange_for_fpga.RearrangeForFPGA()


# image = np.zeros((1080, 1920, 3), dtype=np.uint8)
# rearranged = np.ones((1080, 1920, 5, 4), dtype=np.uint8)

# start = time.time()
# for i in range(100):
#     rearrangeForFPGA.rearrangeForFPGA(rearranged, image)

# end = time.time()

# # print(rearranged)

# print("fps: ", 100 / (end - start) )

