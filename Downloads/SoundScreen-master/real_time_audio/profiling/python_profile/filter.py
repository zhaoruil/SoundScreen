import numpy as np
import wave
import sys
import math
import contextlib
import pyaudio

fname = 'chainsaw.wav'
outname = 'filtered.wav'

cutOffFrequency = 400.0

# from http://stackoverflow.com/questions/13728392/moving-average-or-running-mean
def running_mean(x, windowSize):
  cumsum = np.cumsum(np.insert(x, 0, 0)) 
  return (cumsum[windowSize:] - cumsum[:-windowSize]) / windowSize

def interpret_wav(raw_bytes, n_frames, n_channels, sample_width, interleaved = True):

    if sample_width == 1:
        dtype = np.uint8 # unsigned char
    elif sample_width == 2:
        dtype = np.int16 # signed 2-byte short
    else:
        raise ValueError("Only supports 8 and 16 bit audio formats.")

    channels = np.fromstring(raw_bytes, dtype=dtype)

    if interleaved:
        # channels are interleaved, i.e. sample N of channel M follows sample N of channel M-1 in raw data
        channels.shape = (n_frames, n_channels)
        channels = channels.T
    else:
        # channels are not interleaved. All samples from channel M occur before all samples from channel M-1
        channels.shape = (n_channels, n_frames)

    return channels

def filter_audio(data, nFrames, nChannels, nSampleWidth, sampleRate):
    channels = interpret_wav(data, nFrames, nChannels, nSampleWidth, True)

    # get window size
    # from http://dsp.stackexchange.com/questions/9966/what-is-the-cut-off-frequency-of-a-moving-average-filter
    freqRatio = (cutOffFrequency/sampleRate)
    N = int(math.sqrt(0.196196 + freqRatio**2)/freqRatio)

    #Use moving average
    filtered_data =  running_mean(channels, N).astype(channels.dtype)
    '''
    for i in range (0, nChannels):
        filtered[i] = running_mean(channels[i], N).astype(channels.dtype)
        '''
    return filtered_data
    '''
    #Make a new array result that will be able to streamed
    a = np.array([1,3,5])
    b = np.array([2,4,6])

    c = np.empty((filtered[0].size + filtred[1].size,), dtype=filtered[0].dtype)
    c[0::2] = filtered[0]
    c[1::2] = filtered[1]
    return c
    '''