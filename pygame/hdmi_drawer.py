# Codes for drawing images on a selected HDMI display port



import pygame
import argparse

def rendering():
    # initialize pygame
    pygame.init()
    # create a window with the display mode and selected display port
    W, H= args.display_mode.split(',')
    W = int(W)
    H = int(H)
    window = pygame.display.set_mode((W, H), display=args.display_port)
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
        image = pygame.image.load("./Image_Set/"+ str(i) + ".jpg")
        i=i+1


        image = pygame.transform.scale(image, (W, H))
        window.blit(image, (0, 0)) # start drawing from the top left corner (0,0)
        pygame.display.flip()
        clock.tick(args.fps) # limit the frame rate

    pygame.quit()

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--display_mode', type=str, default='1920, 1080', help='HDMI display mode (Resolution here)')
    parser.add_argument('--display_port', type=int, default='1', help='HDMI display port')
    parser.add_argument('--fps', type=int, default='1', help='frame rate')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    args = parse_args()
    print(args)
    rendering()