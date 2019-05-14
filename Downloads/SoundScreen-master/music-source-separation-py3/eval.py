# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil
import time

import numpy as np
import tensorflow as tf
import pyaudio

from threading import Thread, Semaphore
from queue import Queue

from config import EvalConfig, ModelConfig
from model import Model
from preprocess import to_spectrogram, get_magnitude, get_phase, to_wav_mag_only, soft_time_freq_mask, to_wav, write_wav

r_sem = Semaphore()
p_sem = Semaphore()

def eval():
    p = pyaudio.PyAudio()
    input_signal = Queue(maxsize=0)
    processed_signal = Queue(maxsize=0)

    input_thread = Thread(target = record, args=(input_signal,p,))
    proc_thread = Thread(target = process, args=(input_signal, processed_signal,))
    output_thread = Thread(target = playback, args=(processed_signal,p,))

    print("Starting threads...")
    input_thread.start()
    proc_thread.start()
    output_thread.start()

def record(input_signal,p):
    CHUNKSIZE = EvalConfig.CHUNK
    stream = p.open(
                format=pyaudio.paInt32,
                channels=1,
                rate=48000,
                input=True,
                frames_per_buffer=CHUNKSIZE
    )

    while(True):

        # do this as long as you want fresh samples
        data = stream.read(CHUNKSIZE)
        recorded = np.fromstring(data, dtype=np.int32)
        mixed_wav = np.array([recorded])
        input_signal.put(mixed_wav)
    # close stream
    stream.stop_stream()
    stream.close()

def playback(processed_signal,p):
    CHUNKSIZE = EvalConfig.CHUNK
    stream = p.open(format = pyaudio.paInt32,
                channels = 1,
                rate = 48000,
                output = True,
                frames_per_buffer=CHUNKSIZE

    )
    while(True):
        separated = processed_signal.get()

        #play stream
        data = separated.astype(np.int32).tostring()
        stream.write(data)
        r_sem.release()

        processed_signal.task_done()

    # close stream
    stream.stop_stream()
    stream.close()

def process(input_signal, processed_signal):
    # Model
    model = Model()
    global_step = tf.Variable(0, dtype=tf.int32, trainable=False, name='global_step')

    with tf.Session(config=EvalConfig.session_conf) as sess:
        # Initialized, Load state
        sess.run(tf.global_variables_initializer())
        model.load_state(sess, EvalConfig.CKPT_PATH)

        while(True):
            start = time.time()
            mixed_wav = input_signal.get()

            mixed_spec = to_spectrogram(mixed_wav)
            mixed_mag = get_magnitude(mixed_spec)
            mixed_batch, padded_mixed_mag = model.spec_to_batch(mixed_mag)
            mixed_phase = get_phase(mixed_spec)

            assert (np.all(np.equal(model.batch_to_spec(mixed_batch, EvalConfig.NUM_EVAL), padded_mixed_mag)))

            input_signal.task_done()
            middle = time.time()
            (pred_src1_mag, pred_src2_mag) = sess.run(model(), feed_dict={model.x_mixed: mixed_batch})

            modeltime = time.time()
            seq_len = mixed_phase.shape[-1]
            pred_src1_mag = model.batch_to_spec(pred_src1_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]
            pred_src2_mag = model.batch_to_spec(pred_src2_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]
            middle2 = time.time()

            # Time-frequency masking
            mask_src1 = soft_time_freq_mask(pred_src1_mag, pred_src2_mag)
            mask_src2 = 1. - mask_src1
            pred_src1_mag = mixed_mag * mask_src1
            pred_src2_mag = mixed_mag * mask_src2

            # (magnitude, phase) -> spectrogram -> wav
            pred_src1_wav = to_wav(pred_src1_mag, mixed_phase)
            pred_src2_wav = to_wav(pred_src2_mag, mixed_phase)

            processed_signal.put(pred_src2_wav[0])
            end = time.time()

            print("Process time1 = {0}".format(middle-start))
            print("Process time to start model = {0}".format(modeltime-middle))
            print("Process time to models = {0}".format(middle2-modeltime))
            print("Process time till end = {0}".format(end-middle2))

if __name__ == '__main__':
    eval()
