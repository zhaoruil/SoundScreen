# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil

import numpy as np
import tensorflow as tf
import pyaudio

from threading import Thread, Semaphore
from queue import Queue

from config import EvalConfig, ModelConfig, TrainConfig
from model import Model
from preprocess import to_spectrogram, get_magnitude, get_phase, to_wav_mag_only, soft_time_freq_mask, to_wav, write_wav

r_sem = Semaphore()
p_sem = Semaphore()

def eval():
    p = pyaudio.PyAudio()
    input_signal = Queue()

    input_thread = Thread(target = record, args=(input_signal,p,))
    proc_thread = Thread(target = process, args=(input_signal,p,))

    input_thread.start()
    proc_thread.start()

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
        #r_sem.acquire()

        # do this as long as you want fresh samples
        data = stream.read(CHUNKSIZE, exception_on_overflow = False)
        recorded = np.fromstring(data, dtype=np.int32)
        mixed_wav = np.array([recorded])
        #print("Recorded: {0}".format(len(mixed_wav[0])))

        # if too many items in queue, clear it to put fresh data in
        #if input_signal.qsize() > 3:
        #    with input_signal.mutex:
        #        input_signal.queue.clear()

        #print("     record before put: %d" % input_signal.qsize())
        input_signal.put(mixed_wav)
        #print("     record after put: %d" % input_signal.qsize())

        #p_sem.release()

    # close stream
    stream.stop_stream()
    stream.close()

def process(input_signal,p):
    # Model
    model = Model()
    global_step = tf.Variable(0, dtype=tf.int32, trainable=False, name='global_step')
    CHUNKSIZE = EvalConfig.CHUNK
    stream = p.open(format = pyaudio.paInt32,
                channels = 1,
                rate = 48000,
                output = True,
                frames_per_buffer=CHUNKSIZE
    )

    with tf.Session(config=EvalConfig.session_conf) as sess:
        # Initialized, Load state
        sess.run(tf.global_variables_initializer())
        model.load_state(sess, EvalConfig.CKPT_PATH)

        while(True):
            #p_sem.acquire()
            #print("     process before get: %d" % input_signal.qsize())
            mixed_wav = input_signal.get()

            mixed_spec = to_spectrogram(mixed_wav)
            mixed_mag = get_magnitude(mixed_spec)
            mixed_batch, padded_mixed_mag = model.spec_to_batch(mixed_mag)
            mixed_phase = get_phase(mixed_spec)

            assert (np.all(np.equal(model.batch_to_spec(mixed_batch, EvalConfig.NUM_EVAL), padded_mixed_mag)))

            (pred_src1_mag, pred_src2_mag) = sess.run(model(), feed_dict={model.x_mixed: mixed_batch})

            seq_len = mixed_phase.shape[-1]
            pred_src1_mag = model.batch_to_spec(pred_src1_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]
            pred_src2_mag = model.batch_to_spec(pred_src2_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]

            # Time-frequency masking
            mask_src1 = soft_time_freq_mask(pred_src1_mag, pred_src2_mag)
            mask_src2 = 1. - mask_src1
            pred_src1_mag = mixed_mag * mask_src1
            pred_src2_mag = mixed_mag * mask_src2

            pred_src2_wav = to_wav(pred_src2_mag, mixed_phase)

            # free = stream.get_write_available()
            # print("         free1: %d" % free)
            # if (free - CHUNKSIZE) > CHUNKSIZE * 2:
            #      sleep(0.5)
            data = pred_src2_wav[0].astype(np.int32).tostring()
            stream.write(data)
            # free = stream.get_write_available()
            # print("         free2: %d" % free)
            #r_sem.release()
    stream.stop_stream()
    stream.close()

if __name__ == '__main__':
    eval()
