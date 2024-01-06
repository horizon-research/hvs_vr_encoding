from math import tan, radians, cos, sin, pi
import numpy as np
import cv2
import matplotlib.pyplot as plt
import time

class equirectangular_to_perspective():
    """
    Convert equirectangular image to perspective view including roll rotation.
    :param equirect_img: The input equirectangular image.
    :param fov: Field of View of the perspective projection.
    :param roll: Roll angle in radians (rotation around the forward axis).
    :param pitch: Pitch angle in radians (rotation around the right axis).
    :param yaw: Yaw angle in radians (rotation around the up axis).
    :param height: Height of the output image.
    :param width: Width of the output image.
    :return: Projected perspective image with roll rotation.
    """
    def __init__(self, fov, equi_height, equi_width, out_height, out_width):
        # pre assign because we need them in update_fov
        self.out_height = out_height
        self.out_width = out_width
        self.update_fov(fov)
        self.update_out_dims(out_height, out_width)
        self.update_equi_dims(equi_height, equi_width)
        
    def update_equi_dims(self, equi_height, equi_width):
        self.equi_height = equi_height
        self.equi_width = equi_width
        self.v_res = float(equi_height) / pi
        self.h_res = float(equi_width) / (2 * pi)

        self.v_res = np.asarray(self.v_res, dtype=np.float32) * np.ones_like(self.xp)
        self.h_res = np.asarray(self.h_res, dtype=np.float32) * np.ones_like(self.xp)

    def update_fov(self, fov):
        self.fov = fov
        self.h_fov = radians(fov)
        self.v_fov = self.h_fov * (float(self.out_height) / float(self.out_width))
        self.h_fov = np.asarray(self.h_fov, dtype=np.float32)
        self.v_fov = np.asarray(self.v_fov, dtype=np.float32)

    def update_out_dims(self, out_height, out_width):
        self.out_height = out_height
        self.out_width = out_width
        # parrallelized numpy impl
        # Convert perspective pixel coordinates to normalized degrees coordinates
        self.xs = np.linspace(-1, 1, self.out_width, dtype=np.float32) * np.tan(self.h_fov / 2, dtype=np.float32)
        self.ys = np.linspace(1, -1, self.out_height , dtype=np.float32) * np.tan(self.v_fov / 2, dtype=np.float32 )
        self.xp, self.yp = np.meshgrid(self.xs, self.ys)
        self.zp = np.ones_like(self.xp)
        self.vec = np.array([self.xp, self.yp, self.zp])


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

        # Apply the camera rotation to the vector
        rotated_vec = np.tensordot(R, self.vec, axes=1)

        # Convert 3D coordinates to spherical coordinates
        r = np.linalg.norm(rotated_vec, axis=0)
        theta_s = np.arctan2(rotated_vec[1], rotated_vec[0])
        phi_s = np.arccos(rotated_vec[2] / r)

        # Map the spherical coordinates to equirectangular pixel coordinates
        eq_x = (theta_s + np.pi) * self.h_res
        eq_y = phi_s * self.v_res


        # Get pixel value from equirectangular image if within bounds
        return self.bilinear_interpolate(equirect_img, eq_y, eq_x)
    
    def bilinear_interpolate(self, image, y, x):
        """
        Performs bilinear interpolation for a given set of image points in a vectorized manner.

        :param image: source image
        :param x: array of x coordinates
        :param y: array of y coordinates
        :return: interpolated pixel values
        """
        x1 = np.floor(x)
        y1 = np.floor(y)
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
        y1_idx = y1.astype(np.uint32)
        y2_idx = y2.astype(np.uint32)
        x1_idx = x1.astype(np.uint32)
        x2_idx = x2.astype(np.uint32)

        values = (
                    (image[y1_idx, x1_idx, :] * (1 - dx) * (1 - dy)) +
                    (image[y1_idx, x2_idx, :] * dx * (1 - dy)) +
                    (image[y2_idx, x1_idx, :] * (1 - dx) * dy) +
                    (image[y2_idx, x2_idx, :] * dx * dy)
                )
        
        return values



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

    project_time = 0

    projector = equirectangular_to_perspective(fov, equirectangular_image.shape[0], equirectangular_image.shape[1], perspective_height, perspective_width)

    output_imgs = []
    for i in range(len(pitch_angles)):
        pitch_angle = pitch_angles[i]
        yaw_angle = yaw_angles[i]
        roll_angle = roll_angles[i]
    
        # Perform the projection from equirectangular to perspective view
        equirectangular_image = np.asarray(equirectangular_image, dtype=np.float32)
        t1 = time.time()
        perspective_image = projector.project(equirectangular_image, radians(roll_angle), radians(pitch_angle), radians(yaw_angle))
        t2 = time.time()
        project_time += t2-t1
        # Save the perspective image
        output_imgs.append(perspective_image)

    # Save the perspective images using cube like 3 x 4 grid
    output_img = np.ones((perspective_height * 3, perspective_width * 4, 3))
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
    cv2.imwrite(output_path, output_img.astype(np.uint8))
    

    return project_time


def measure_fps_project_960_1080(test_times=10):
    equirectangular_image = cv2.imread('images/office.png') # replace with the actual path to your equirectangular image
    projector = equirectangular_to_perspective(110, equirectangular_image.shape[0], equirectangular_image.shape[1], 1080, 960)
    equirectangular_image = np.asarray(equirectangular_image, dtype=np.float32)
    t1 = time.time()
    for i in range(test_times):
        perspective_image = projector.project(equirectangular_image, radians(0), radians(90), radians(90))
    t2 = time.time()

    fps = 1 / (t2-t1) * test_times
    
    return fps

if __name__ == '__main__':
    # Example usage:
    cube_time = draw_cube()
    print("cube_time: ", cube_time)
    fps = measure_fps_project_960_1080(test_times = 10)
    print("fps: ", fps)

