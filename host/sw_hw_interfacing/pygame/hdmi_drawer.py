# Codes for drawing images on a selected HDMI display port



import pygame
import numpy as np
import argparse

from pygame_drawer import Pygame_drawer

def rendering():
    # initialize pygame
    pygame.init()
    # create a window with the display mode and selected display port
    W, H= args.display_mode.split(',')
    W = int(W)
    H = int(H)
    flags = pygame.FULLSCREEN
    window = pygame.display.set_mode((W, H), display=args.display_port)
    pygame.display.toggle_fullscreen()
    clock = pygame.time.Clock() # For controlling the frame rate

    running = True

    # import ipdb; ipdb.set_trace()
    frame_num = 1
    frame_dir = "./reversed_office_18_5_5/"
    i = 0

    pygame_drawer = Pygame_drawer(width = 3840, height = 2160, display_port = args.display_port)
    while running:
    
        i = i%frame_num
        # image_npy = np.load("./hogrider_20s_4k_out_t2r/"+ str(i) + ".npy")
        image_npy_reversed = np.load( frame_dir + "frame"+str(i)+".npy")
        pygame_drawer.draw(image_npy_reversed)
        i=i+1


        # image = pygame.transform.scale(image, (W, H))
        # window.blit(image, (0, 0)) # start drawing from the top left corner (0,0)
        # pygame.display.flip()
        clock.tick(args.fps) # limit the frame rate

        # import ipdb
        # ipdb.set_trace()
        # import ipdb
        # ipdb.set_trace()

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--display_mode', type=str, default='3840, 2160', help='HDMI display mode (Resolution here)')
    parser.add_argument('--display_port', type=int, default='1', help='HDMI display port')
    parser.add_argument('--fps', type=int, default='60', help='frame rate')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    args = parse_args()
    print(args)
    rendering()