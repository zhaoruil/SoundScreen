# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil
import time

import numpy as np
import pyaudio
import threading

from threading import Thread
from queue import Queue

g_CHUNKSIZE = 1024
start = 0
end = 0
itr = 0
f = open("../profile_python.txt", "a")
sem_record = threading.BoundedSemaphore(value=1)
sem_playback = threading.Semaphore(0)
    
def eval():
    global f, itr
    
    p = pyaudio.PyAudio()
    #creates threadsafe queue for input and process signal
    input_signal = Queue(maxsize=0)

    
    #2 threads to input and output
    input_thread = Thread(target = record, args=(input_signal, p))
    output_thread = Thread(target = playback, args=(input_signal, p, f, itr))

    input_thread.start()
    output_thread.start()

def record(input_signal, p):
    global sem_record, sem_playback
    
    stream = p.open(input_device_index=2,
                format=pyaudio.paInt32,
                channels=2,
                rate=96000,
                input=True,
                frames_per_buffer=g_CHUNKSIZE)

    for _ in range(50):
        # acquire semaphore
        sem_record.acquire()
        # profile
        record_start = time.perf_counter()

        # do this as long as you want fresh samples
        data = stream.read(g_CHUNKSIZE)
        recorded = np.fromstring(data, dtype=np.int32)
        mixed_wav = np.array([recorded])
        
        input_signal.put(mixed_wav)
        sem_playback.release()
        record_end = time.perf_counter()
        record_timing = ((record_end - record_start) * 1000)
        record_diff = str(record_timing)
        f.write(record_diff + " record\n")


    # close stream
    stream.stop_stream()
    stream.close()

def playback(input_signal, p, f, itr):
    global sem_record, sem_playback
    
    stream = p.open(output_device_index=0, format=pyaudio.paInt32,
                channels=2,
                rate=96000,
                output=True,
                frames_per_buffer=g_CHUNKSIZE)
                
    for _ in range(50):
        sem_playback.acquire()
        separated = input_signal.get()
        playback_start = time.perf_counter()

        #play stream
        data = separated.astype(np.int32).tostring()
        stream.write(data)
        sem_record.release()
        playback_end = time.perf_counter()

        # profile
        playback_timing = ((playback_end - playback_start) * 1000000)
        playback_diff = str(playback_timing)
        f.write(playback_diff + " playback\n")
        
    #itr += 1
    #if itr == 10:
    f.close()
    #exit()

    input_signal.task_done()

    # close stream
    stream.stop_stream()
    stream.close()

if __name__ == '__main__':
    eval() 
