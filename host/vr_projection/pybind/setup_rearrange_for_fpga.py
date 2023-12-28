from setuptools import setup, Extension
import sys
import os

from setuptools import setup, Extension
import os

os.environ['CC'] = 'aarch64-linux-gnu-gcc'
os.environ['CXX'] = 'aarch64-linux-gnu-g++'

# Determine current directory
current_dir = os.path.dirname(os.path.realpath(__file__))

# Include directories for pybind11 and Python headers
# Adjust these paths according to your environment
include_dirs = [
    os.path.join(current_dir, 'include'),  # If you have local includes
    '/home/lwk/.local/lib/python3.10/site-packages/pybind11/include',
]

# Define the extension module
rearrange_for_fpga_module = Extension(
    name='rearrange_for_fpga',  # Name of the module
    sources=['rearrange_for_fpga.cpp'],  # Source file
    extra_compile_args=["-O3", "-fPIC", "-std=c++11", '-mcpu=cortex-a53'],  # Extra arguments passed to the compiler
    include_dirs=include_dirs,
    extra_link_args=['-mcpu=cortex-a53']
)

# setup() call to register the module
setup(
    name='rearrange_for_fpga',  # Name of the package
    version='1.0',  # Version number
    description='A Python module for reading matrices from files using C++ and pybind11',  # Description
    ext_modules=[rearrange_for_fpga_module]  # List of extension modules
)