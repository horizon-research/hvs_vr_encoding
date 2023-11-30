from math import tan, radians, cos, sin, pi
import numpy as np
import cv2


# Since the previous transformation might have been incorrect, we will redefine the projection function
# to properly handle equirectangular to plane projection.

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
    R_roll = np.array([[cos(roll), -sin(roll), 0],
                       [sin(roll), cos(roll), 0],
                       [0, 0, 1]])
    R_pitch = np.array([[1, 0, 0],
                        [0, cos(pitch), -sin(pitch)],
                        [0, sin(pitch), cos(pitch)]])
    R_yaw = np.array([[cos(yaw), 0, sin(yaw)],
                      [0, 1, 0],
                      [-sin(yaw), 0, cos(yaw)]])
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
roll_angle = 0  # Roll angle
pitch_angle = 90  # Pitch angle
yaw_angle = 270  # Yaw angle (previously roll angle)
perspective_width = 800
perspective_height = 600
fov = 90  # Field of view

equirectangular_image = cv2.imread('office.png') # replace with the actual path to your equirectangular image
# Perform the projection from equirectangular to perspective view
perspective_image_with_roll = equirectangular_to_perspective_with_roll(equirectangular_image, fov, radians(roll_angle), radians(pitch_angle), radians(yaw_angle), perspective_height, perspective_width)

# Save the perspective image
output_path = 'perspective_image.png'
cv2.imwrite(output_path, perspective_image_with_roll)
output_path
