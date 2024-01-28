from BD_encoder.BD_enc_cpu import bd_encoder as cpu_bd_encoder
from BD_decoder.BD_dec_cpu import bd_decoder as cpu_bd_decoder
import numpy as np
from PIL import Image

if __name__ == "__main__":
    image_name = "middle_perspective_image.png"
    # load image
    img = Image.open("./test_data/" + image_name)
    npimage = np.array(img)

    # CPU
    print("CPU version:")
    enc_result = cpu_bd_encoder(npimage, tile_size=4)
    img_dec = cpu_bd_decoder(enc_result, tile_size=4)

    if np.all(img_dec == npimage):
        print("Success!")


        
    

    

