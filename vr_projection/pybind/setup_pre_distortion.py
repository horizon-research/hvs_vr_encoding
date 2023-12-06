from setuptools import setup, Extension
import sys
import os

# Determine current directory
current_dir = os.path.dirname(os.path.realpath(__file__))

# Include directories for pybind11 and Python headers
# Adjust these paths according to your environment
include_dirs = [
    os.path.join(current_dir, 'include'),  # If you have local includes
    '/home/lwk/.local/lib/python3.10/site-packages/pybind11/include',
]

# Define the extension module
matrix_reader_module = Extension(
    name='pre_distortion',  # Name of the module
    sources=['pre_distortion.cpp'],  # Source file
    extra_compile_args=["-O3", "-fPIC", "-std=c++11"],  # Extra arguments passed to the compiler
    include_dirs=include_dirs
)

# setup() call to register the module
setup(
    name='pre_distortion',  # Name of the package
    version='1.0',  # Version number
    description='A Python module for reading matrices from files using C++ and pybind11',  # Description
    ext_modules=[matrix_reader_module]  # List of extension modules
)