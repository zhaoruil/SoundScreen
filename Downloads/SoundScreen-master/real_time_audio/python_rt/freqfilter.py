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
        #print(type(audio_data))
        audio_data.shape = (n_frames, n_channels)
        channels = audio_data.T
    else:
        # channels are not interleaved. All samples from channel M occur before all samples from channel M-1
        channels.shape = (n_channels, n_frames)

    return channels

def filter_audio(raw_data, n_frames, n_channels, sample_width, sample_rate, lowcut, highcut, oldvalue_r, oldvalue_l):
    # channels is int32 type
    #f_filtered = open("filtered.txt", 'w+')
    #f_raw = open("orignal.txt", 'w+')
    
    channels = interpret_wav(raw_data, n_frames, n_channels, sample_width, True)
    filtered = butter_bandpass_filter(channels, lowcut, highcut, sample_rate, order=2).astype(channels.dtype)
    n_wrongdata = int(300/4096*n_frames)
    filtered[0][0:n_wrongdata] = np.linspace(oldvalue_r, filtered[0][n_wrongdata+1], num=n_wrongdata)
    filtered[1][0:n_wrongdata] = np.linspace(oldvalue_l, filtered[1][n_wrongdata+1], num=n_wrongdata)
    #filtered[0] = [x*1.1 for x in filtered[0]]
    #filtered[1] = [x*1.1 for x in filtered[1]]
    '''
    filtered[0][0:300] = [0]*300
    filtered[0][-300:] = [0]*300
    filtered[1][0:300] = [0]*300
    filtered[1][-300:] = [0]*300
    f_raw.write('\n'.join(str(x) for x in channels[0]))
    f_filtered.write('\n'.join(str(x) for x in filtered[0]))
    '''
    #f_raw.close()
    #f_filtered.close()
    # Interleaving the outputs
    
    output = [val for pair in zip(filtered[0], filtered[1]) for val in pair]
    output = np.asarray(output)

    return output
