# -*- coding: utf-8 -*-
# !/usr/bin/env python
'''
By Dabi Ahn. andabi412@gmail.com.
https://www.github.com/andabi
'''

import os
import shutil
import time

import numpy as np
import pyaudio
import wave
import filter

from threading import Thread
from queue import Queue

g_CHUNKSIZE = 1024
def eval():
    p = pyaudio.PyAudio()
    #creates threadsafe queue for input and process signal
    input_signal = Queue(maxsize=0)
    processed_signal = Queue(maxsize=0)

    #3 threads to input, process and output
    #target is the function name that is ran at .start()
    input_thread = Thread(target = record, args=(input_signal,p,))
    proc_thread = Thread(target = process, args=(input_signal, processed_signal,))
    output_thread = Thread(target = playback, args=(processed_signal,p,))

    input_thread.start()
    proc_thread.start()
    output_thread.start()

def record(input_signal,p):
    #CHUNKSIZE = 1024
    #CHUNKSIZE = EvalConfig.CHUNK
    stream = p.open(format=pyaudio.paInt16,
                channels=2,
                rate=96000,
                input=True,
                frames_per_buffer=g_CHUNKSIZE)

    while(True):

        # do this as long as you want fresh samples
        data = stream.read(g_CHUNKSIZE)
        recorded = np.fromstring(data, dtype=np.int16)
        mixed_wav = np.array([recorded])
        input_signal.put(mixed_wav)

    # close stream
    stream.stop_stream()
    stream.close()

def playback(processed_signal,p):
    stream = p.open(format = pyaudio.paInt16,
                channels = 2,
                rate = 96000,
                output = True,
                frames_per_buffer=g_CHUNKSIZE)
                
    while(True):
        separated = processed_signal.get()

        #play stream
        data = separated.astype(np.int16).tostring()
        stream.write(data)

        processed_signal.task_done()
    # close stream
    stream.stop_stream()
    stream.close()

def process(input_signal, processed_signal):
    # frequency filtering code
    while(True):
        frames_per_buffer = g_CHUNKSIZE
        channels = 1
        sample_width = 2
        sample_rate = 48000
        signal = input_signal.get()
        filtered_signal = filter.filter_audio(signal,frames_per_buffer, channels, sample_width, sample_rate)

        processed_signal.put(filtered_signal)
        input_signal.task_done()
'''
    model = Model()
    global_step = tf.Variable(0, dtype=tf.float32, trainable=False, name='global_step')

    with tf.Session(config=EvalConfig.session_conf) as sess:
        # Initialized, Load state
        sess.run(tf.global_variables_initializer())
        model.load_state(sess, EvalConfig.CKPT_PATH)

        while(True):
            mixed_wav = input_signal.get()

            mixed_spec = to_spectrogram(mixed_wav)
            mixed_mag = get_magnitude(mixed_spec)
            mixed_batch, padded_mixed_mag = model.spec_to_batch(mixed_mag)
            mixed_phase = get_phase(mixed_spec)

            assert (np.all(np.equal(model.batch_to_spec(mixed_batch, EvalConfig.NUM_EVAL), padded_mixed_mag)))

            input_signal.task_done()

            (pred_src1_mag, pred_src2_mag) = sess.run(model(), feed_dict={model.x_mixed: mixed_batch})

            seq_len = mixed_phase.shape[-1]
            pred_src1_mag = model.batch_to_spec(pred_src1_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]
            pred_src2_mag = model.batch_to_spec(pred_src2_mag, EvalConfig.NUM_EVAL)[:, :, :seq_len]

            # Time-frequency masking
            mask_src1 = soft_time_freq_mask(pred_src1_mag, pred_src2_mag)
            mask_src2 = 1. - mask_src1
            pred_src1_mag = mixed_mag * mask_src1
            pred_src2_mag = mixed_mag * mask_src2

            # (magnitude, phase) -> spectrogram -> wav
            if EvalConfig.GRIFFIN_LIM:
                pred_src1_wav = to_wav_mag_only(pred_src1_mag, init_phase=mixed_phase, num_iters=EvalConfig.GRIFFIN_LIM_ITER)
                pred_src2_wav = to_wav_mag_only(pred_src2_mag, init_phase=mixed_phase, num_iters=EvalConfig.GRIFFIN_LIM_ITER)
            else:
                pred_src1_wav = to_wav(pred_src1_mag, mixed_phase)
                pred_src2_wav = to_wav(pred_src2_mag, mixed_phase)

            processed_signal.put(pred_src2_wav[0])



def bss_eval_global(mixed_wav, src1_wav, src2_wav, pred_src1_wav, pred_src2_wav):
    len_cropped = pred_src1_wav.shape[-1]
    src1_wav = src1_wav[:, :len_cropped]
    src2_wav = src2_wav[:, :len_cropped]
    mixed_wav = mixed_wav[:, :len_cropped]
    gnsdr = gsir = gsar = np.zeros(2)
    total_len = 0
    for i in range(EvalConfig.NUM_EVAL):
        sdr, sir, sar, _ = bss_eval_sources(np.array([src1_wav[i], src2_wav[i]]),
                                            np.array([pred_src1_wav[i], pred_src2_wav[i]]), False)
        sdr_mixed, _, _, _ = bss_eval_sources(np.array([src1_wav[i], src2_wav[i]]),
                                              np.array([mixed_wav[i], mixed_wav[i]]), False)
        nsdr = sdr - sdr_mixed
        gnsdr += len_cropped * nsdr
        gsir += len_cropped * sir
        gsar += len_cropped * sar
        total_len += len_cropped
    gnsdr = gnsdr / total_len
    gsir = gsir / total_len
    gsar = gsar / total_len
    return gnsdr, gsir, gsar
'''
if __name__ == '__main__':
    eval()
