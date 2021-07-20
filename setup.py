from glob import glob
from setuptools import setup, Extension, find_packages

_scopedvar_module = Extension(
    "mocona._scopedvar",
    sources=glob("mocona/csrc/*.c"),
    extra_compile_args=["-Wno-missing-braces"],
)

setup(
    name="mocona",
    version="0.1.0",
    description="TODO",
    ext_modules=[_scopedvar_module],
    packages=find_packages(".", exclude=("*.so",)),
    package_data={"mocona": ["py.typed", "*.pyi"]},
)
