from BD_encoder.BD_enc_gpu import bd_encoder as gpu_bd_encoder
from BD_decoder.BD_dec_gpu import bd_decoder as gpu_bd_decoder
import numpy as np
from PIL import Image
import cupy as cp


if __name__ == "__main__":
    image_name = "middle_perspective_image.png"
    # load image
    img = Image.open("./test_data/" + image_name)
    npimage = np.array(img)

    # GPU
    print("GPU version:")
    cpimage = cp.array(npimage)
    enc_result = gpu_bd_encoder(cpimage, tile_size=4)
    img_dec = gpu_bd_decoder(enc_result, tile_size=4)
    if cp.all(img_dec == cpimage):
        print("Success!")

        
    

    

