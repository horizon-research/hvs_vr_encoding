"""
This script optimizes and finds base delta compression rate data for 10 sample 1080p images,
data is saved to a csv file
"""

import sys
import os
import pandas as pd
from red_blue_optimization import color_conversion, base_delta

if __name__ == "__main__":

    image_dir = "../1080Samples/"
    opt_dir = "./Images/opt/"
    format = ".bmp"

    origBD_compressions = []
    optBD_compressions = []
    compression_improves = []

    for p in range(10):
        with open("Red_Blue_Opt_Output.txt", 'w') as out:
            sys.stdout = out
            print("Sample " + str(p))

            image_name = "1080_" + str(p)

            color_conversion(image_dir, opt_dir, image_name, format, out)

            orig_sz = os.stat(image_dir + image_name + format).st_size
            origBD_sz = base_delta(image_dir, image_name, format, True, True, out)
            optBD_sz = base_delta(opt_dir, "opt" + image_name, format, True, True, out)

            origBD_compression = (1 - origBD_sz/orig_sz) * 100
            origBD_compressions.append(origBD_compression)
            txt = "{cr:.2f}%"
            print("Sample " + str(p) + " original base delta compression rate:", txt.format(cr=origBD_compression))

            optBD_compression = (1 - optBD_sz/orig_sz) * 100
            optBD_compressions.append(optBD_compression)
            txt = "{cr:.2f}%"
            print("Sample " + str(p) + " optimized base delta compression rate:", txt.format(cr=optBD_compression))

            compression_improve = optBD_compression - origBD_compression
            compression_improves.append(compression_improve)
            txt = "{cr:.2f}%"
            print("Sample " + str(p) + " compression improvement:", txt.format(cr=compression_improve))

            print()

    data = {
        "Orig Comp Rate": origBD_compressions,
        "Opt Comp Rate": optBD_compressions,
        "Comp Improvement": compression_improve
    }
    df = pd.DataFrame(data)
    df.to_csv("1080_Sample_Compression_Data.csv")
