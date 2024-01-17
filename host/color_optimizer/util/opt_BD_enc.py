import numpy as np

from PIL import Image

import time



def bd_compress_rate(npimage):
    # Assuming npimage is your input image with shape (height, width, 3)
    height, width, _ = npimage.shape

    # Initialize arrays
    tags = np.zeros((height // 4, width // 4, 3), dtype="int32")
    bases = np.zeros((height // 4, width // 4, 3), dtype="uint8")
    bitlens = np.zeros((height // 4, width // 4, 3), dtype="uint8")
    deltas = np.zeros((height // 4, width // 4, 4, 4, 3), dtype="int8")

    # Reshape npimage to process 4x4 tiles
    npimage = npimage.reshape(height // 4, 4, width // 4, 4, 3).transpose(0, 2, 1, 3, 4)

    # Compute min, max, and base for each tile and each color channel
    tiles_min = npimage.min(axis=(2, 3))
    tiles_max = npimage.max(axis=(2, 3))
    bases = np.floor( (tiles_max + tiles_min + 1) / 2)

    # Compute bit lengths
    bitlens = np.ceil(np.log2( (tiles_max - bases) + (bases - tiles_min) + 1 ))

    # Compute deltas
    deltas = (npimage - bases[..., np.newaxis, np.newaxis, :])

    # Update tags, bases, and deltas arrays
    tags = np.cumsum(bitlens, axis=1)

    # Calculate tag bit lengths
    # records the bit length necessary to hold the tags for every color channel
    # based on the row with the highest scan sum 
    max_r_tags = np.max(tags[:,-1,0])
    if (max_r_tags != 0):
        r_tag_bitlen = np.ceil(np.log2(max_r_tags))
    else:
        r_tag_bitlen = 0

    max_g_tags = np.max(tags[:,-1,1])
    if (max_g_tags != 0):
        g_tag_bitlen = np.ceil(np.log2(max_g_tags))
    else:
        g_tag_bitlen = 0

    max_b_tags = np.max(tags[:,-1,2])
    if (max_b_tags != 0):
        b_tag_bitlen = np.ceil(np.log2(max_b_tags))
    else:
        b_tag_bitlen = 0

    # Compute total compressed size
    tag_sum = ((r_tag_bitlen + g_tag_bitlen + b_tag_bitlen) * (height // 4) * (width // 4)) / 8
    base_sum = (height // 4) * (width // 4) * 3
    delta_sum = np.sum(bitlens) / 8 * 16
    compressed_size = base_sum + tag_sum + delta_sum
    orig_size = height * width * 3 

    return 1 - float(compressed_size / orig_size)

# import ipdb; ipdb.set_trace()


if __name__ == "__main__":
    image_name = "WaterScape.bmp"
    # load image
    img = Image.open("../Images/orig/" + image_name)
    npimage = np.array(img)

    test_time = 10
    t1 = time.time()
    for i in range(test_time):
        compress_rate = bd_compress_rate(npimage)

    t2 = time.time()

    print ("Compression rate: " + str(compress_rate))
    print ("FPS: " + str(test_time / (t2 - t1)))

