# Scripts for running pipeline on different HW

## Support configurations:
- CPU
- GPU
- GPU + FPGA

## Usage

### Run on CPU

TBA

### Run on GPU

TBA

### Run on FPGA

TBA

## End to End FPS on different Coniguration

Below shows End to End FPS of different configurations.

For each configuration there are two settings, one is SW is run seqentially, this will results in lower FPS, another is that SW is run on ROS, this can enable parrallelized pipelining and reach higher FPS.

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
