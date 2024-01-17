import tkinter as tk
from tkinter import Scale
import time

import tkinter as tk

# init tkinter
root = tk.Tk()
root.title("Parameter Adjustment")

# Set the size of the window
window_width = 800
window_height = 600

# Get the screen width and height
screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()

# Calculate x and y coordinates for positioning the window
# Adjust these values based on your monitor's configuration

# Set the geometry of the window
root.geometry(f"{window_width}x{window_height}+{0}+{0}")


# Value dictionary
slider_values = {
    "Roll": 0,
    "Yaw": 0,
    "Pitch": 0,
    "Center FOV": 10,
    "Gradual Filter Period(s)": 5.00,
    "Horizontal FOV": 80,
    "abc_scaler": 1.00
}

# update the display value and store the actual value
def update_value(name, val, scale_min, scale_max):
    # calculate the actual value
    actual_value = scale_min + (float(val) / 100) * (scale_max - scale_min)
    # update the dictionary
    slider_values[name] = actual_value
    # update the display value
    value_labels[name].config(text=f"{actual_value:.2f}")

# Configure the parameters range
parameters = [
    ("Roll", 0, 6.28, 25),
    ("Yaw", 0, 6.28, 0),
    ("Pitch", -1.57, +1.57, 100),
    ("Center FOV", 0, 50, 20),
    ("Gradual Filter Period(s)", 0.00, 10.0, 50),
    ("Horizontal FOV", 80, 120, 0),
    ("abc_scaler", 0.00, 10.0, 50)
]

# store the labels in a dictionary
value_labels = {}

abc_scaler_slider = None

# create the sliders and labels
for name, min_val, max_val, default_v in parameters:
    # name label
    param_label = tk.Label(root, text=name)
    param_label.pack()

    # value label
    value_label = tk.Label(root, text=f"{min_val:.2f}")
    value_label.pack()
    value_labels[name] = value_label

    # slider
    slider = tk.Scale(root, from_=0, to=100, orient="horizontal", length=500, showvalue=False, 
                      command=lambda val, name=name, min_val=min_val, max_val=max_val: update_value(name, val, min_val, max_val))
    slider.set(default_v)
    slider.pack()

    if name == "abc_scaler":
        abc_scaler_slider = slider

running = True
def on_close():
    global running
    running = False

# Add an exit button
exit_button = tk.Button(root, text="Exit", command=on_close)
exit_button.pack()


running = True
def get_running():
    global running
    return running

def get_slider_values():
    global slider_values
    return slider_values

start_time = 0.0
gradual_filter = False
def set_gradual_filter():
    global gradual_filter
    gradual_filter = True
    global start_time
    start_time = time.time()

def upadate_gradual_filter():
    global gradual_filter
    global start_time
    if gradual_filter:
        if time.time() - start_time > slider_values["Gradual Filter Period(s)"]:
            gradual_filter = False
            start_time = 0.0
        else:
           abc_scaler = (time.time() - start_time) / slider_values["Gradual Filter Period(s)"]
           abc_scaler_slider.set(int(50*abc_scaler))

filtering_button = tk.Button(root, text="Start Gradual Filtering", command=set_gradual_filter)
filtering_button.pack()


use_uncompressed_image_on_right_eye = False

def set_use_uncompressed_image_on_right_eye():
    global use_uncompressed_image_on_right_eye
    use_uncompressed_image_on_right_eye = not use_uncompressed_image_on_right_eye

def get_use_uncompressed_image_on_right_eye():
    global use_uncompressed_image_on_right_eye
    return use_uncompressed_image_on_right_eye

use_uncompressed_image_on_right_eye_button = tk.Button(root, text="Uncompressed Image on Right (click twice to disable)", command=set_use_uncompressed_image_on_right_eye)
use_uncompressed_image_on_right_eye_button.pack()


# root.mainloop()


    