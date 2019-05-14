# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil
import time
import freqfilter
import sys

import numpy as np
import pyaudio
import wave

from threading import Thread
from queue import Queue

g_CHUNKSIZE = 2048
g_CHUNKS = 40
#g_CHUNKSIZE =530
start = 0
end = 0
itr = 0

oldvalue_r = 0
oldvalue_l = 0

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

    record_f = wave.open("record.wav", mode='wb')
    #setparams([nchannels, sampwidth, framerate, nframes, comptype, companme])
    record_f.setparams([2, 4, 48000, g_CHUNKS*g_CHUNKSIZE, 'NONE', 'not compressed'])
    stream = p.open(input_device_index=5,
                format=pyaudio.paInt32,
                channels=2,
                rate=48000,
                input=True,
                frames_per_buffer=g_CHUNKSIZE)
    while 1:
    #for _ in range(g_CHUNKS):

        # do this as long as you want fresh samples
        data = stream.read(g_CHUNKSIZE, exception_on_overflow = False)
        recorded = np.fromstring(data, dtype=np.int32)
        mixed_wav = np.array([recorded])

        input_signal.put(mixed_wav)
        #input_signal.put(data)
        #record_f.writeframes(mixed_wav[0])
    #record_f.close()
    # close stream
    stream.stop_stream()
    stream.close()

#def playback(input_signal, p, f, itr):
def playback(input_signal, p):
    global oldvalue_r, oldvalue_l
    playback_f = wave.open("playback.wav", mode='wb') 
    playback_f.setparams([2, 4, 48000, g_CHUNKS * g_CHUNKSIZE, 'NONE', 'not compressed'])
    stream = p.open(output_device_index=0,
                format=pyaudio.paInt32,
                channels=2,
                rate=48000,
                output=True,
                frames_per_buffer=g_CHUNKSIZE)
    while 1:
    #for _ in range(g_CHUNKS):
        separated = input_signal.get()
        if sys.argv[1] == 'f':
            # So input_signal.get() returns a 2D array of size 1x2048, where the information is interleaved channel1, channel2, channel1, channel2...
            filtered_audio = freqfilter.filter_audio(separated[0], g_CHUNKSIZE, 2, 4, 48000, int(sys.argv[2]), int(sys.argv[3]), oldvalue_r, oldvalue_l)
            oldvalue_r = filtered_audio[-2]
            oldvalue_l = filtered_audio[-1]
            #playback_f.writeframes(filtered_audio)

            #play stream
            data = filtered_audio.astype(np.int32).tostring()

        elif sys.argv[1] == 'p':
            data = separated[0].astype(np.int32).tostring()
            
        #data = separated.astype(np.int32).tostring()
        stream.write(data)
        #stream.write(separated)
        input_signal.task_done()
        #playback_f.write(" ".join(str(x) for x in filtered_audio))

        #print("playback time: " + str(playback_timing))
    #f.close()


    # close stream
    stream.stop_stream()
    stream.close()

if __name__ == '__main__':
    eval()
