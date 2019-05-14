#! /bin/bash

# compile Cython
python3 setup.py build_ext --inplace

# compile C
gcc audio_record_python.c -o arecord_py -lpthread -lportaudio $(python3-config --cflags) $(python3-config --ldflags) -I /usr/local/lib/python3.5/dist-packages/numpy/core/include -g
