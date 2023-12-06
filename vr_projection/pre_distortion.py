import numpy as np
import cv2
import time

def bilinear_interpolate(image, y, x):
    """
    Performs bilinear interpolation for a given image point.

    :param image: source image
    :param x: x coordinate
    :param y: y coordinate
    :return: interpolated pixel value
    """
    x1, y1 = int(x), int(y)
    x2, y2 = x1 + 1, y1 + 1

    # Boundaries check
    if x1 >= image.shape[1]-1 or y1 >= image.shape[0]-1 or x1 < 0 or y1 < 0:
        return 0  # or image[y1, x1] if you don't want to return a zero

    # Calculate differences
    dx, dy = x - x1, y - y1

    # Interpolate using the four nearest pixels
    value = (image[y1, x1] * (1 - dx) * (1 - dy) +
             image[y1, x2] * dx * (1 - dy) +
             image[y2, x1] * (1 - dx) * dy +
             image[y2, x2] * dx * dy)
    return value


def distort_pixel(x, y, k1, k2, cx, cy, width, height):
    """
    Apply pre-distortion to a single pixel to compensate for lens distortion.
    :param x, y: Coordinates of the original pixel.
    :param k1, k2: Radial distortion coefficients.
    :param cx, cy: Optical center of the lens (assumed to be the image center).
    :return: Coordinates of the pre-distorted pixel.
    """
    x = x - cx
    y = y - cy

    ppi = 401
    x = x / ppi * 25.4
    y = y / ppi * 25.4
    z = 39.07

    x = x / z
    y = y / z
    r2 = x**2 + y**2
    factor = 1 + k1 * r2 + k2 * r2**2

    x_distorted = x * factor
    y_distorted = y * factor

    x_distorted = x_distorted * z
    y_distorted = y_distorted * z

    x_distorted = x_distorted / 25.4 * ppi
    y_distorted = y_distorted / 25.4 * ppi

    x_distorted = x_distorted + cx
    y_distorted = y_distorted + cy

    return x_distorted, y_distorted

def pre_distort_image(image, k1, k2, cx, cy):
    """
    Pre-distort an image to compensate for lens distortion in VR.
    :param image: Input original image.
    :param k1, k2: Radial distortion coefficients.
    :param cx, cy: Optical center of the lens.
    :return: Pre-distorted image.
    """
    height, width = image.shape[:2]
    pre_distorted_img = np.zeros_like(image)

    for i in range(width):
        for j in range(height):
            x_distorted, y_distorted = distort_pixel(i, j, k1, k2, cx, cy, width, height)
            if 0 <= x_distorted < width and 0 <= y_distorted < height:
                pre_distorted_img[j, i] = bilinear_interpolate(image, y_distorted, x_distorted)
                # pre_distorted_img[j, i] = image[int(y_distorted), int(x_distorted)]

    return pre_distorted_img


if __name__ == '__main__':
    # Example usage:
    image_name =  "images/middle_perspective_image.png" # Load your image here
    image = cv2.imread(image_name)
    height, width = image.shape[:2]
    k1, k2 = 0.33582564, 0.55348791 # Radial distortion coefficients
    cx, cy = width / 2, height / 2 # Assuming center of the image is the optical center

    # start = time.time()
    # for i in range(10):
    pre_distorted_image = pre_distort_image(image, k1, k2, cx, cy)
    # end = time.time()
    # print("fps: ", 10 / (end - start) )

    output_path = 'images/pre_distorted_image.png'
    pre_distorted_image = np.tile(pre_distorted_image, (1, 2, 1))

    # reshape to 1080 x 1920

    pre_distorted_image = cv2.resize(pre_distorted_image, (1920, 1080))


    cv2.imwrite(output_path, pre_distorted_image.astype(np.uint8))