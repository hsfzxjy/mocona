from glob import glob
from distutils.core import setup, Extension

_scopedvar_module = Extension(
    "mocona._scopedvar",
    sources=glob("mocona/csrc/*.c"),
    extra_compile_args=["-Wno-missing-braces"],
)

setup(
    name='mocona',
    version='0.1.0',
    description='TODO',
    ext_modules=[_scopedvar_module],
)
