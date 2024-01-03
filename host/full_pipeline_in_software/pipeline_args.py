import argparse
from math import radians
def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--in_images_folder', type=str, help='path to the 360-video')
    parser.add_argument('--out_images_folder', type=str, help='path to the output video')
    # parameters for perspective projection
    parser.add_argument('--fov', type=float, default=110.0, help='fov')
    parser.add_argument('--perspective_width', type=int, default=960, help='perspective_width')
    parser.add_argument('--perspective_height', type=int, default=1080, help='perspective_height')
    parser.add_argument('--roll', type=float, default=0, help='in radians')
    parser.add_argument('--yaw', type=float, default=radians(90), help='in radians')
    parser.add_argument('--pitch', type=float, default=radians(90), help='in radians')

    # parameters for lens correction
    parser.add_argument('--k1', type=float, default=0.33582564, help='k1')
    parser.add_argument('--k2', type=float, default=0.55348791, help='k2')
    
    parser.add_argument('--num_workers', type=int, default=8, help='num_workers')

    args = parser.parse_args()
    args.cx = args.perspective_width / 2
    args.cy = args.perspective_height / 2
    return args