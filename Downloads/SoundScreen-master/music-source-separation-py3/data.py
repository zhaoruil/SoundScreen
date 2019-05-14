# -*- coding: utf-8 -*-
# !/usr/bin/env python
'''
By Dabi Ahn. andabi412@gmail.com.
https://www.github.com/andabi
'''

import random
import os
from config import ModelConfig
from preprocess import get_random_wav
from preprocess import get_random_wav_eval


class Data:
    def __init__(self, path, n_path, v_path):
        self.path = path
        self.n_path = n_path
        self.v_path = v_path

    def next_wavs(self, sec, size=1):
        wavfiles = []
        n_wavs = []
        v_wavs = []

        for (root, dirs, files) in os.walk(self.path):
            wavfiles.extend(['{}/{}'.format(root, f) for f in files if f.endswith(".wav")])
        # retrieve all noise and voice wav files in respective directories
        for (root, dirs, files) in os.walk(self.n_path):
            n_wavs.extend(['{}/{}'.format(root, f) for f in files if f.endswith(".wav")])
        for (root, dirs, files) in os.walk(self.v_path):
            v_wavs.extend(['{}/{}'.format(root, f) for f in files if f.endswith(".wav")])

        # retrieve one random wavfile from each directory
        wavfiles = random.sample(wavfiles, size)
        n_wavs = random.sample(n_wavs, size)
        v_wavs = random.sample(v_wavs, size)
        #print("n_wavs: {0}".format(n_wavs))
        #print("v_wavs: {0}".format(v_wavs))

        #mixed, src1, src2 = get_random_wav(wavfiles, sec, ModelConfig.SR)
        mixed, src1, src2 = get_random_wav(wavfiles, n_wavs, v_wavs, sec, ModelConfig.SR)

        return mixed, src1, src2, wavfiles

    def next_wavs_eval(self, sec, size=1):
        wavfiles = []
        n_wavs = []
        v_wavs = []
        for (root, dirs, files) in os.walk(self.path):
            wavfiles.extend(['{}/{}'.format(root, f) for f in files if f.endswith(".wav")])

        # retrieve one random wavfile from each directory
        wavfiles = random.sample(wavfiles, size)


        #mixed, src1, src2 = get_random_wav(wavfiles, sec, ModelConfig.SR)
        mixed, src1, src2 = get_random_wav_eval(wavfiles, sec, ModelConfig.SR)
        return mixed, src1, src2, wavfiles

    def get_wav_names(self):
        # retrieve one random wavfile from each directory
        wavfiles = random.sample(wavfiles, size)
