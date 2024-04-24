# Exploiting Human Color Discrimination for Memoryand Energy-Efficient Image Encoding in Virtual Reality: An FPGA Demo
[Weikai Lin](https://linwk20.github.io/)

## 1. Overview

This is an FPGA demonstration of the color discrimination-guided framebuffer compression for VR, which is described in our [ASPLOS 2024 paper](https://horizon-lab.org/pubs/asplos24-vr.pdf). Our algorithm leverages the (eccentricity-dependent) color discrimination of human visual system to bring pixel colors closer to each other and, thus, enhance the compression effectiveness of the existing Base Delta (BD) algorithm used in today's framebuffer compression.

### 1.1 Overall Pipeline

The figure illustrates the end-to-end system pipeline, which takes a panoramic (equirectangular) video, projects it to both eyes, and compresses the projected videos using our (numerically lossy but perceptually lossless) compression algorithm, which works on top of the BD algorithm.  For the description of a variant of the BD algorithm, see [this paper](https://dl.acm.org/doi/10.1145/3352460.3358298) (among other sources you can find online). The compressed video is displayed on a [Waveshare OLED](https://www.amazon.com/gp/product/B083BKSVNP/) compatible with [Google Cardboard](https://arvr.google.com/cardboard/).

### 1.2 A Sample Output
- Left: Original (BD compression rate = 34.62%), Right: Color-Optimized  (BD compression rate = 42.08%)

<img src="doc_images/compares.png" alt="Alt text" width="800"/>

## 2. Directory Organization


- [scripts/](scripts/) : Scripts for running demo.
    - [pipeline_on_cpu/](scripts/pipeline_on_cpu/): scripts for running software-only pipeline on CPU
    - [pipeline_on_gpu/](scripts/pipeline_on_gpu/): scripts for running software-only pipeline on GPU
    - [pipeline_on_gpu_fpga/](scripts/pipeline_on_gpu_fpga/): scripts for running pipeline on GPU + FPGA

- [host/](host/): Modules run on the host machine (CPU and/or GPU)
    - [projection/](host/projection/): perspective projection from eqirectangular images ([why such a projection is necessary and how it's done?](https://cs.rochester.edu/courses/572/fall2022/decks/lect17-immersive.pdf))
    - [len_correction/](host/len_correction/): lens correction
    - [color_optimizer/](host/color_optimizer/): color optimizer (the core of our compression algorithm)
    - [base_delta/](host/base_delta/): the baseline BD encoder and decoder
    - [video_processing/](host/video_processing/): video data pre/post-processing

- [fpga/](fpga/): Modules run on FPGA board (HLS implementations)
    - [tile_color_optimizer_hls/](fpga/tile_color_optimizer_hls/): color optimizer (the core of our compression algorithm)
    - [len_correction_hls/](fpga/len_correction_hls/): lens correction
    - [rearrangment_hls/](fpga/rearrangment_hls/): Verilog and HLS implementation of 4x4 to 1x1 rearrangement IP (RIP) on FPGA, which is used to convert data format between the color optimizer IP and the lens correction IP
    - [double_output_hls/](fpga/double_output_hls/): copy left-eye image to the right eye (yes, this demo doesn't provide stereo depth cue --- see later; also has the `t_last` signal needed by DMA)
    - [pynq_scripts/](fpga/pynq_scripts/): Jupyter notebook running on the PS in ZCU104
    - [end2end_bitstream/](fpga/end2end_bitstream/): pre-generated bitstream generated for the GPU+FPGA demo (so that you can skip building the project)
    - [ip_repo/](fpga/ip_repo/): exported HLS IPs needed by the GPU+FPGA demo
    - [BD_enc_hls/](fpga/BD_enc_hls/): BD encoder (not yet integrated into the pipeline)
    - [BD_dec_hls/](fpga/BD_dec_hls/): BD decoder (not yet integrated into the pipeline)
    - [dma_hls/](fpga/dma_hls/): a customized DMA (can deal with variable transaction size, etc., and is not yet integrated into the pipeline)
    - [vivado_scripts/](fpga/vivado_scripts/): scripts for generating and connecting all modules in the block design (TBD)


## 3. The Software-Only Pipeline (CPU and/or GPU/CUDA)

It is useful to run the pipeline in the software-only mode for quick visual debugging.  The pipeline is relatetively simple.

<img src="doc_images/pipeline_software_only.png" alt="Alt text" width="800"/>

### 3.1 Preparing equirectangular images
First you need to prepare a panoramic video encoded in the equirectangular format.  If you don't have one, you can download our videos [here](https://drive.google.com/drive/folders/1A16SSEeHIVEVFFKJjBaMvGev1zm46eUa?usp=sharing), which has all 6 videos used in the paper.

Then run:

```bash
cd <top_folder>
# Decode Video to a Folder of Images
python3 host/video_encode_decode/decode_video.py --video_path <downloaded_video.mp4> --out_images_folder ./decoded_images
```

Now you will find the decoded images in [decoded_images/](decoded_images/) in the main folder.

### 3.2 Run the compression pipeline

We provide scripts to run the full pipeline in SW, including a CPU only and a CPU-GPU implementation. All modules currently run sequentially. It can be extend to use a ROS-like interface to enable pipelining.

For simplicity, the left- and right-eye images are exactly the same, since the input is a single equirectangular image, which supports only 3 DoF.  If the input video is captured in, for instance, an Omni-Directional Stereo (ODS) format, we could render actual stereo disparity.  See [this slide deck](https://cs.rochester.edu/courses/572/fall2022/decks/lect17-immersive.pdf) for details.  Because of this limitation, observers don't get depth perception from stereo disparity.

1. The script to run the whole pipeline for one frame is implemented in `scripts/pipeline_on_<device>/per_frame_seq_pipeline.py`.  Read that file to see how to use and concatenate all modules together.

2. You can also trigger each individual module; each module's folder has a readme that describes briefly the function of each module and how to run it. For example, to trigger the persepective projection module, run:

```bash
cd <top_folder>/host/projection
python3 equirect_to_pespective_cpu.py # if you want to use cuda acceleration, run: python3 equirect_to_pespective_cuda.py
```

3. Here is how to run the whole pipeline in the two modes.  See [<top_folder>/scripts/args.py](scripts/args.py) for all supported args.

- For CPU:
```bash
cd <top_folder>
python3 scripts/pipeline_on_cpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --foveated --save_imgs
```

- For GPU:
```bash
cd <top_folder>
python3 scripts/pipeline_on_gpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --display_port 0 --foveated
```

For the GPU implementation, we also provide a GUI for tuning certain parameters in real-time.

<img src="doc_images/gui.png" alt="Input Image" style="width: 300px; margin-right: 20px;" />


4. After running the code above, you will see the compressed output in the [corrected_opt_images/](corrected_opt_images/) folder in main directory (if you've added the `--save_imgs` argument)

### 3.3 Video playback

Once you get the compressed output, you can encode the compressed images into a video and playback the video on your VR display in realtime:

```bash
cd <top_folder> 
python3 host/video_encode_decode/encode_images_to_video.py --video_path ./videos/corrected_opt_images.mp4 --images_folder ./corrected_opt_images --fps 30
```

The output video will be `./videos/corrected_opt_images.mp4`

## 4. The GPU-FPGA Pipeline

The overall pipeline is divided into two groups: one operating on the host machine and the other on an FPGA board (which in this demo is [Zynq UltraScale+ ZCU104](https://www.xilinx.com/products/boards-and-kits/zcu104.html)). The host machine and the FPGA board are interconnected via an HDMI cable, as is the connection between the FPGA board and the display.
This pipeline here is functionally very similar to the software pipeline, but looks much more complicated but it has a bunch of modules to deal with host-FPGA communication and inter-IP data conversion.

Two notes:
- The color optimizer is now split into two components: elliptical prediction, which runs on the host, and color adjustment, which runs on the FPGA.
- The IP blocks related to the baseline BD encoder/decoder are currently not integrated into the pipeline yet, but will be added soon enough.

<img src="doc_images/pipeline.png" alt="Alt text" width="800"/>

### 4.1 Setup vivado block design and get bitstream for FPGA

We provide the entire vivado project [here](https://drive.google.com/file/d/1ukujYRWgAs_QBbeWNI5sZ5nr2opewLNR/view?usp=drive_link) (1.6GB) which is used to generate the bitstream for ZCU104.  It's also useful if you want to check the detailed settings or use it to bootstrap your project.  If we get time, we will also share a script that's used to generate the vivado project (so that you don't need to download a giant file).  If you just want to run the code without having to generate the bistream youserlf, you can find the pre-generated `.bit` and `.hwh` files [here](fpga/end2end_bitstream/).

<!-- We provide fully automatic script that can build vivado block design and generate bitstream and hardware handoff file. See Readme in [sripts/vivado](sripts/vivado) for detailed tutorial and reminder. 
```bash
source sripts/vivado/build_bd.sh
source sripts/vivado/generate_bit_hwh.sh
source sripts/vivado/timing_check.sh # make sure the implemented result meet timing requirement
``` -->

### 4.2 Setup PYNQ

(1) Put the generated `end2end.bit` and `end2end.hwh` to a folder in ZCU104.

(2) Put our [PYNQ scripts](fpga/pynq_scripts) to the **same** folder.

(3) If this is the first time you run this demo, you need to first configure your system:
- First, connect your host machine with ZCU104's HDMI-IN port through an HDMI cable, and connect ZCU104's HDMI-OUT port with an external display, then run [host_setting.ipynb](fpga/host_setting.ipynb). This will help the GPU driver on your host machine recognize the ZCU104's HDMI.  Your FPGA will be recognized as an external display on your host machine.
- Turn off ANY video augmentation setting on the display representing ZCU104.  If you use Nvidia Control Panel on a Windows machine, for instance, the setting might look something like the screenshot below.  The reason is that we are using HDMI to transmit data between the host machine and the FPGA board, and you don't want the GPU driver on your host machine to muck about the data, which they usually do to "optimize visual experience".
- Set the display representing the FPGA to use 4K@60Hz since what the HDMI driver on the FPGA will use.

<img src="doc_images/display_setting.png" alt="Alt text" width="800"/>

(4) Now run the `hdmi_close` block in the PYNQ notebook and then close the notebook (which is important or PYNQ will likely crash).

### 4.3 Run the GPU+FPGA pipeline

(1) On the host, run the following (assuming you have prepared `./decoded_images` as discussed in 3.2). See [<top_folder>/scripts/args.py](scripts/args.py) for all supported args. (`--save_imgs` is not supported here)

```bash
cd <top_folder>
# since we add video switch function in gpu+fpga demo, need to cp the video to 
# folder with name <in_images_folderi> i=0 to video_num-1, the reason is our 
# script will automatically search the folder with name <in_images_folderi> i=0 to video_num-1
cp -r ./decoded_images ./decoded_images0
python3 scripts/pipeline_on_gpu_fpga/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --display_port 0 --foveated --video_num 1  # change the display port the display representing ZCU104
```

After running the code above, you should see a Pygame window and a GUI similar to that in the GPU demo but with a button to switch videos.

(2) In PYNQ, run [board_demo.ipynb](fpga/host_setting.ipynb).  You should see the video output on the display.

(3) You should see output on the display now. Don't forget to run the `hdmi_close` block in the PYNQ notebook before ending the demo.

## 5. Performance Measurement

### Variants:
- CPU : a combination of Numpy and Pytorch-CPU implementations.
- GPU : a combination of Cupy, Pytorch-GPU, or Numba implementations, whichever is faster.
- GPU + FPGA

### End-to-End FPS

The results do not include the image loading time since that's a one-time cost; we preload the images before running.  FPS is measured under a 1080x960 image.  The "display" in "pipeline+display" refers to the operation that sends the generated pixels to the display (which in software pipelines would be things like `cv2.show()` and in the hardware pipeline is simply taken care of by VDMA).

| Config          | Squential SW (pipeline only \| pipeline + display)
|:-----------------:|:-------------:|
| CPU (EPYC-Zen3)       | 0.66 \| 0.65 
| GPU1 (RTX-4090)        | 51.0 \| 40.5 
| GPU2 (RTX-4060 Mobile on WSL*) | 18.9 \| 17.1 
| GPU2  + FPGA (ZCU104)   | 29.76 |

<!-- | Config          | Squential SW (pipeline only \| whole loop w. display) | SW on ROS(pipeline ony / whole loop w. display) |
|:-----------------:|:-------------:|:-------------:|
| CPU (EPYC-Zen3)       | 0.66 \| 0.65 |TBA|
| GPU (RTX-4090)        | 51.0 \| 40.5 |TBA|
| GPU (RTX-4060 Mobile on WSL*) | 18.9 \| 17.1 |TBA|
| GPU + FPGA (ZCU104)   | TBA |TBA| -->

*: WSL is acronym for Windows Subsystem for Linux.

### FPS of individal modules

| HW          | Projection | Lens correction | Ellipsoid prediction (part of Color optimizer) | The rest of Color optimizer | BD ENC | BD DEC
|:----------------:|:----------:|:--------------:|:--------------------:|:---------------:|:---------------:|:---------------:|
| CPU (EPYC-Zen3)   | 6          | 6.5            | 4.3                  | 1.3               | 7.2 | 23.4
| GPU (RTX-4090)   | 1060       | 1246           | 275                  | 65.7              | 614.4 |  511.8
| GPU (RTX-4060 Mobile on WSL)   | 409.6    | 448.7            | 115.4    | 26.7            | 606.11 | 700.18†
| FPGA (ZCU104)   | --      | 144           | --                   | 288 *            | 288 | 280

*: GPU and CPU optimize both the red and blue channel, but the FPGA performs only the blue-channel optimization for now (basically a compression rate-vs-FPGA resource trade-off).

†: BD DEC on 4060 is faster than 4090.  The reason might be because of the CPU difference: our 4060 platform uses an i9-13900HX which has significantly higher single thread performance than the EPYC-Zen3 on our 4090 platform.
