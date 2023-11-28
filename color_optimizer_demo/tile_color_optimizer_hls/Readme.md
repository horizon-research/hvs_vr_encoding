# HLS implementation for color optimizer

It is a fp32 implementation of the color optimizer, only perform the blue channel compression.  
**Fmax** = 300MHz, **Initial Interval** = 16, **FPS** = 144 images/sec  
(loopMin, loopMax are fp16 because data dependentcy cause it cannot run at 300MHz@fp32 )


## Files Organization

- `tile_color_optimizer_func_tb.cpp`: Contain Test_bench
- `tile_color_optimizer_func.h`: Contain hardware implementation
- `tile_color_optimizer_func.cpp`: Contain top-level function


## HLS reports

### Interface
![Alt text](imgs/interface.png)
### Latency
![Alt text](imgs/latency.png)
### Util in HLS (will be lower after vivado synth)
![Alt text](imgs/util_hls.png)
### Dataflow
![Alt text](imgs/data_flow.png)