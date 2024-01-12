## Color Optimizer Examples
### How to run example usecase :
#### - (1) measure fps of ```color_optimizer``` and ```only_generate_ellipseson``` 1080x960 images 
#### - (2) optimize color of an 1080 x 1920 image

For CPU vrsion (about 1 fps / 4.3 fps on EPYC-Zen3 CPU), run:
```bash
python3 equirect_to_pespective_cpu.py
```

For GPU version (about 53 fps / 275 fps on RTX-4090), run:
```bash
python3 red_blue_optimization_cuda.py 
```


### Input Image v.s. Optimized Image
<p float="left">
  <img src="Images/orig/WaterScape.bmp" alt="Input Image" style="width: 40%; margin-right: 20px;" />
  <img src="Images/opt/WaterScape.bmp" alt="Output Image" style="width: 40%;" />
</p>



### Compression rate Improvement from example: 17.07%

```
original base delta compression rate: 21.31%
optimized base delta compression rate: 30.66%
compression improvement: 9.36%
```

