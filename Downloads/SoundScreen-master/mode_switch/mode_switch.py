# -*- coding: utf-8 -*-
# !/usr/bin/env python

import os
import shutil
import time

from threading import Thread
from queue import Queue

def eval():
    mode = Queue(maxsize=0)

    poll_thread = Thread(target = poll, args=(mode,))
    proc_thread = Thread(target = process, args=(mode,))

    poll_thread.start()
    proc_thread.start()

def poll(mode):
    while(True):
        curr_mode = input("Input mode")
        mode.put(curr_mode)

def process(mode):
    curr_mode = 1
    while(True):
        try:
            next_mode = mode.get_nowait()
        except:
            print("Continuing on mode: {0}".format(curr_mode))
        else:
            curr_mode = next_mode
            print("Now on mode: {0}".format(curr_mode))

if __name__ == '__main__':
    eval()
