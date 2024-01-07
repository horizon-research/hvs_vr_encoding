# Gaze-Contingent Image/Video Encoding For Virtual Reality: An FPGA Demo

## 1. Overview

&nbsp; &nbsp; This project demonstrates color optimizer in the ASPLOS-2024 Paper [Exploiting Human Color Discrimination for Memory and Energy-Efficient Image Encoding in Virtual Reality](https://horizon-lab.org/pubs/asplos24-vr.pdf) on a FPGA board. This optimizer uses the limits of human color perception to reduce image color size. By subtly adjusting pixel colors within a humanly imperceptible range which depends on eccentricity, it brings colors of pixels closer, enhancing the efficiency of the following Base Delta compression algorithm. Finally, this foveated compression can make the system more memory and energy-efficient

### 1.1 Overall Pipeline

&nbsp; &nbsp; The figure illustrates the project's comprehensive pipeline, transforming panoramic video into the foveated compressed (Color Optimiz) video compatible with Google Cardboard. The pipeline is divided into two main module groups: one operating on the host machine and the other on an FPGA. The host machine handles video decoding, projection, parameter precomputation, and rearrangement. In contrast, the FPGA accelerates color adjustment and lens correction. These two platforms are interconnected via an HDMI cable. The final output is rendered on the VR display for immersive viewing.

<img src="doc_images/pipeline.png" alt="Alt text" width="800"/>

### 1.2 Examples of the foveated compressed output
<img src="doc_images/md_office.jpg" alt="Alt text" width="800"/>

## 2. Files Organization

- `host/`: Codes for the Host Machine.
    - `video_decode/`: codes for video decode.
    - `projection/`: codes for eqirectangular to binocular images projection.
    - `fpga_input_generation/`: codes for abc, dkl computaton and reaarangement.
    - `pygame/`: codes for sending image contains parameters through HDMI.
    - `len_correction/`: codes for len correction (implemented in software)
    - `color_optimizer`:  codes for color optimizer (implemented in software)
    - `full_pipeline_in_software/`: codes to run the full pipeline in software. Convenient for expected results observation and parameter configuration.


- `fpga/`:  Codes for FPGA.
    - `tile_color_optimizer_hls/`: HLS implementation of color optimizer.
    - `len_correction_hls/`: HLS implementation of len correction.
    - `rearrangment/`: Verilog and HLS implementation of reaarange ment IP (RIP) on FPGA.
    - `vivado/`: codes for generate and connect all other modules in a block design.
    - `pynq_scripts/`: Jupyter note code for control modules in run time using ARM on the fpga board. 

## 3. Usage of Software-Only Pipeline (CPU or GPU (CUDA) )

&nbsp; &nbsp;  This section is about how to run the full pipeline in software only manner. It is useful quick check of  expected result. It mainly contains below pipeline

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


(1) The scripts to run the whole pipeline for one frame is implemented in [scripts/pipeline_on_\<device\>/per_frame_seq_pipeline.py](scripts/pipeline_on_cpu/per_frame_seq_pipeline.py) (this link is to cpu imlemetation), please refer it to see how to use and concatenate all modules implemented in CPU. For CUDA (GPU) implementation, see the one under GPU folder[scripts/pipeline_on_gpu>/per_frame_seq_pipeline.py](scripts/pipeline_on_gpu/per_frame_seq_pipeline.py).

(2) Every module's main function also shows example of how to use it. You just need to go to the corresponding folder and run the python code. For example, the example code for projection is drawing the cube map and test FPS, you can run it as follow:
```bash
cd <top_folder>/host/projection
python3 equirect_to_pespective_cpu.py # if you want to use cuda acceleration, run: python3 equirect_to_pespective_cuda.py
```

(3) We provide scripts to run the full color optimizer pipeline in SW, including CPU, GPU implementations. All of them contain sequential loop implementation and ROS-like parrallel implementation. For CPU, we also provide multicore version. Here we only introduce how to run CPU implementation using single core and sequential loop.

For this project, the left and right eye images are exactly the same, since the input is a single equirectangular image, which supports only 3 DoF.  If the input video is captured in, for instance, an Omni-Directional Stereo (ODS) format, we could render actual stereo disparity.  See [this slide deck](https://cs.rochester.edu/courses/572/fall2022/decks/lect17-immersive.pdf) for details.  Because of this limitation, observers don't get depth perception from stereo disparity.

See [<top_folder>/scripts/args.py](scripts/args.py) for supported args.

- To run CPU implementation using single core and sequential loop. (For other settings please see readme in [scripts/](scripts/))
```bash 
cd <top_folder>
python3 scripts/pipeline_on_cpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --save_imgs

```

(4) After running the above codes, you will see output in [corrected_opt_images/](corrected_opt_images/) folder in main directory. (if you add --save_imgs)

### 3.5 Video Encoding
If you use CPU implementation, and you want to observe the realtime results. You can encode images back to video then playback it on your VR display in realtime.
```bash
cd <top_folder> 
python3 host/video_encode_decode/encode_images_to_video.py --video_path ./videos/opt_corrected_video.mp4 --images_folder ./corrected_opt_images --fps 30
```

The output video will be at ```./videos/opt_corrected_video.mp4```

## 4. Usage of SW-HW Pipeline (GPU-FPGA)

TO BE DONE
