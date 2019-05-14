# to quit -> CTRL + \

# --------------- Program Includes ---------------
import numpy as np
cimport numpy as np
from scipy.signal import butter, lfilter
from libc.stdint cimport uint16_t, int32_t

# np.import_array()

# --------------- Program Defines ---------------
cdef enum:
    SAMPLE_RATE       = 48000
    FRAMES_PER_SAMPLE = 1024
    NUM_CHANNELS      = 2
    BUFFER_SIZE       = FRAMES_PER_SAMPLE * NUM_CHANNELS

ctypedef np.int32_t sample_t
DTYPE = np.int32

# --------------- Functions -----------------
cdef butter_bandpass(
    uint16_t cutoff_freq_low,
    uint16_t cutoff_freq_high,
    uint16_t order
):
    cdef:
        float nyq = 0.5 * SAMPLE_RATE
        float low = cutoff_freq_low / nyq
        float high = cutoff_freq_high / nyq

    b, a = butter(order, [low, high], btype='bandpass')

    return b, a

cdef butter_bandpass_filter(
    np.ndarray[sample_t, ndim=2] data,
    uint16_t cutoff_low,
    uint16_t cutoff_high,
    uint16_t order=5
):
    b, a = butter_bandpass(cutoff_low, cutoff_high, order=order)
    y = lfilter(b, a, data)

    return y

# if having issues getting thing working, can just put this function 
# in Cython and have everything else in Python
cdef interleave_channels(
    np.ndarray[sample_t, ndim=2] channels
):
    cdef:
        np.ndarray[sample_t, mode="c"] audio = np.empty(BUFFER_SIZE, dtype=DTYPE)
        # int32_t  *audio_ptr = &audio[0]
        uint16_t  audio_idx = 0
        uint16_t  idx       = 0

    while idx < FRAMES_PER_SAMPLE:
        audio[audio_idx]     = channels[0][idx]
        audio[audio_idx + 1] = channels[1][idx]

        audio_idx += 2
        idx       += 1

    return audio

cpdef filter_audio(
    # int32_t  *data_C,
    np.ndarray[sample_t, mode="fortran"] recorded_samples,
    uint16_t cutoff_low,
    uint16_t cutoff_high
):

    cdef:
        # np.ndarray[sample_t, ndim=1, mode="c"] recorded_samples
        # audio data comes in fortran order, when you transpose, your new ndarray is c-ordered
        np.ndarray[sample_t, ndim=2, mode="c"] data
        np.ndarray[sample_t, ndim=2, mode="fortran"] tmp

    #print(recorded_samples.flags)
    # create matrix with interleaved channels (i.e. sample N of channel 2 follows sample N of channel 1)
    tmp  = recorded_samples.reshape((FRAMES_PER_SAMPLE, NUM_CHANNELS),order='F')

    #print(tmp.flags)
    tmp = tmp.copy(order='F')

    #print(tmp.flags)
    data = tmp.T

    #print("after transform")

    # butterworth bandpass filter
    #print("cutoff_low : " + str(cutoff_low))
    #print("cutoff_high : " + str(cutoff_high))
    data = butter_bandpass_filter(data, cutoff_low, cutoff_high, order=2).astype(DTYPE)
    
    #test = interleave_channels(data)
    
    return interleave_channels(data)
