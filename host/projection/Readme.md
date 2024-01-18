## equirect_to_pespective.py Examples
### How to run example usecase (perspective cube and 1080x960 projection with fps measurement)
For CPU vrsion (about 6 fps on EPYC-Zen3 CPU), run:
```bash
python3 equirect_to_pespective_cpu.py
```

For GPU version (about 1060 fps on RTX-4090), run:
```bash
python3 equirect_to_pespective_cuda.py
```


### Example: Input Image v.s. Output perspective cube (in folder images/)
<p float="left">
  <img src="images/office.png" alt="Input Image" style="width: 400px; margin-right: 20px;" />
  <img src="images/cube_perspective_image.png" alt="Output Image" style="width: 300px;" />
</p>