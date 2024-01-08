import argparse
from math import radians
def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--in_images_folder', type=str, help='path to the 360-video')
    parser.add_argument('--out_images_folder', type=str, help='path to the output video')
    # parameters for perspective projection
    parser.add_argument('--h_fov', type=float, default=80.0, help='horizontal_fov')
    parser.add_argument('--perspective_width', type=int, default=960, help='perspective_width')
    parser.add_argument('--perspective_height', type=int, default=1080, help='perspective_height')
    parser.add_argument('--equi_width', type=int, default=1920, help='perspective_width')
    parser.add_argument('--equi_height', type=int, default=1080, help='perspective_height')
    parser.add_argument('--roll', type=float, default=0, help='in radians')
    parser.add_argument('--yaw', type=float, default=radians(90), help='in radians')
    parser.add_argument('--pitch', type=float, default=radians(90), help='in radians')

    # parameters for lens correction
    parser.add_argument('--k1', type=float, default=0.33582564, help='k1')
    parser.add_argument('--k2', type=float, default=0.55348791, help='k2')
    parser.add_argument('--cx', type=float, default=480, help='cx')
    parser.add_argument('--cy', type=float, default=540, help='cy')
    parser.add_argument('--ppi', type=float, default=353.4, help='ppi')
    parser.add_argument('--display_distance', type=float, default=39.07, help='display_distance(mm)')

    # parameters for color optimization
    parser.add_argument('--tile_size', type=int, default=4, help='tile_size')
    parser.add_argument('--foveated', action='store_true', help='foveated')
    parser.add_argument('--max_ecc', type=float, default=15.0, help='max_ecc')
    parser.add_argument('--fixed_c', type=float, default=1e-4, help='c will be fixed to this value, since we empirically found that predicted c_s were too small')
    parser.add_argument('--ecc_no_compress', type=float, default=30.0, help='ecc (deg) smaller than this value will not be compressed (I will set abc = 1e-5)')
    
    parser.add_argument('--num_workers', type=int, default=8, help='num_workers')
    parser.add_argument('--save_imgs', action='store_true', help='save_imgs')
    parser.add_argument('--display', action='store_true', help='display real-time results')
    parser.add_argument('--display_port', type=int, default=0, help='port for video display')

    args = parser.parse_args()
    args.cx = args.perspective_width / 2
    args.cy = args.perspective_height / 2
    return args

