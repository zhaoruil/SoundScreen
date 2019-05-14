import numpy as np
import math
import contextlib
import pyaudio

from scipy.signal import freqz
from scipy.signal import butter, lfilter
import scipy.io.wavfile as wavfile

def butter_bandpass(lowcut, highcut, fs, order=5):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a

def butter_bandpass_filter(data, lowcut, highcut, fs, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

def interpret_wav(audio_data, n_frames, n_channels, sample_width, interleaved = True):

    if sample_width == 1:
        dtype = np.uint8 # unsigned char
    elif sample_width == 2:
        dtype = np.int16 # signed 2-byte short
    elif sample_width == 4:
        dtype = np.int32 # signed 4-byte int
    else:
        raise ValueError("Only supports 8, 16 and 32 bit audio formats.")


    if interleaved:
        # channels are interleaved, i.e. sample N of channel M follows sample N of channel M-1 in raw data
        audio_data.shape = (n_frames, n_channels)
        channels = audio_data.T
    else:
        # channels are not interleaved. All samples from channel M occur before all samples from channel M-1
        channels.shape = (n_channels, n_frames)

    return channels

def filter_audio(raw_data, n_frames, n_channels, sample_width, sample_rate):
    # channels is int32 type
    channels = interpret_wav(raw_data, n_frames, n_channels, sample_width, True)
    lowcut = 50
    highcut = 500
    filtered = butter_bandpass_filter(channels, lowcut, highcut, sample_rate, order=2).astype(channels.dtype)
    #print("size of filtered : " + str(len(filtered)))
    #print("size of 1 filtered channel : " + str(len(filtered[0])))

    # another way of interleaving outputs
    output = [val for pair in zip(filtered[0], filtered[1]) for val in pair]
    #print("size of output : " + str(len(output)))

    output = np.ascontiguousarray(output, dtype=np.int32)
    return output
