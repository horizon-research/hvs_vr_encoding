import tkinter as tk
from tkinter import Scale
import time

import tkinter as tk

import tkinter as tk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import random
import time

# init tkinter
root = tk.Tk()
root.title("Parameter Adjustment")

# Set the size of the window
window_width = 1300
window_height = 1000

# Get the screen width and height
screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()

# Calculate x and y coordinates for positioning the window
# Adjust these values based on your monitor's configuration

# Set the geometry of the window
root.geometry(f"{window_width}x{window_height}+{0}+{0}")


# Value dictionary
slider_values = {
    "Roll (rad)": 0,
    "Yaw (rad)": 0,
    "Pitch (rad)": 0,
    "Center FOV  (deg)": 10,
    # "Gradual Filter Period(s)": 5.00,
    "Horizontal FOV (deg)": 80,
    "Ellipsoid Scale": 0.00
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
    ("Roll (rad)", 0, 6.28, 25),
    ("Yaw (rad)", 0, 6.28, 0),
    ("Pitch (rad)", -1.57, +1.57, 100),
    ("Center FOV (deg)", 0, 50, 20),
    # ("Gradual Filter Period(s)", 0.00, 10.0, 50),
    ("Horizontal FOV (deg)", 80, 120, 0),
    ("Ellipsoid Scale", 0.00, 10.0, 0)
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

# filtering_button = tk.Button(root, text="Start Gradual Filtering", command=set_gradual_filter)
# filtering_button.pack()


use_uncompressed_image_on_right_eye = False

def set_use_uncompressed_image_on_right_eye():
    global use_uncompressed_image_on_right_eye
    use_uncompressed_image_on_right_eye = not use_uncompressed_image_on_right_eye

def get_use_uncompressed_image_on_right_eye():
    global use_uncompressed_image_on_right_eye
    return use_uncompressed_image_on_right_eye

# use_uncompressed_image_on_right_eye_button = tk.Button(root, text="Uncompressed Image on Right (click twice to disable)", command=set_use_uncompressed_image_on_right_eye)
# use_uncompressed_image_on_right_eye_button.pack()



# Initialize lists to store data for each plot
record_length = 15
compression_rates = [None] * record_length
ellipsode_scales = [None] * record_length  # Assuming this is similar to compression_rates
timer = list(range(-record_length + 1, 1))


new_rate = None
new_scale = None

# Function to update the BD Compression Rate plot
def update_rate_and_plot():
    global new_scale
    global new_rate

    ellipsode_scales.append(new_scale)
    ellipsode_scales.pop(0)

    line2.set_ydata(ellipsode_scales)
    line2.set_xdata(timer)
    canvas2.draw()

    new_rate = new_rate  # Simulated data
    compression_rates.append(new_rate)
    compression_rates.pop(0)
    line1.set_ydata(compression_rates)
    line1.set_xdata(timer)
    canvas1.draw()

    root.after(1000, update_rate_and_plot)

# Function to update the Ellipsode Scale plot
def update_ellipsode_scale_and_plot():
    new_scale = slider_values["Ellipsoid Scale"]  # Simulated data
    ellipsode_scales.append(new_scale)
    ellipsode_scales.pop(0)



fig1, ax1 = plt.subplots()
canvas1 = FigureCanvasTkAgg(fig1, master=root)
widget1 = canvas1.get_tk_widget()
widget1.pack(side='left', padx=10, pady=10)

line1, = ax1.plot(timer, compression_rates, color="b", label='Compression Rate')  # Line object creation
ax1.set_title("BD Compression Rate Over Time")
ax1.set_xlabel("Time Steps")
ax1.set_ylabel("Compression Rate")
ax1.set_ylim([0, 1])
ax1.set_xlim([-14, 0])
ax1.legend(loc='upper right')

fig2, ax2 = plt.subplots()
canvas2 = FigureCanvasTkAgg(fig2, master=root)
widget2 = canvas2.get_tk_widget()
widget2.pack(side='right', padx=10, pady=10)

line2, = ax2.plot(timer, ellipsode_scales, color="r", label='Ellipsode Scale')
ax2.set_title("Ellipsode Scale Over Time")
ax2.set_xlabel("Time Steps")
ax2.set_ylabel("Ellipsode Scale")
ax2.set_ylim([0, 10])
ax2.set_xlim([-14, 0])
ax2.legend(loc='upper right')

root.after(1000, update_rate_and_plot)

# Start the Tkinter event loop
# root.mainloop()


def update_new_rate(rate):
    global new_rate
    new_rate = rate

def update_new_scale(scale):
    global new_scale
    new_scale = scale
    