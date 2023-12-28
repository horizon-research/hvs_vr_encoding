import sys
import importlib
import cv2
from pathlib import Path
import os
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
vid_path = vid_path.split('/')
print(vid_path)
fname = vid_path[-1].split('.')[0]
ofname = fname+"_out.mp4"
opath = vid_path[:-1]
opath.append(ofname)
print(opath)
ofil = '/'+os.path.join(*opath)
print(ofil)
length = int(vid.get(cv2.CAP_PROP_FRAME_COUNT))
fps = vid.get(cv2.CAP_PROP_FPS)
width = vid.get(cv2.CAP_PROP_FRAME_WIDTH)
height = vid.get(cv2.CAP_PROP_FRAME_HEIGHT)
print(type(width),type(height))
vid_length = int(length/fps)
out = cv2.VideoWriter(ofil,cv2.VideoWriter_fourcc(*'mp4v'),fps=4*fps,frameSize=(int(width),int(height)))
print(ofil)
# exit()
with Progress() as progress:
	task1 = progress.add_task("[red]Reading Video...",total=length)

	while vid.isOpened():
		ret,frame = vid.read()
		if ret==False:
			break
		ecc_map = build_foveated_ecc_map(60,0,0,35,frame.shape[0],frame.shape[1])
		tf = frame/255
		tf = tf.reshape(int(frame.size/3),3)
		ecc_map = ecc_map.reshape(int(frame.size/3),1)
		dkl_centres = (RGB2DKL @ tf.T).T
		centres_abc = color_model.compute_ellipses(tf,ecc_map)
		dkl_centres *= (1<<16)-1
		centres_abc *= (1<<16)-1
		dkl_centres = np.ceil(dkl_centres)
		centres_abc = np.ceil(dkl_centres)
		dkl_centres = dkl_centres.astype(np.uint16).reshape(frame.shape)
		centres_abc = centres_abc.astype(np.uint16).reshape(frame.shape)
		dkl1 = np.zeros_like(frame)
		dkl1 = (dkl_centres>>8).astype(np.uint8)
		out.write(dkl1)
		dkl2 = np.zeros_like(frame)
		dkl2 = dkl_centres.astype(np.uint8)
		out.write(dkl2)
		abc1 = np.zeros_like(frame)
		abc1 = (centres_abc>>8).astype(np.uint8)
		out.write(abc1)
		abc2 = np.zeros_like(frame)
		abc2 = centres_abc.astype(np.uint8)
		out.write(abc2)
		progress.update(task1,advance=1)