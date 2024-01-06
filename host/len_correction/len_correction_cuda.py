import numpy as np
import cupy as cp
import cv2
import time


class len_correction():
    """
    Pre-distort an image to compensate for lens distortion in VR.
    :param image: Input original image.
    :param k1, k2: Radial distortion coefficients.
    :param cx, cy: Optical center of the lens.
    :return: Pre-distorted image.
    """

    def __init__(self, k1, k2, cx, cy, height, width, ppi=401, inch2mm=25.4, z=39.07):
        self.height = height
        self.width = width
        self.ppi = ppi
        self.inch2mm = inch2mm
        self.z = z
        self.transform_const = 1 / ppi * inch2mm / z
        self.transform_const = cp.asarray(self.transform_const, dtype=cp.float32)
        self.update_lens_params(k1, k2, cx, cy)

    
    def update_lens_params(self, k1, k2, cx, cy): # for dynamic lens correction
        self.k1 = cp.asarray(k1, dtype=cp.float32)
        self.k2 = cp.asarray(k2, dtype=cp.float32)
        self.cx = cp.asarray(cx, dtype=cp.float32)
        self.cy = cp.asarray(cy, dtype=cp.float32)

        self.x_grid, self.y_grid = cp.meshgrid(cp.arange(width, dtype=cp.float32), cp.arange(height, dtype=cp.float32))
        self.x_distorted , self.y_distorted = self.get_distorted_idxs()
        self.zero_pixel_idx = cp.where((self.x_distorted > width) | (self.x_distorted < 0) | (self.y_distorted > height) | (self.y_distorted < 0))

    def correct(self, image):
        corrected_image = self.bilinear_interpolate(image, self.y_distorted, self.x_distorted)
        corrected_image[self.zero_pixel_idx] = 0
        return corrected_image
    
    def bilinear_interpolate(self, image, y, x):
        """
        Performs bilinear interpolation for a given set of image points in a vectorized manner.

        :param image: source image
        :param x: array of x coordinates
        :param y: array of y coordinates
        :return: interpolated pixel values
        """
        x1 = cp.floor(x)
        y1 = cp.floor(y)
        x2 = x1 + 1
        y2 = y1 + 1

        # Boundaries check
        x1 = cp.clip(x1, 0, image.shape[1] - 1)
        y1 = cp.clip(y1, 0, image.shape[0] - 1)
        x2 = cp.clip(x2, 0, image.shape[1] - 1)
        y2 = cp.clip(y2, 0, image.shape[0] - 1)

        # Calculate differences
        dx = x - x1
        dy = y - y1
        dx = dx[..., cp.newaxis]  # Add channel dimension
        dy = dy[..., cp.newaxis]
        # import ipdb; ipdb.set_trace()

        # Interpolate
        y1_idx = y1.astype(cp.uint32)
        y2_idx = y2.astype(cp.uint32)
        x1_idx = x1.astype(cp.uint32)
        x2_idx = x2.astype(cp.uint32)

        values = (
                    (image[y1_idx, x1_idx, :] * (1 - dx) * (1 - dy)) +
                    (image[y1_idx, x2_idx, :] * dx * (1 - dy)) +
                    (image[y2_idx, x1_idx, :] * (1 - dx) * dy) +
                    (image[y2_idx, x2_idx, :] * dx * dy)
                )
        
        return values
    
    def get_distorted_idxs(self):
        """
        Apply pre-distortion to a single pixel to compensate for lens distortion.
        :param x, y: Coordinates of the original pixel.
        :param k1, k2: Radial distortion coefficients.
        :param cx, cy: Optical center of the lens (assumed to be the image center).
        :return: Coordinates of the pre-distorted pixel.
        """
        x = (self.x_grid - cx) * self.transform_const
        y = (self.y_grid - cy) * self.transform_const
        r2 = x**2 + y**2
        factor = 1 + k1 * r2 + k2 * r2**2
        x_distorted = x * factor / self.transform_const + cx
        y_distorted = y * factor / self.transform_const + cy

        return x_distorted, y_distorted
    

if __name__ == '__main__':
    # Example usage:
    image_name =  "images/middle_perspective_image.png" # Load your image here
    image = cv2.imread(image_name)
    height, width = image.shape[:2]
    k1, k2 = 0.33582564, 0.55348791 # Radial distortion coefficients
    cx, cy = width / 2, height / 2 # Assuming center of the image is the optical center
    len_correction = len_correction(k1, k2, cx, cy, height, width)
    image = cp.asarray(image, dtype=cp.float32)

    test_times = 500
    t1 = time.time()
    for i in range(test_times):
        len_correction_img = len_correction.correct(image)
    t2 = time.time()
    len_correction_img = cp.asnumpy(len_correction_img)

    print("fps: ", 1/(t2-t1)*test_times)

    cv2.imwrite("images/middle_perspective_image_len_correction.png", len_correction_img.astype(np.uint8))