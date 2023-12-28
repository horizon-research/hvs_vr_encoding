import os
import sys
import importlib
import cv2
from pathlib import Path

from PIL import Image
import numpy as np
from timeit import default_timer as timer
# import pandas as pd
from util.ecc_map import *
from rich.progress import Progress

bcm = importlib.import_module("color_model").get_class("base")
color_model = bcm({})
path = "./io/color_model"
model_fn = "model.pth"
model_root = Path(path)
color_model.load(model_root / model_fn)

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ

vid_path = sys.argv[1]
vid = cv2.VideoCapture(vid_path)
length = int(vid.get(cv2.CAP_PROP_FRAME_COUNT))

global_min = np.inf
global_max = -np.inf

with Progress() as progress:
	task1 = progress.add_task("[red]Reading Video...",total=length)

	while vid.isOpened():
		ret,frame = vid.read()
		if ret==False:
			break
		# print(type(frame))
		# print(frame.shape,frame.size/3)
		ecc_map = build_foveated_ecc_map(60,0,0,35,frame.shape[0],frame.shape[1])
		tf = frame/255
		tf = tf.reshape(int(frame.size/3),3)
		ecc_map = ecc_map.reshape(int(frame.size/3),1)
		dkl_centres = (RGB2DKL @ tf.T).T
		centres_abc = color_model.compute_ellipses(tf,ecc_map)
		maxv = max(np.amax(dkl_centres),np.amax(centres_abc))
		minv = max(np.amin(dkl_centres),np.amax(centres_abc))
		global_max = max(maxv,global_max)
		global_min = min(minv,global_min)
		progress.update(task1,advance=1)

print(global_max,global_min)