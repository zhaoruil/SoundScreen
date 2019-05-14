# -*- coding: utf-8 -*-
#!/usr/bin/env python
'''
By Dabi Ahn. andabi412@gmail.com.
https://www.github.com/andabi
'''

import tensorflow as tf
from utils import closest_power_of_two

# TODO tf arg
# Model
class ModelConfig:
    SR = 48000                # Sample Rate
    L_FRAME = 1024           # default 1024
    L_HOP = closest_power_of_two(L_FRAME / 4)
    SEQ_LEN = 4
    # For Melspectogram
    N_MELS = 512
    F_MIN = 0.0

# Train
class TrainConfig:
    CASE = str(ModelConfig.SEQ_LEN) + 'frames_ikala'
    CKPT_PATH = 'Michel_Model_48kHz/' + CASE
    GRAPH_PATH = 'graphs/' + CASE + '/train'
    DATA_PATH = 'dataset/test'
    VOICE_DATA_PATH = '../../Training_Data/Michel'
    NOISE_DATA_PATH = '../../Training_Data/UrbanSound'
    LR = 0.0001
    #FINAL_STEP = 100000
    FINAL_STEP = 5001
    CKPT_STEP = 500
    NUM_WAVFILE = 1
    SECONDS = 8.192 # To get 512,512 in melspecto
    RE_TRAIN = True
    session_conf = tf.ConfigProto(
        device_count={'CPU': 1, 'GPU': 1},
        #intra_op_parallelism_threads=8,
        #inter_op_parallelism_threads=8,
        intra_op_parallelism_threads=0,
        inter_op_parallelism_threads=0,
        allow_soft_placement=True,
        gpu_options=tf.GPUOptions(
            allow_growth=False,
            per_process_gpu_memory_fraction=0.25
        ),
    )



# TODO seperating model and case
# TODO config for each case
# Eval
class EvalConfig:
    # CASE = '1frame'
    # CASE = '4-frames-masking-layer'
    CASE = str(ModelConfig.SEQ_LEN) + 'frames_ikala'
    CKPT_PATH = 'Dan_Model_48kHz/' + CASE
    #CKPT_PATH = ''
    GRAPH_PATH = 'graphs/' + CASE + '/eval'
    # DATA_PATH = 'dataset/eval/kpop'
    # DATA_PATH = 'dataset/mir-1k/Wavfile'
    # DATA_PATH = 'dataset/ikala'
    DATA_PATH = 'dataset/test'
    GRIFFIN_LIM = False
    GRIFFIN_LIM_ITER = 1000
    NUM_EVAL = 1
    SECONDS = 8
    RE_EVAL = True
    EVAL_METRIC = False
    WRITE_RESULT = True
    RESULT_PATH = 'results/output'
    CONT = False
    CHUNK = 16000
    #CHUNK = 10000 # use this for non real-time processing
    session_conf = tf.ConfigProto(
        device_count={'CPU': 1, 'GPU': 1},
        gpu_options=tf.GPUOptions(allow_growth=True),
        log_device_placement=False
    )
