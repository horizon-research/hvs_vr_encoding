from math import tan, radians, cos, sin, pi
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


def equirectangular_to_perspective(equirect_img, fov, roll, pitch, yaw, height, width):
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
    # Dimensions of the equirectangular image
    eq_height, eq_width, _ = equirect_img.shape

    # Create a new image with the desired dimensions and FOV
    perspective_img = np.zeros((height, width, 3), dtype=equirect_img.dtype)

    # Calculate necessary values
    h_fov = radians(fov)
    v_fov = h_fov * (float(height) / float(width)) 
    v_res = float(eq_height) / pi
    h_res = float(eq_width) / (2 * pi)

    # Calculate the camera rotation matrix
    R_roll = np.array([[1, 0, 0],
                    [0, cos(roll), -sin(roll)],
                    [0, sin(roll), cos(roll)]])

    R_pitch = np.array([[cos(pitch), 0, sin(pitch)],
                        [0, 1, 0],
                        [-sin(pitch), 0, cos(pitch)]])

    R_yaw = np.array([[cos(yaw), -sin(yaw), 0],
                    [sin(yaw), cos(yaw), 0],
                    [0, 0, 1]])

    R = R_roll @ R_pitch @ R_yaw

    # parrallelized numpy impl
    # Convert perspective pixel coordinates to normalized degrees coordinates
    x = np.linspace(-1, 1, width) * np.tan(h_fov / 2)
    y = np.linspace(1, -1, height) * np.tan(v_fov / 2)
    
    xp, yp = np.meshgrid(x, y)
    zp = np.ones_like(xp)

    # Apply the camera rotation to the vector
    vec = np.array([xp, yp, zp])
    rotated_vec = np.tensordot(R, vec, axes=1)

    # Convert 3D coordinates to spherical coordinates
    r = np.linalg.norm(rotated_vec, axis=0)
    theta_s = np.arctan2(rotated_vec[1], rotated_vec[0])
    phi_s = np.arccos(rotated_vec[2] / r)

    # Map the spherical coordinates to equirectangular pixel coordinates
    eq_x = (theta_s + np.pi) * h_res
    eq_y = phi_s * v_res

    # Get pixel value from equirectangular image if within bounds
    perspective_img = bilinear_interpolate(equirect_img, eq_y, eq_x)

    return perspective_img

# Now we will use the new function to create the perspective image from the equirectangular image
def draw_cube():
    equirectangular_image = cv2.imread('images/office.png') # replace with the actual path to your equirectangular image
    roll_angles =  [90, 0, -90,   0,   0,   0] # Roll angle
    pitch_angles = [0, 90,   0, -90,   0, 180]  # Pitch angle
    yaw_angles =   [0, 90, 180, 270,  90,  90] # Yaw angle

    perspective_width = 360
    perspective_height = 360
    fov = 90  # Field of view

    output_imgs = []
    for i in range(len(pitch_angles)):
        pitch_angle = pitch_angles[i]
        yaw_angle = yaw_angles[i]
        roll_angle = roll_angles[i]

        # Perform the projection from equirectangular to perspective view
        perspective_image_with_roll = equirectangular_to_perspective(equirectangular_image, fov, radians(roll_angle), radians(pitch_angle), radians(yaw_angle), perspective_height, perspective_width)

        # Save the perspective image
        output_imgs.append(perspective_image_with_roll)

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

if __name__ == '__main__':
    # Example usage:
    t1 = time.time()
    draw_cube()
    t2 = time.time()
    print("time: ", t2-t1)
