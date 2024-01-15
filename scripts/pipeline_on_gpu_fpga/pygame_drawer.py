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
        image = pygame.image.frombuffer(image.tostring(), image.shape[1::-1], "BGR") #need to use BGR for pygame since it is BGR too
        self.window.blit(image, (0, 0))
        pygame.display.flip()

    def close(self):
        pygame.quit()