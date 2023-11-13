"""
This script approximates the optimal vectors to use in the color optimization process executed in red_blue_optimization.
These vectors are meant to represent the vector spanning from the minimum point to the maximum point of an ellipsoid on
with respect to a given color channel. These vectors are found for an ellipsoid by finding the intersection of the two
planes defined by the partial derivatives of the ellipsoid equation with respect to the two adjacent color axes to the 
one given. The optimal vecotr is found by taking a mean of a sample of such vectors found from an evenly distributed 
sample of colors. 
Vectors vary greatly with respect to the eccentricity and c value specified, so new vectors will need to be found when
testing with new c values especially.
"""

import sys
import importlib
import numpy as np
import pandas as pd
from pathlib import Path

sys.path.append('..')

# for virtual machines
# import base_color_model
# color_model = base_color_model.BaseColorModel()
# color_model.initialize()

bcm = importlib.import_module("color_model").get_class("base")
color_model = bcm({})

path = "../io/color_model"
model_fn = "model.pth"
model_root = Path(path)
color_model.load(model_root / model_fn)

from util.colorspace import RGB2sRGB, sRGB2RGB, XYZ2RGB, LMS2XYZ, DKL2LMS, LMS2DKL, XYZ2LMS, RGB2XYZ
DKL2RGB = XYZ2RGB @ LMS2XYZ @ DKL2LMS
RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ

# ids for color directions
red_i = 0
green_i = 1
blue_i = 2

# hardcoded constants for eccentricity and ellipsoid c value
ECC = 25
C_VALUE = .002 #1e-5

# Finds the optimal min to max spanning vectors for generated ellipsoids along a given color direction
# by taking finding the mean vector from the vectors found for a sample of colors
# saves sample data for vectors to csv file is csv is True
def find_opt_vec(col_i, csv):

    # evenly distributed sample of colors to find vectors for
    sample = np.linspace(1, 255, 120)   # sample 1728000 colors
    rr, gg, bb = np.meshgrid(sample, sample, sample)
    colors = np.stack([rr, gg, bb], axis=-1).reshape(-1, 3)

    print("Sample size:", colors.shape[0], "colors")

    # converts colors to dkl
    srgb = colors/255
    rgb = sRGB2RGB(srgb)
    dkl = (RGB2DKL @ rgb.T).T

    # generates ellipdoids for each color
    ecc_map = np.full((colors.shape[0],1),ECC, dtype=float)
    abc = color_model.compute_ellipses(srgb, ecc_map)
    abc[:,2] = C_VALUE

    # print(abc.shape)
    # print(abc)

    # Finds coefficients that define each ellipse
    abc_sq = np.square(abc)
    dkl_sq = np.square(dkl)
    fac = dkl_sq/abc_sq
    fac = fac[:,0]+fac[:,1]+fac[:,2]-1
    A = 1/abc_sq[:,0]
    B = 1/abc_sq[:,1]
    C = 1/abc_sq[:,2]
    D = -2*A*dkl[:,0]
    E = -2*B*dkl[:,1]
    F = -2*C*dkl[:,2]

    tm = np.zeros((6,9))
    tm[0:3,0:3] = RGB2DKL*RGB2DKL
    tm[3:6,3:6] = RGB2DKL
    tm[0:3,6:9] = 2*RGB2DKL*np.roll(RGB2DKL,-1,axis=1)

    # converts 6 dkl coefficients into 9 rgb coefficients
    params = np.stack((A,B,C,D,E,F))/fac
    nparams = tm.T @ params

    # uses rgb coefficients to find vector spanning from the minimum
    # point on an ellipsoid to the maximum point on the given color axis
    # this calculation is based on the intersection of the two planes defined
    # by the partial derivatives of the ellipsoid equation with respect to the 
    # two adjacent color directions to the color given
    if (col_i == red_i):
        i = 4*nparams[1]*nparams[2]-np.square(nparams[7])
        j = nparams[8]*nparams[7]-2*nparams[6]*nparams[2]
        k = nparams[6]*nparams[7]-2*nparams[8]*nparams[1]
        vecs = np.stack((i,j,k))

    elif (col_i == green_i):
        i = 2*nparams[6]*nparams[2]-nparams[7]*nparams[8]
        j = np.square(nparams[8])-4*nparams[0]*nparams[2]
        k = 2*nparams[0]*nparams[7]-nparams[8]*nparams[6]
        vecs = np.stack((i,j,k))

    elif (col_i == blue_i):
        i = nparams[6]*nparams[7]-2*nparams[1]*nparams[8]
        j = nparams[6]*nparams[8]-2*nparams[0]*nparams[7]
        k = 4*nparams[0]*nparams[1]-np.square(nparams[6])
        vecs = np.stack((i,j,k))

    # convert vectors found to unit vectors
    norms = np.linalg.norm(vecs, axis=0)
    unit_vecs = vecs / norms

    # print("unit_vecs:", unit_vecs.T)

    # finds the angles that define the vector
    alpha = np.arccos(vecs[0]/norms)
    beta = np.arccos(vecs[1]/norms)
    gamma = np.arccos(vecs[2]/norms)

    # save sample data to csv file if specified
    if (csv):
        data = {
            "Alpha": alpha,
            "Beta": beta,
            "Gamma": gamma,
            "Unit X": unit_vecs[0],
            "Unit Y": unit_vecs[1],
            "Unit Z": unit_vecs[2]
        }

        df = pd.DataFrame(data)
        if (col_i == red_i):
            df.to_csv("Optimal_Blue_Vec_Output.csv")
        elif (col_i == green_i):
            df.to_csv("Optimal_Green_Vec_Output.csv")
        elif (col_i == blue_i):
            df.to_csv("Optimal_Red_Vec_Output.csv")

    opt_vec = np.mean(unit_vecs, axis=1)
    return opt_vec

if __name__ == "__main__":

    red_vec = find_opt_vec(red_i, False)
    print("Optimal red vector:", red_vec)

    green_vec = find_opt_vec(green_i, False)
    print("Optimal red vector:", green_vec)

    blue_vec = find_opt_vec(blue_i, False)
    print("Optimal red vector:", blue_vec)