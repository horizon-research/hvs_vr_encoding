from math import tan, radians, cos, sin, pi
import cupy as cp
import numpy as np
import cv2
import matplotlib.pyplot as plt
import time

def bilinear_interpolate(image, y, x):
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

class equirectangular_to_perspective():
    def __init__(self, fov, equi_height, equi_width, out_height, out_width):
        self.out_height = out_height
        self.out_width = out_width
        self.update_equi_dims(equi_height, equi_width)
        self.update_fov(fov)
        self.update_out_dims(out_height, out_width)
        
    def update_equi_dims(self, equi_height, equi_width):
        self.equi_height = equi_height
        self.equi_width = equi_width
        self.v_res = float(equi_height) / pi
        self.h_res = float(equi_width) / (2 * pi)

        self.v_res = cp.asarray(self.v_res, dtype=cp.float32)
        self.h_res = cp.asarray(self.h_res, dtype=cp.float32)

    def update_fov(self, fov):
        self.fov = fov
        self.h_fov = radians(fov)
        self.v_fov = self.h_fov * (float(self.out_height) / float(self.out_width))
        self.h_fov = cp.asarray(self.h_fov, dtype=cp.float32)
        self.v_fov = cp.asarray(self.v_fov, dtype=cp.float32)

    def update_out_dims(self, out_height, out_width):
        self.out_height = out_height
        self.out_width = out_width
        # parrallelized numpy impl
        # Convert perspective pixel coordinates to normalized degrees coordinates
        self.x = cp.linspace(-1, 1, self.out_width, dtype=cp.float32) * cp.tan(self.h_fov / 2, dtype=cp.float32)
        self.y = cp.linspace(1, -1, self.out_height , dtype=cp.float32) * cp.tan(self.v_fov / 2, dtype=cp.float32 )
        self.xp, self.yp = cp.meshgrid(self.x, self.y)
        self.zp = cp.ones_like(self.xp)
        self.vec = cp.array([self.xp, self.yp, self.zp])


    def project(self, equirect_img, roll, pitch, yaw):
        # Calculate the camera rotation matrix
        R_roll = np.array([[1, 0, 0],
                        [0, cos(roll), -sin(roll)],
                        [0, sin(roll), cos(roll)]], dtype=np.float32)

        R_pitch = np.array([[cos(pitch), 0, sin(pitch)],
                            [0, 1, 0],
                            [-sin(pitch), 0, cos(pitch)]], dtype=np.float32)

        R_yaw = np.array([[cos(yaw), -sin(yaw), 0],
                        [sin(yaw), cos(yaw), 0],
                        [0, 0, 1]], dtype=np.float32)

        R = R_roll @ R_pitch @ R_yaw

        R = cp.asarray(R, dtype=cp.float32)

        # Apply the camera rotation to the vector
        rotated_vec = cp.tensordot(R, self.vec, axes=1)

        # Convert 3D coordinates to spherical coordinates
        r = cp.linalg.norm(rotated_vec, axis=0)
        theta_s = cp.arctan2(rotated_vec[1], rotated_vec[0])
        phi_s = cp.arccos(rotated_vec[2] / r)

        # Map the spherical coordinates to equirectangular pixel coordinates
        eq_x = (theta_s + cp.pi) * self.h_res
        eq_y = phi_s * self.v_res


        # Get pixel value from equirectangular image if within bounds
        return bilinear_interpolate(cp.asarray(equirect_img), eq_y, eq_x)



# Now we will use the new function to create the perspective image from the equirectangular image
def draw_cube():
    equirectangular_image = cv2.imread('images/office.png') # replace with the actual path to your equirectangular image
    roll_angles =  [90, 0, -90,   0,   0,   0] # Roll angle
    pitch_angles = [0, 90,   0, -90,   0, 180]  # Pitch angle
    yaw_angles =   [0, 90, 180, 270,  90,  90] # Yaw angle

    # import ipdb; ipdb.set_trace()

    perspective_width = 360
    perspective_height = 360
    fov = 90  # Field of view

    equirectangular_image = equirectangular_image

    project_time = 0

    projector = equirectangular_to_perspective(fov, equirectangular_image.shape[0], equirectangular_image.shape[1], perspective_height, perspective_width)

    output_imgs = []
    for i in range(len(pitch_angles)):
        pitch_angle = pitch_angles[i]
        yaw_angle = yaw_angles[i]
        roll_angle = roll_angles[i]
    
        # Perform the projection from equirectangular to perspective view
        t1 = time.time()
        perspective_image = projector.project(equirectangular_image, radians(roll_angle), radians(pitch_angle), radians(yaw_angle))
        t2 = time.time()
        project_time += t2-t1
        # Save the perspective image
        output_imgs.append(perspective_image)

    # Save the perspective images using cube like 3 x 4 grid
    output_img = cp.ones((perspective_height * 3, perspective_width * 4, 3))
    output_img.fill(255)
    i = 0
    j = 1
    output_img[i * perspective_height:(i + 1) * perspective_height, j * perspective_width:(j + 1) * perspective_width, :] = output_imgs[4]
    i = 1
    j = 0
    output_img[i * perspective_height:(i + 1) * perspective_height, j * perspective_width:(j + 1) * perspective_width, :] = output_imgs[0]
    i = 1
    j = 1
    output_img[i * perspective_height:(i + 1) * perspective_height, j * perspective_width:(j + 1) * perspective_width, :] = output_imgs[1]
    i = 1
    j = 2
    output_img[i * perspective_height:(i + 1) * perspective_height, j * perspective_width:(j + 1) * perspective_width, :] = output_imgs[2]
    i = 1
    j = 3
    output_img[i * perspective_height:(i + 1) * perspective_height, j * perspective_width:(j + 1) * perspective_width, :] = output_imgs[3]
    i = 2
    j = 1
    output_img[i * perspective_height:(i + 1) * perspective_height, j * perspective_width:(j + 1) * perspective_width, :] = output_imgs[5]

    output_path = 'images/cube_perspective_image.png'
    cv2.imwrite(output_path, cp.asnumpy(output_img.astype(np.uint8)))
    

    return project_time


def project_960_1080():
    equirectangular_image = cv2.imread('images/office.png') # replace with the actual path to your equirectangular image
    projector = equirectangular_to_perspective(110, equirectangular_image.shape[0], equirectangular_image.shape[1], 1080, 960)
    t1 = time.time()
    perspective_image = projector.project(equirectangular_image, radians(0), radians(90), radians(90))
    t2 = time.time()
    
    return t2-t1

if __name__ == '__main__':
    # Example usage:
    cube_time = draw_cube()
    print("cube_time: ", cube_time)
    
    _960_1080_time = 0
    for i in range(100):
        _960_1080_time += project_960_1080()
    print("_960_1080_time: ", _960_1080_time/100)

