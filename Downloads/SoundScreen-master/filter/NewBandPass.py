import numpy as np
import wave
import sys
import math
import contextlib
import pyaudio
import time

fname = 'chainsaw.wav'
outname = 'filtered.wav'

cutOffFrequency = 200.0

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

with contextlib.closing(wave.open(fname,'rb')) as spf:
    sampleRate = spf.getframerate()
    ampWidth = spf.getsampwidth()
    nChannels = spf.getnchannels()
    nFrames = spf.getnframes()

    # Extract Raw Audio from multi-channel Wav File
    signal = spf.readframes(nFrames*nChannels)
    spf.close()
    channels = interpret_wav(signal, nFrames, nChannels, ampWidth, True)

    # get window size
    # from http://dsp.stackexchange.com/questions/9966/what-is-the-cut-off-frequency-of-a-moving-average-filter
    freqRatio = (cutOffFrequency/sampleRate)
    N = int(math.sqrt(0.196196 + freqRatio**2)/freqRatio)

    lowcut = 100
    highcut = 500
    f = open("profile_newbp.txt", "w")

    inputs = [[], []]
    
    inputs[0] = (channels[0])[100000:150000]
    inputs[1] = (channels[1])[100000:150000]

    # profile
    start = time.perf_counter()
    filtered = butter_bandpass_filter(inputs, lowcut, highcut, sampleRate, order=2).astype(channels.dtype)
    end = time.perf_counter()
    val = ((end - start) * 1000000)
    diff = str(val)
    # f.write(diff + "\n")

    output = []
    print(filtered)
    # interleave output
    for i in range(len(filtered[0])):
        output.append(filtered[0][i])
        output.append(filtered[1][i])
    
    output = np.asarray(output)

    wav_file = wave.open(outname, "w")
    wav_file.setparams((1, ampWidth, sampleRate, nFrames, spf.getcomptype(), spf.getcompname()))
    wav_file.writeframes(output.tobytes('C'))
    #wav_file.writeframes(filtered[1].tobytes('C'))
    wav_file.close()
