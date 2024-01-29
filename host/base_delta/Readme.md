##  Base-Delta Encoder and Decoder
This BD implementation is a little different from the paper implementation, we add two improvements:
- store bitlens instead of tags since bitlens is shorter.
- using minimum value as base instead of average of max and min since it can prevent signed / unsigned conversion.

### 1. Encoder - Decoder Loop back test:

We have a loopback test for correctness, in this test we first encode an image and decode it back. By comparing the original image and the decode one, we can verify the correctness of code.

To run Loop back test for CPU implementation, run:
```bash
python3 loop_back_test_cpu.py
```

for GPU version, run:
```bash
python3 loop_back_test_gpu.py
```


### 2. Encoder example usecase: Compress one 960x1080 image with fps and compress rate measurement, tile_size = 4x4, the result will be saved in test_data/ using pickle:

(1) For CPU version, we use fully vectorized numpy and numba for acceleration. It runs at about 7.2 fps on EPYC-Zen3 CPU, you can try the example by running:
```bash
cd BD_encoder
python3 BD_enc_cpu.py 
```

(2) For GPU version, we use fully vectorized cupy and fine-grained cuda (from numba) for acceleration. It runs at about 614.4 fps on RTX-4090, you can try the example by running:
```bash
cd BD_encoder
python3 BD_enc_gpu.py
```


### 3. Decoder example usecase: Decode a 960x1080 encode by encoder image with fps measurement, tile_size = 4x4, the result won't be saved:

(1) For CPU version, we use fully vectorized numpy and numba for acceleration. It runs at about 23.4 fps on EPYC-Zen3 CPU, you can try the example by running:
```bash
cd BD_decoder
python3 BD_dec_cpu.py 
```

(2) For GPU version, we use fully vectorized cupy and fine-grained cuda (from numba) for acceleration. It runs at about 511.8 fps on RTX-4090, you can try the example by running:
```bash
cd BD_decoder
python3 BD_dec_gpu.py
```
