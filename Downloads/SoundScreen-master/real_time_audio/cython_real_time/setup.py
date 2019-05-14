#!/usr/bin/env python3
# python3 setup.py build_ext --inplace

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import numpy

ext = [
    Extension(
	'audio_helper',
        ['audio_helper.pyx'],
        include_dirs = [numpy.get_include()],
        libraries = ['portaudio', 'pthread'],
        ),
]

ext_1 = [
    Extension(
	'audio_helper_1',
        ['audio_helper_1_callback.pyx'],
        libraries = ['portaudio',],
        ),
]

setup(
    name = 'audio_helper',
	ext_modules = cythonize(
		ext,
		compiler_directives = {
			'language_level': 3,
			'boundscheck': False,  # performance increase
			'wraparound': False  # performance increase
			},
		)
)

'''setup(
    name='test',
	ext_modules=cythonize('test.pyx')
)'''
