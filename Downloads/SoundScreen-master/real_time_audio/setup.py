#!/usr/bin/env python3
# python3 setup.py build_ext --inplace

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import numpy

ext = [
    Extension(
	'bp_filter',
        ['bp_filter.pyx'],
        include_dirs = [numpy.get_include()],
        ),
]

setup(
    name = 'bp_filter',
	ext_modules = cythonize(
		ext,
		compiler_directives = {
			'language_level': 3,
			'boundscheck': False,  # performance increase
			'wraparound': False  # performance increase
			},
		)
)

