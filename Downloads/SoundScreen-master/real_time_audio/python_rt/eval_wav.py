# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil
import time
import freqfilter 

import numpy as np
import pyaudio
import soundfile as sf

from threading import Thread
from queue import Queue

g_CHUNKSIZE = 1024

def write_wav(data):
    sf.write('benis.wav', data, ModelConfig.SR, format='wav', subtype='PCM_16')

def eval():
    p = pyaudio.PyAudio()
    #creates threadsafe queue for input and process signal
    input_signal = Queue(maxsize=0)
    
    #2 threads to input and output
    input_thread = Thread(target = record, args=(input_signal, p))
    #output_thread = Thread(target = playback, args=(input_signal, p, f, itr))
    output_thread = Thread(target = playback, args=(input_signal, p))

    input_thread.start()
    output_thread.start()

def record(input_signal, p):
    
    stream = p.open(input_device_index=2,
                format=pyaudio.paInt32,
                channels=2,
                rate=48000,
                input=True,
                frames_per_buffer=g_CHUNKSIZE)
    while 1:
        # do this as long as you want fresh samples
        data = stream.read(g_CHUNKSIZE, exception_on_overflow = False)
        input_signal.put(data)

    # close stream
    stream.stop_stream()
    stream.close()

#def playback(input_signal, p, f, itr):
def playback(input_signal, p):
    
    stream = p.open(output_device_index=0, format=pyaudio.paInt32,
                channels=2,
                rate=48000,
                output=True,
                frames_per_buffer=g_CHUNKSIZE)
                
    to_wav = []

    #while 1:
    for i in range(100):
        # So input_signal.get() returns a 2D array of size 1x2048, where the information is interleaved channel1, channel2, channel1, channel2...
        separated = input_signal.get()
        print(len(separated))
        filtered_audio = freqfilter.filter_audio(separated[0], g_CHUNKSIZE, 2, 4, 48000)

        # play stream
        data = filtered_audio.astype(np.int32).tostring()
        #data = separated.astype(np.int32).tostring()
        
        to_wav.append(data)
        
        #stream.write(data)
        #stream.write(separated)
        input_signal.task_done()

    write_wav(to_wav)
    exit(0)
    
    # close stream
    stream.stop_stream()
    stream.close()

if __name__ == '__main__':
    eval() 
