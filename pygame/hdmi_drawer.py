# Codes for drawing images on a selected HDMI display port



import pygame
import numpy as np
import argparse

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
    i = 0
    while running:
        for event in pygame.event.get():
            print(event)
            if event.type == pygame.QUIT:
                running = False

        # ==================== Your Video Reading code here ====================
        i = i%10 
        # image_npy = np.load("./hogrider_20s_4k_out_t2r/"+ str(i) + ".npy")
        # # import ipdb
        # # ipdb.set_trace()
        # image = pygame.surfarray.make_surface(image_npy.transpose(1,0,2))
        # import ipdb
        # ipdb.set_trace()
        # image = pygame.image.load("./hogrider_20s_4k_out_t2r/"+ str(i) + ".png")
        image = pygame.image.load("./Image_Set/"+ str(i) + "_tile_to_row" + ".jpg")
        i=i+1


        # image = pygame.transform.scale(image, (W, H))
        window.blit(image, (0, 0)) # start drawing from the top left corner (0,0)
        pygame.display.flip()
        clock.tick(args.fps) # limit the frame rate

    pygame.quit()

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--display_mode', type=str, default='3840, 2160', help='HDMI display mode (Resolution here)')
    parser.add_argument('--display_port', type=int, default='1', help='HDMI display port')
    parser.add_argument('--fps', type=int, default='1', help='frame rate')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    args = parse_args()
    print(args)
    rendering()