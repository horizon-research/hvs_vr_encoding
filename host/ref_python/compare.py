

import numpy as np
from PIL import Image
from base_delta import base_delta
import os


python_img = Image.open("../orig/office_first_frame.bmp")


board_img = np.load("board_office_18_5_5.npy")
board_img = Image.fromarray(board_img.astype(np.uint8))

# board_img = Image.open("office_first_frame.bmp")


size1 = base_delta(python_img, False, False)
size2 = base_delta(board_img, False, False)

orig_sz = os.stat("../orig/office_first_frame.bmp").st_size

print("size1: ", size1)
print("size2: ", size2)


python_img = np.array(python_img)
board_img = np.array(board_img)


# compare the two images, record max and mean error and draw the error map
python_img = python_img.astype(np.float32)
board_img = board_img.astype(np.float32)

error = abs(python_img - board_img)
max_error = error.max()
mean_error = error.mean()
# import ipdb; ipdb.set_trace()
print("max_error: ", max_error)
print("mean_error: ", mean_error)

# normalize error map to 1~255
# import ipdb; ipdb.set_trace()
# import ipdb; ipdb.set_trace()
error_map = error.sum(axis=2)
error_map = error_map / error_map.max() * 255

error_map = error_map.astype(np.uint8)
error_map = Image.fromarray(error_map)
error_map.save("error_map.png")

# use subplot ot draw the python_img, board_img and error map in one img, and title is max err and mean err
import matplotlib.pyplot as plt
fig, ax = plt.subplots(2,2)
# add title
fig.suptitle("max_error: " + str(max_error) + " mean_error: " + str(mean_error))
# make title bigger
fig.subplots_adjust(top=0.88)
ax[0, 0].imshow(python_img.astype(np.uint8))
rate1 = (1 - size1/orig_sz) * 100
rate2 = (1 - size2/orig_sz) * 100
ax[0, 0].set_title("origin_img, BD rate {:.2f} %".format(rate1))
ax[0, 1].imshow(board_img.astype(np.uint8))
ax[0, 1].set_title("board_compressed_img, BD rate {:.2f} %".format(rate2))
ax[1, 0].imshow(error_map)
ax[1, 0].set_title("error_map")

error = error.reshape(-1)
# draw the error distribution
ax[1,1].set_title("error distribution")
ax[1,1].hist(error, bins=100)
plt.savefig("error_distribution.png")

# disable axis
for i in range(2):
    for j in range(2):
        ax[i, j].axis("off")
ax[1, 1].axis("on")
# make subplot bigger
# fig.set_size_inches(18.5, 10.5)

# make margin smaller
fig.tight_layout()

plt.savefig("compare.png")


