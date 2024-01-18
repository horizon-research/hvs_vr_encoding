# Gaze-Contingent Image/Video Encoding For Virtual Reality: An FPGA Demo

## 1. Overview

&nbsp; &nbsp; This project demonstrates color optimizer in the ASPLOS-2024 Paper [Exploiting Human Color Discrimination for Memory and Energy-Efficient Image Encoding in Virtual Reality](https://horizon-lab.org/pubs/asplos24-vr.pdf) on a FPGA board. This optimizer uses the limits of human color perception to reduce image color size. By subtly adjusting pixel colors within a humanly imperceptible range which depends on eccentricity, it brings colors of pixels closer, enhancing the efficiency of the following Base Delta compression algorithm. Finally, this foveated compression can make the system more memory and energy-efficient

### 1.1 Overall Pipeline (BD not included now.)

&nbsp; &nbsp; The figure illustrates the project's comprehensive pipeline, transforming panoramic video into the foveated compressed (Color Optimiz) video compatible with Google Cardboard. The pipeline is divided into two main module groups: one operating on the host machine and the other on an FPGA. The host machine handles video decoding, projection, parameter precomputation, and rearrangement. In contrast, the FPGA accelerates color adjustment and lens correction. These two platforms are interconnected via an HDMI cable. The final output is rendered on the VR display for immersive viewing.

<img src="doc_images/pipeline.png" alt="Alt text" width="800"/>

### 1.2 Examples of the foveated compressed output
<img src="doc_images/md_office.jpg" alt="Alt text" width="800"/>

## 2. Files Organization

- `host/`: Modules run on Host Machine.
    - `video_encode_decode/`: codes for video decode and encode.
    - `projection/`: codes for eqirectangular to perspective images projection.
    - `len_correction/`: codes for len correction (implemented in software)
    - `color_optimizer/`:  codes for color optimizer (implemented in software)
    - `BD_enc/` : TBD
    - `BD_dec/` : TBD
    - `fpga_interfaceing/` : To be seperated from scripts/pipeline_on_gpu_fpga/per_frame_seq_pipeline.py

- `fpga/`: Modules run on FPGA.
    - `tile_color_optimizer_hls/`: HLS implementation of color optimizer.
    - `len_correction_hls/`: HLS implementation of lens correction.
    - `rearrangment_hls/`: Verilog and HLS implementation of 4x4 to 1x1 reaarange ment IP (RIP) on FPGA.
    - `double_output_hls/`:  HLS implementation of copy left image to right on FPGA. It also has t_last signal needed by DMA.
    - `pynq_scripts/`: Jupyter note code run on PS of ZCU104
    - `end2end_bitstream/` : Built bitstream for the GPU+FPGA demo.
    - `ip_repo/`: Contain exported HLS IPs needed by GPU+FPGA demo.
    - `BD_enc_hls/` : TBD
    - `BD_dec_hls/` : TBD

- `scripts/` :
    - `pipeline_on_cpu/`: scripts for running software-only pipeline on CPU.
    - `pipeline_on_gpu/`: scripts for running software-only pipeline on GPU.
    - `pipeline_on_gpu_fpga/`: scripts for running pipeline on GPU + FPGA.
    - `vivado_scripts/`: scripts for generate and connect all modules in the block design.

## 3. Usage of Software-Only Pipeline (CPU or GPU (CUDA only) )

&nbsp; &nbsp;  This section is about how to run the full pipeline in software only manner. It is useful for quick check of expected result. It mainly contains below pipeline

<img src="doc_images/pipeline_software_only.png" alt="Alt text" width="800"/>

### 3.1 Video Preparation: 
To start this experiment, you need to prepare a panoramic Video in equirectangular format.

If you don't have one, you can download our demo video from https://drive.google.com/file/d/1feO5JxJLpI8r2QzsmC18rC69gUGPxRmw/view?usp=sharing

```bash
cd <top_folder>
mkdir videos # make videos directory in the main folder
# this link is for wget, above one is for browser
wget 'https://drive.google.com/uc?id=1feO5JxJLpI8r2QzsmC18rC69gUGPxRmw' -O videos/demo_video.mp4
```

### 3.2 Decode Video to a Folder of Images

In this step, we need to decode the videos to images to facilitate later multiprocessing.
```bash
cd <top_folder>
python3 host/video_encode_decode/decode_video.py --video_path ./videos/demo_video.mp4 --out_images_folder ./decoded_images
```
Now you can find decoded images in [decoded_images/](decoded_images/) in the main folder. To reduce downstream computatation, you can choose to preserve only 60 of them
```bash
cd <top_folder>
bash host/video_encode_decode/filter_decoded_images.bash "./decoded_images" 60
```

### 3.3 Run the Full color optimizer pipeline

&nbsp; &nbsp;  We provide scripts to run the full color optimizer pipeline in SW, including CPU, GPU implementations. All modules are corrently run in sequential order. It can be extend to ROS-like parrallel implementation in the future.

&nbsp; &nbsp;  For this project, the left and right eye images are exactly the same, since the input is a single equirectangular image, which supports only 3 DoF.  If the input video is captured in, for instance, an Omni-Directional Stereo (ODS) format, we could render actual stereo disparity.  See [this slide deck](https://cs.rochester.edu/courses/572/fall2022/decks/lect17-immersive.pdf) for details.  Because of this limitation, observers don't get depth perception from stereo disparity.

(1) The scripts to run the whole pipeline for one frame is implemented in `scripts/pipeline_on_\<device\>/per_frame_seq_pipeline.py`, please refer them to see how to use and concatenate all modules implemented in CPU , GPU.

(2) Every module's main function also shows example of how to use it. For example, the example code for projection is drawing the cube map and test FPS, you can run it as follow:
```bash
cd <top_folder>/host/projection
python3 equirect_to_pespective_cpu.py # if you want to use cuda acceleration, run: python3 equirect_to_pespective_cuda.py
```

(3) Here is how to run on CPU and GPU
- For CPU:
```bash
cd <top_folder>
python3 scripts/pipeline_on_cpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --foveated
```
- For GPU:
```bash
cd <top_folder>
python3 scripts/pipeline_on_gpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --display_port 0 --foveated
```

For GPU implementation, we provide a GUI as below for real-time parameters adjustment:

<img src="doc_images/gui.png" alt="Input Image" style="width: 300px; margin-right: 20px;" />


See [<top_folder>/scripts/args.py](scripts/args.py) for all supported args.


(4) After running the above codes, you will see output in [corrected_opt_images/](corrected_opt_images/) folder in main directory. (if you add --save_imgs)

### 3.5 Video Encoding
If you use CPU implementation and want to observe the realtime results. You can encode images back to video then playback it on your VR display in realtime.
```bash
cd <top_folder> 
python3 host/video_encode_decode/encode_images_to_video.py --video_path ./videos/corrected_opt_images.mp4 --images_folder ./corrected_opt_images --fps 30
```
The output video will be in ```./videos/corrected_opt_images.mp4```

## 4. Usage of SW-HW Pipeline (GPU-FPGA)

This pipeline are basically the same as overall pipeline, except that we add a `output_doubler` after the `lens_correction` since we use same image for both eye because of restriction comes from input equirectangular image as explaned above.

### 4.1 Setup vivado block design and get bitstream for FPGA

Hint: You can choose to skip this since we provide pre-generated .bit and .hwh [here](fpga/end2end_bitstream/).

We provide fully automatic script that can build vivado block design and generate bitstream and hardware handoff file. See Readme in [sripts/vivado](sripts/vivado) for detailed tutorial and reminder. 
```bash
source sripts/vivado/build_bd.sh
source sripts/vivado/generate_bit_hwh.sh
source sripts/vivado/timing_check.sh # make sure the implemented result meet timing requirement
```

### 4.2 Setup PYNQ

(1) Put the generated `end2end.bit` and `end2end.hwh` to a folder in PYNQ.

(2) Put our [PYNQ scripts](fpga/pynq_scripts) to the SAME script

(3) If this is the first time you run this demo, you need to config your diver:
- First, plug HDMI to ZCU104's HDMI-IN, plug display to ZCU104's HDMI-OUT, then run [host_setting.ipynb](fpga/host_setting.ipynb). This will help your GPU driver recognize the ZCU104's HDMI (it will be treated as a display).
- Turn off ANY augmentation on the display representing ZCU104, the reason is we want to send the unchanged Ellipsode data through HDMI port.
- Set that display to be 4K@60Hz since we will use 4K@60Hz bandwidth.

(4) Setup is done, run hdmi_close block in the script then close the notebook. (It is important or pynq will  likely crash)

### 4.3 Run the GPU+FPGA demo

(1) On the host, run: (You need to prepare `./decoded_images` as shown in 3.2 before run this )

```bash
cd <top_folder>
python3 scripts/pipeline_on_gpu_fpga/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --display_port 0 --foveated # change the display port the display representing ZCU104
```

After running the above code, you should see a Pygame window and a GUI like GPU demo.

See [<top_folder>/scripts/args.py](scripts/args.py) for all supported args. (--save_img is not supported here)

(2) On the PYNQ, run [board_demo.ipynb](fpga/host_setting.ipynb) , then you will se output on the display.

(3) You should see output on the display now. Don't forget to close HDMI before ending the demo.


## 5. FPS Results
### Variants:
- CPU : Parrallelized Numpy implementation.
- GPU : Cupy or Pytorch-GPU implementation.
- GPU + FPGA (Offload color_optimizer+Len_correction+Display_rendering to FPGA.)

### End to End FPS on different Coniguration

For each configuration there are two settings, one is SW is run sequentially, which will result in lower FPS, and another is that SW is run on ROS, which can enable parallelized pipelined computations and reach higher FPS. The below FPSs do not contain image loading time since we preload the image before running.

(1) Pipeline with: Projection → Len Correction → Ellipsoid prediction → Color optimizer (w/o Ellipsoid prediction). FPS is measured under a 1080x960 image. (need to add BD ENC / DEC)

| Config          | Squential SW (pipeline only \| whole loop w. display) | SW on ROS(pipeline ony / whole loop w. display) |
|:-----------------:|:-------------:|:-------------:|
| CPU (EPYC-Zen3)       | 0.66 \| 0.65 |TBA|
| GPU (RTX-4090)        | 51.0 \| 40.5 |TBA|
| GPU (RTX-4060 Laptop WSL) | 18.9 \| 17.1 |TBA|
| GPU + FPGA (ZCU104)   | TBA |TBA|

###  Modules' FPS on different Platforms
Below table shows FPS achieved used different HW.

| HW          | Projection | Len correction | Ellipsoid prediction | Color optimizer (w/o Ellipsoid prediction) | BD ENC | BD DEC
|:----------------:|:----------:|:--------------:|:--------------------:|:---------------:|:---------------:|:---------------:|
| CPU (EPYC-Zen3)   | 6          | 6.5            | 4.3                  | 1.3               | TBA | TBA
| GPU (RTX-4090)   | 1060       | 1246           | 275                  | 65.7              | TBA | TBA
| GPU (RTX-4060 Laptop WSL)   | 409.6    | 448.7            | 115.4                 | 26.7            | TBA | TBA
| FPGA (ZCU104)   | --      | TBA           | --                   | 288 *            | TBA | TBA

*GPU and CPU optimize both red and blue channel, FPGA only perform blue optimization now, but it only take around 20% of the FPGA resource.
