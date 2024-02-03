# Customized DMA

## In this folder we implement a DMA that is customized for irregular length DDR access, it is important for BD-encoder and BD-decoder to write to and read  from memory.

## Feature
- Support transaction-granuity memory access, every in/out transaction can contain a extra last bit indicate whether this transaction is 
- Burst Transmission: supported by hls::burst_maxi
- Double Buffering inside DDR
last of a frame.


```c++
struct dma_t
{
	data_t data;
	ap_uint<1> last;
};
```

## Configuration

You can config the data_width (data_t) and MaxBurstSize in dma.h at line 6 and line 7

## Limitation
- Csim works fine, however, Cosim doesn't support volatile input memory with burst acess as agument, which means the memory will only be read/write once at the start and end of Cosim, as a result, the update from Writer can't be propagate to reader, it cause that we need to initial the memory properly before start cosim, and we can only test 2 frames.(since we only have double buffer)
    - if we want to test the actual hardware behavior, we need to do it in Vivado, including writing a test pattern generator, concatenating it with DMA, and using BRAM to emulate DDR behavior.


