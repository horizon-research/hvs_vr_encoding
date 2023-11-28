# HLS implementation for perceptual-color-compression encoder

Author: Weikai Lin;  &nbsp;  Date: 11/13/2023

## Overview

&nbsp; This HLS-implementation re-write the compression encoder part in ```./BaseDelta/red_blue_optimization.py``` to a HLS-friendly cpp-format.

## Input / Output

(1) Input: Ellipsode parameters and abc parameters of 4x4 tile.

(2) Output: the moved compressed sRGB colors.

## Files Organization
1. &nbsp; ``` encoder_tb.cpp ```: Test bench for encoder.
2. &nbsp; ```encoder.cpp``` : hardware implementation
3. &nbsp; ```encoder.h``` : header file to help ``` encoder_tb.cpp ``` find ```encoder.cpp```
4. &nbsp; ```golden_seq``` : folder include inputs and ouputs for functional test.
5. &nbsp; ```golden_seq``` 
6. &nbsp; ```BaseDelta```  : Reference python code.
7. &nbsp; ```scripts```  : folder contains scripts that can make process automatical.

## Usage

### 1. Environments Preparation

### 2. Simulation golden sequence preparation

### 3. Csim and Cosim

### 4. Sythesize the hardware

### 5. Integrate to Vivado Block design

## Progressing:

1. Reading the python codes and ASPLOS 2024 paper
2. Generating one tile gold seq


## TODOs
- [   ] One tile enoder
    - [   ] generate gold sequence for one tile
    - [   ] write tb for one tile
    - [   ] write hardware for one tile
    - [   ] pass one tile cosim and csim

- [   ] Real image enoder
    - [   ] generate gold sequence for whole image
    - [   ]  write tb for for whole image
    - [   ] pass  whole image cosim and csim

- [   ] Integrate to Vivado Block design
    - [  ] IP geneation
    - [  ] Cosim with whole stream in vivado block design




