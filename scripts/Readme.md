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
### Run on GPU+FPGA

TBA

## End to End FPS on different Coniguration

For each configuration there are two settings, one is SW is run sequentially, which will result in lower FPS, and another is that SW is run on ROS, which can enable parallelized pipelining and reach higher FPS. The below FPSs do not contain image loading time since we preload the image before running.

(1) Pipeline with: Projection → Len Correction → Ellipsoid prediction → Color optimizer (w/o Ellipsoid prediction). FPS is measured under a 1080x960 image. (need to add BD ENC / DEC)

| Config          | Squential SW (pipeline only \| whole loop w. display) | SW on ROS(pipeline ony / whole loop w. display) |
|:-----------------:|:-------------:|:-------------:|
| CPU (EPYC-Zen3)       | 0.66 \| 0.65 |TBA|
| GPU (RTX-4090)        | 51.0 \| 40.5 |TBA|
| GPU (RTX-4060 Laptop WSL) | 18.9 \| 17.1 |TBA|
| GPU + FPGA (ZCU104)   | TBA |TBA|


## Modules' FPS on different Platforms

Below table shows FPS achieved used different HW.

| HW          | Projection | Len correction | Ellipsoid prediction | Color optimizer (w/o Ellipsoid prediction) | BD ENC | BD DEC
|:----------------:|:----------:|:--------------:|:--------------------:|:---------------:|:---------------:|:---------------:|
| CPU (EPYC-Zen3)   | 6          | 6.5            | 4.3                  | 1.3               | TBA | TBA
| GPU (RTX-4090)   | 1060       | 1246           | 275                  | 65.7              | TBA | TBA
| GPU (RTX-4060 Laptop WSL)   | 409.6    | 448.7            | 115.4                 | 26.7            | TBA | TBA
| FPGA (ZCU104)   | --      | TBA           | --                   | 288            | TBA | TBA
