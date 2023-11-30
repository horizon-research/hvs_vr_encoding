from math import tan, radians, cos, sin, pi
import numpy as np
import cv2
import matplotlib.pyplot as plt

def equirectangular_to_perspective_with_roll(equirect_img, fov, roll, pitch, yaw, height, width):
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
    v_fov = radians(fov)
    h_fov = v_fov * (float(width) / float(height))
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

    # For each pixel in the perspective image, find the corresponding pixel in the equirectangular image
    for y in range(height):
        for x in range(width):
            # Convert perspective pixel coordinates to normalized coordinates (-0.5 to 0.5)
            xp = (2 * (x / (width - 1)) - 1) * tan(h_fov / 2)
            yp = (1 - 2 * (y / (height - 1))) * tan(v_fov / 2)
            zp = 1

            # Apply the camera rotation to the vector
            [xs, ys, zs] = np.dot(R, [xp, yp, zp])

            # Convert 3D coordinates to spherical coordinates
            r = np.sqrt(xs * xs + ys * ys + zs * zs)
            theta_s = np.arctan2(ys, xs)
            phi_s = np.arccos(zs / r)

            # Map the spherical coordinates to equirectangular pixel coordinates
            eq_x = (theta_s + pi) * h_res
            eq_y = phi_s * v_res

            # Get pixel value from equirectangular image if within bounds
            if 0 <= int(eq_x) < eq_width and 0 <= int(eq_y) < eq_height:
                perspective_img[y, x] = equirect_img[int(eq_y), int(eq_x)]

    return perspective_img

# Now we will use the new function to create the perspective image from the equirectangular image

equirectangular_image = cv2.imread('office.png') # replace with the actual path to your equirectangular image




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
    perspective_image_with_roll = equirectangular_to_perspective_with_roll(equirectangular_image, fov, radians(roll_angle), radians(pitch_angle), radians(yaw_angle), perspective_height, perspective_width)

    # Save the perspective image
    output_imgs.append(perspective_image_with_roll)

# Save the perspective images using cube like 3 x 4 grid
output_img = np.ones((perspective_height * 3, perspective_width * 4, 3))
output_img.fill(255)
# import ipdb; ipdb.set_trace()
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

output_path = 'cube_perspective_image.png'
cv2.imwrite(output_path, output_img.astype(np.uint8))
output_path
