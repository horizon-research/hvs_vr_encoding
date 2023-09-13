# hdmi_drawer.py shows how to draw image to a hdmi port


## now it only periodically read & display the images in folder Image_Set

### (1) Example command (Run in this folder):
```
python hdmi_drawer.py --display_mode 1920,1080 --display_port 1 --fps 1
```

### (2) Dependent Library
```
pip install pygame
pip install argparse
```

### (3) Supported args

```
def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--display_mode', type=str, default='1920, 1080', help='HDMI display mode (Resolution here)')
    parser.add_argument('--display_port', type=int, default='1', help='HDMI display port')
    parser.add_argument('--fps', type=int, default='1', help='frame rate')
    args = parser.parse_args()
    return args
```