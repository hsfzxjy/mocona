import sys
from glob import glob

from setuptools import setup, Extension, find_packages
import distutils

sources = glob("mocona/csrc/*.c") + glob("mocona/csrc/*/*.c")

if distutils.ccompiler.get_default_compiler() == "unix":
    compile_args = ["-Wno-missing-braces"]
else:
    compile_args = []

_scopedvar_module = Extension(
    "mocona._scopedvar",
    sources=sources,
    extra_compile_args=compile_args,
)

setup(
    name="mocona",
    version="0.1.2",
    description="TODO",
    ext_modules=[_scopedvar_module],
    packages=find_packages(".", exclude=("*.so",)),
    package_data={"mocona": ["py.typed", "*.pyi"]},
)
