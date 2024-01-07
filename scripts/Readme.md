# Scripts for running pipeline on different HW

## Support configurations:
- CPU (use cv2.imshow for display.)
- GPU (use HW-accelerated pygame for display.)
- GPU + FPGA (use HW-accelerated pygame for communication with FPGA.)

## Usage

make sure tou have prepared the decoded images as discussed in the readme of main folder

### Run on CPU
```bash
cd <top_folder>
python3 scripts/pipeline_on_cpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display
```
### Run on GPU
```bash
cd <top_folder>
python3 scripts/pipeline_on_gpu/per_frame_loop.py --in_images_folder ./decoded_images --out_images_folder ./corrected_opt_images --display --display_port 0
```
### Run on FPGA

TBA

## End to End FPS on different Coniguration

Below shows End to End FPS of different configurations.

For each configuration there are two settings, one is SW is run seqentially, this will results in lower FPS, another is that SW is run on ROS, this can enable parrallelized pipelining and reach higher FPS. The below FPSs do not contain image loading time since we preload the image before running.

| Config          | Squential SW (pipeline only \| whole loop w display) | SW on ROS(pipeline ony / whole loop w display) |
|:-----------------:|:-------------:|:-------------:|
| CPU (EPYC-Zen3)       | 0.66 \| 0.65 |TBA|
| GPU (RTX-4090)        | 43.46 \| 34.6 |TBA|
| GPU + FPGA (ZCU104)   | TBA |TBA|


## Modules' FPS on different Platform

Below table shows FPS achieved used different HW.

| HW          | Projection | Len correction | Ellipsoid prediction | Color optimizer (w/o Ellipsoid prediction) |
|:----------------:|:----------:|:--------------:|:--------------------:|:---------------:|
| CPU (EPYC-Zen3)   | 6          | 6.5            | 4.3                  | 1.3               |
| GPU (RTX-4090)   | 1060       | 1246           | 275                  | 65.7              |
| FPGA (ZCU104)   | --      | TBA           | --                   | 144            |
