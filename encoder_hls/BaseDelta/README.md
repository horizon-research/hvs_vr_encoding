# Base Delta Compression Optimization

## The Approach
This compression method hinges on the idea that base delta compression could be optimized if the colors within a given tile compressed using a base delta scheme were closer together. If this were the casem then the number of bits needed to represent the deltas for these tiles would decrease and thus the total size of the compressed image would decrease.

## Base Delta Compression
Base Delta is a compression scheme that compresses an image tile by tile, in testing 4x4 pixel tiles were used. For every such tile, a 'base' is determined for every color channel (in our model this base is the mean between the minimum and maximum values in every channel), and each pixel is then encoded as a 'delta', an offset from the calculated base. Since these deltas will span a smaller range of values than the pixel's original 8 bit encodings (for each color channel), a certain degree of compression can be achieved. The degree that the number of bits per pixel is reduced, then, is constrained by how close to the base the min and max deltas are. If this distance can be reduced, then the compression rate can be increased.

## Files in this directory
red_blue_optimization.py is a script that optimizes the tiles within an image to achieve better compression rates using base delta by moving the colors within a given tile closer together within either the red or blue direction. This script also compresses the original and optimized images using base delta, the data for which can be found as csv files and hdf5 files in Images/BaseDeltaData/. The hdf5 files within this directory can also be decoded back into their original images using the base_delta_decoder.py script.

sample_opt_compression_data.py uses red_blue_optimization.py to optimize and compress some sample images. These samples can be found within the Images folder and the results of their compression can be found within the included .csv file in this directory.

### Linear Classifier
Some cursory code for training a classifier to precompute whether a given tile should be optimized along the red or blue direction can be found within the red_blue_classifier.py and red_blue_data_collection.py scripts. This classifier was never optimized and still produced fairly inaccurate results.

## Encoded Images
It should be noted that the images compressed using base delta were not actually saved in the proper bitstream format that would represent how they would actually be compressed, the data for their compressed forms was merely saved as csv files and hdf5 files, from which their compressed size can be calculated.