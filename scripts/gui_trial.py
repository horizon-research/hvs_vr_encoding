import tkinter as tk
from tkinter import Scale
import time

import tkinter as tk

# 初始化主窗口
root = tk.Tk()
root.title("Parameter Adjustment")

# 用于存储所有滑块值的字典
slider_values = {
    "Roll": 0,
    "Yaw": 0,
    "Pitch": 0,
    "FOV": 80,
    "abc_scaler": 0.00
}

# 更新显示值并存储实际值的函数
def update_value(name, val, scale_min, scale_max):
    # 计算实际值
    actual_value = scale_min + (float(val) / 100) * (scale_max - scale_min)
    # 更新字典中的值
    slider_values[name] = actual_value
    # 更新标签显示
    value_labels[name].config(text=f"{actual_value:.2f}")

# 参数配置
parameters = [
    ("Roll", 0, 6.28),
    ("Yaw", 0, 6.28),
    ("Pitch", -1.57, +1.57),
    ("FOV", 80, 120),
    ("abc_scaler", 0.00, 2.0)
]

# 存储所有标签的字典
value_labels = {}

# 为每个参数创建滑块和标签
for name, min_val, max_val in parameters:
    # 参数名标签
    param_label = tk.Label(root, text=name)
    param_label.pack()

    # 显示滑块值的标签
    value_label = tk.Label(root, text=f"{min_val:.2f}")
    value_label.pack()
    value_labels[name] = value_label

    # 创建滑块
    slider = tk.Scale(root, from_=0, to=100, orient="horizontal", length=500, showvalue=False, 
                      command=lambda val, name=name, min_val=min_val, max_val=max_val: update_value(name, val, min_val, max_val))
    slider.pack()

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