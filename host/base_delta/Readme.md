##  Base-Delta Encoder
### How to run example usecase (Compress one 960x1080 image with fps measurement, tile_size = 4x4, the result will be saved using pickle):

This BD-Encoder use signed value to represent Delta, unsigned value maybe a little more effictient and intuitive, but here we just align with the paper.

(1) For CPU version, we use fully vectorized numpy and numba for acceleration. It runs at about 6.57 fps on EPYC-Zen3 CPU, you can try the example by running:
```bash
python3 BD_enc_cpu.py 
```

(2) For GPU version, we use fully vectorized cupy and fine-grained cuda (from numba) for acceleration. It runs at about 400 fps on RTX-4090, you can try the example by running:
```bash
python3 BD_enc_gpu.py
```
