import numpy as np
import cv2
import time

def bilinear_interpolate(image, y, x):
    """
    Performs bilinear interpolation for a given set of image points in a vectorized manner.

    :param image: source image
    :param x: array of x coordinates
    :param y: array of y coordinates
    :return: interpolated pixel values
    """
    x1 = np.floor(x).astype(int)
    y1 = np.floor(y).astype(int)
    x2 = x1 + 1
    y2 = y1 + 1

    # Boundaries check
    x1 = np.clip(x1, 0, image.shape[1] - 1)
    y1 = np.clip(y1, 0, image.shape[0] - 1)
    x2 = np.clip(x2, 0, image.shape[1] - 1)
    y2 = np.clip(y2, 0, image.shape[0] - 1)

    # Calculate differences
    dx = x - x1
    dy = y - y1
    dx = dx[..., np.newaxis]  # Add channel dimension
    dy = dy[..., np.newaxis]
    # import ipdb; ipdb.set_trace()

    # Interpolate
    values = (
                (image[y1, x1, :] * (1 - dx) * (1 - dy)) +
                (image[y1, x2, :] * dx * (1 - dy)) +
                (image[y2, x1, :] * (1 - dx) * dy) +
                (image[y2, x2, :] * dx * dy)
              )

    return values


def distort_pixel(x_grid, y_grid, k1, k2, cx, cy):
    """
    Apply pre-distortion to a single pixel to compensate for lens distortion.
    :param x, y: Coordinates of the original pixel.
    :param k1, k2: Radial distortion coefficients.
    :param cx, cy: Optical center of the lens (assumed to be the image center).
    :return: Coordinates of the pre-distorted pixel.
    """
    ppi = 401
    inch2mm = 25.4
    z = 39.07
    transform_const = 1 / ppi * inch2mm / z

    x = (x_grid - cx) * transform_const
    y = (y_grid - cy) * transform_const
    r2 = x**2 + y**2
    factor = 1 + k1 * r2 + k2 * r2**2
    x_distorted = x * factor / transform_const + cx
    y_distorted = y * factor / transform_const + cy

    return x_distorted, y_distorted

def len_correction(image, k1, k2, cx, cy):
    """
    Pre-distort an image to compensate for lens distortion in VR.
    :param image: Input original image.
    :param k1, k2: Radial distortion coefficients.
    :param cx, cy: Optical center of the lens.
    :return: Pre-distorted image.
    """
    height, width = image.shape[:2]
    len_correction_img = np.zeros_like(image)

    x_grid, y_grid = np.meshgrid(np.arange(width), np.arange(height))
    x_distorted , y_distorted = distort_pixel(x_grid, y_grid, k1, k2, cx, cy)
    
    len_correction_img = bilinear_interpolate(image, y_distorted, x_distorted)

    # import ipdb; ipdb.set_trace()

    len_correction_img[x_distorted > width] = 0
    len_correction_img[x_distorted < 0] = 0
    len_correction_img[y_distorted > height] = 0
    len_correction_img[y_distorted < 0] = 0

    return len_correction_img

if __name__ == '__main__':
    # Example usage:
    image_name =  "images/middle_perspective_image.png" # Load your image here
    image = cv2.imread(image_name)
    height, width = image.shape[:2]
    k1, k2 = 0.33582564, 0.55348791 # Radial distortion coefficients
    cx, cy = width / 2, height / 2 # Assuming center of the image is the optical center
    t1 = time.time()
    len_correction_img = len_correction(image, k1, k2, cx, cy)
    t2 = time.time()

    print("time: ", t2-t1)

    cv2.imwrite("images/middle_perspective_image_len_correction.png", len_correction_img.astype(np.uint8))