import pygame
import cv2
import numpy as np

class Pygame_drawer:
    def __init__(self, width, height, display_port):
        pygame.init()
        self.W, self.H= width, height
        self.W = int(self.W)
        self.H = int(self.H)
        self.window = pygame.display.set_mode((self.W, self.H), pygame.DOUBLEBUF | pygame.HWSURFACE, display=display_port)
        pygame.display.toggle_fullscreen()
        self.clock = pygame.time.Clock()
    
    def draw(self, image):
        # image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        # image = image.transpose(1,0,2)
        # image = pygame.surfarray.make_surface(image)
        image = pygame.image.frombuffer(image.tostring(), image.shape[1::-1], "BGR")
        self.window.blit(image, (0, 0))
        pygame.display.flip()

    def close(self):
        pygame.quit()