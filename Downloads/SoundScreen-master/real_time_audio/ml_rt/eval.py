# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil
import time

import numpy as np
import tensorflow as tf
import pyaudio

from threading import Thread
from queue import Queue

from config import EvalConfig, ModelConfig
from model import Model
from preprocess import to_spectrogram, get_magnitude, get_phase, to_wav_mag_only, soft_time_freq_mask, to_wav, write_wav

def interpret_wav(audio_data, n_frames, n_channels, sample_width, interleaved = True):

    if sample_width == 1:
        dtype = np.uint8 # unsigned char
    elif sample_width == 2:
        dtype = np.int16 # signed 2-byte short
    elif sample_width == 4:
        dtype = np.int32 # signed 4-byte int
    else:
        raise ValueError("Only supports 8, 16 and 32 bit audio formats.")


    if interleaved:
        # channels are interleaved, i.e. sample N of channel M follows sample N of channel M-1 in raw data
        audio_data.shape = (n_frames, n_channels)
        channels = audio_data.T
    else:
        # channels are not interleaved. All samples from channel M occur before all samples from channel M-1
        channels.shape = (n_channels, n_frames)

    return channels

def process(raw_data, n_frames, n_channels, sample_width, sample_rate):
    # Model
    model = Model()
    global_step = tf.Variable(0, dtype=tf.int32, trainable=False, name='global_step')

    # channels is int32 type
    channels = interpret_wav(raw_data, n_frames, n_channels, sample_width, True)

    mixed_wav = channels[0]
    with tf.Session(config=EvalConfig.session_conf) as sess:
        # Initialized, Load state
        sess.run(tf.global_variables_initializer())
        model.load_state(sess, EvalConfig.CKPT_PATH)

        mixed_spec = to_spectrogram(mixed_wav)
        mixed_mag = get_magnitude(mixed_spec)
        mixed_batch, padded_mixed_mag = model.spec_to_batch(mixed_mag)
        mixed_phase = get_phase(mixed_spec)

        (pred_src1_mag, pred_src2_mag) = sess.run(model(), feed_dict={model.x_mixed: mixed_batch})

        seq_len = mixed_phase.shape[-1]
        pred_src1_mag = model.batch_to_spec(pred_src1_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]
        pred_src2_mag = model.batch_to_spec(pred_src2_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]

        # Time-frequency masking
        mask_src1 = soft_time_freq_mask(pred_src1_mag, pred_src2_mag)
        mask_src2 = 1. - mask_src1
        pred_src2_mag = mixed_mag * mask_src2

        pred_src2_wav = to_wav(pred_src2_mag, mixed_phase)

        processed_signal.put(pred_src2_wav[0])

    return pred_src2_wav[0].astype(channels.dtype)
