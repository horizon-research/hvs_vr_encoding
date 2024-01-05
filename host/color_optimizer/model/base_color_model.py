from pathlib import Path
import sys

import numpy as np
import pandas as pd
import torch
from torch import nn
from torch.utils.data import Dataset, DataLoader

from .invoker import Module
from util.colorspace import calibrate_DKL_colorspace, sRGB2RGB, RGB2sRGB, RGB2XYZ, XYZ2LMS, LMS2DKL
from util import torch_rbf as rbf


class BaseColorModel(Module):
    @classmethod
    def args(cls):
        # Specify arguments to pass from command line
        return {
            "layer_widths": [4, 3],  # (C_LM, C_S, V, e) --> (a, b, c)
            "layer_centres": [5],
            "rng_seed": 0,
        }

    def initialize(self):
        super(BaseColorModel, self).initialize()
        torch.manual_seed(self.opt.rng_seed)
        self.model = Network(
            self.opt.layer_widths, self.opt.layer_centres, rbf.gaussian)

    def optimize(self, x, y, nepochs, batch_size, lr, loss_func):
        self.model.train()
        obs = x.size(0)
        train_set = SimpleDataset(x, y)
        train_loader = DataLoader(train_set, batch_size=batch_size, shuffle=True)
        optimizer = torch.optim.Adam(self.model.parameters(), lr=lr)
        epoch = 0
        for epoch in range(nepochs):
            current_loss = 0
            progress = 0
            for batch_id, (x_batch, y_batch) in enumerate(train_loader):
                optimizer.zero_grad()
                y_hat = self.model.forward(x_batch)
                loss = loss_func(y_hat, y_batch)
                current_loss += 1/(batch_id+1) * (loss.item() - current_loss)
                loss.backward()
                optimizer.step()
                progress += y_batch.size(0)
                sys.stdout.write('\rEpoch %d, Progress: %d/%d, Loss %f\t' %\
                                 (epoch+1, progress, obs, current_loss*10000))

    def eval(self, x):
        self.model.eval()
        if type(x) is np.ndarray:
            x = torch.tensor(x, dtype=torch.float32)
        return self.model.forward(x)

    def compute_ellipses(self, img, ecc_map):
        # Convert Image to DKL
        RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ
        img_dkl = (RGB2DKL @ sRGB2RGB(img.reshape(-1, 3)).T).T

        # Compute Neutral Gray for each pixel
        lum = img_dkl[:, -1]
        pedestal = np.stack([lum, lum, lum])
        pedestal_dkl = (RGB2DKL @ pedestal).T

        # Compute Color Contrast
        contrasts = img_dkl / (pedestal_dkl+1e-9) - 1

        # Evaluate Model
        ecc_map = ecc_map.reshape(-1, 1)
        inp = np.concatenate((contrasts, ecc_map), axis=-1)
        c_abc = self.eval(inp).detach().numpy()
        #import matplotlib.pyplot as plt
        #plt.figure()
        #plt.imshow(img)
        #plt.figure()
        #plt.imshow(c_abc[..., 0].reshape(*img.shape[:2]))
        #plt.figure()
        #plt.imshow(c_abc[..., 1].reshape(*img.shape[:2]))
        #plt.figure()
        #plt.imshow(c_abc[..., 2].reshape(*img.shape[:2]))
        #plt.show()
        abc = abs(pedestal_dkl * c_abc)
        return abc


    def compute_ellipses_vectorized(self, img, ecc_map):
        # Convert Image to DKL
        RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ
        img_dkl = (RGB2DKL @ sRGB2RGB(img.reshape(-1, 3)).T).T

        # Compute Neutral Gray for each pixel
        lum = img_dkl[:, -1]
        pedestal = np.stack([lum, lum, lum])
        pedestal_dkl = (RGB2DKL @ pedestal).T

        # Compute Color Contrast
        contrasts = img_dkl / (pedestal_dkl+1e-9) - 1

        # Evaluate Model
        ecc_map = ecc_map.reshape(-1, 1)
        inp = np.concatenate((contrasts, ecc_map), axis=-1)
        c_abc = self.eval(inp).detach().numpy()
        #import matplotlib.pyplot as plt
        #plt.figure()
        #plt.imshow(img)
        #plt.figure()
        #plt.imshow(c_abc[..., 0].reshape(*img.shape[:2]))
        #plt.figure()
        #plt.imshow(c_abc[..., 1].reshape(*img.shape[:2]))
        #plt.figure()
        #plt.imshow(c_abc[..., 2].reshape(*img.shape[:2]))
        #plt.show()
        abc = abs(pedestal_dkl * c_abc)
        return abc

    def apply_filter(self, img, ecc_map, energy_vec):
        RGB2DKL = LMS2DKL @ XYZ2LMS @ RGB2XYZ
        DKL2RGB = np.linalg.inv(RGB2DKL)

        # Compute Energy normal vector in DKL
        energy_vec_dkl = energy_vec @ DKL2RGB
        energy_vec_dkl[-1] = 0

        # Evaluate energy minimizing dkl value
        abc = self.compute_ellipses(img, ecc_map)
        denom = np.sqrt(np.sum(abc**2 * energy_vec_dkl**2, axis=-1, keepdims=True))
        img_dkl = (RGB2DKL @ sRGB2RGB(img.reshape(-1, 3)).T).T
        out_dkl = img_dkl + abc**2 * energy_vec_dkl / (denom + 1e-9)
        out = (DKL2RGB @ out_dkl.T).T.reshape(img.shape)
        return RGB2sRGB(out)

    def save(self, path):
        torch.save(self.model.state_dict(), path)

    def load(self, path):
        self.model.load_state_dict(torch.load(path))

    def export(self, path):
        torch.onnx.export(
            self.model,
            torch.ones((1, 4)),
            path,
            opset_version=9,
            input_names=['inp'],
            output_names=['out'],
        )

    def dump_weights(self, path):
        root = Path(path)
        root.mkdir(parents=True, exist_ok=True)
        for k, v in self.model.state_dict().items():
            df = pd.DataFrame(v.numpy())
            df.to_csv(root / k, index=False, header=False)



class SimpleDataset(Dataset):
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __len__(self):
        return self.x.size(0)

    def __getitem__(self, idx):
        x = self.x[idx]
        y = self.y[idx]
        return (x, y)


class Network(nn.Module):
    def __init__(self, layer_widths, layer_centres, basis_func):
        super(Network, self).__init__()
        self.rbf_layers = nn.ModuleList()
        self.linear_layers = nn.ModuleList()
        self.output_layer = nn.Sigmoid()
        for i in range(len(layer_widths) - 1):
            self.rbf_layers.append(rbf.RBF(layer_widths[i], layer_centres[i], basis_func))
            self.linear_layers.append(nn.Linear(layer_centres[i], layer_widths[i+1]))

        # Hardcode maximum supported contrast
        self.lm_max = 0.3025
        self.s_max = 0.00655
        w = torch.tensor([1 / self.lm_max, 1 / self.s_max, 1, 1/35], dtype=torch.float32)
        b = torch.tensor([0, 0, 0, 0], dtype=torch.float32)
        self.normalize = lambda x: w * x + b
        self.denormalize = lambda y: (y - b) * w

    def forward(self, x):
        out = self.normalize(x)
        for i in range(len(self.rbf_layers)):
            out = self.rbf_layers[i](out)
            out = self.linear_layers[i](out)
        return self.output_layer(out) * torch.tensor([self.lm_max, self.s_max, self.s_max])
