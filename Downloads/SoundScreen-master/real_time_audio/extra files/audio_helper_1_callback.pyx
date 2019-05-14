# Works for a bit then the callback just stops getting called

# --------------- Program Includes ---------------
import numpy as np
cimport numpy as np
from libc.math cimport sqrt, pow
from libc.stdint cimport uint16_t, uint32_t, int32_t

# can't be in portaudio.h section or else Pa_OpenStream() can't see it
ctypedef int PaStreamCallback(
                const void *input,
                void *output,
                unsigned long frameCount,
                const PaStreamCallbackTimeInfo *timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData)

cdef extern from "<portaudio.h>" nogil:
# types
    ctypedef void          PaStream
    ctypedef unsigned long PaSampleFormat
    ctypedef int           PaError
    ctypedef int           PaDeviceIndex
    ctypedef double        PaTime
    ctypedef unsigned long PaStreamFlags

# structs & enums
    ctypedef enum   PaErrorCode:
        paNoError
    ctypedef enum   PaStreamCallbackResult:
        paContinue
        paComplete
        paAbort
    ctypedef struct PaStreamCallbackTimeInfo:
        pass
    ctypedef struct PaStreamCallbackFlags:
        pass
    ctypedef struct PaStreamParameters:
        PaDeviceIndex  device
        int            channelCount
        PaSampleFormat sampleFormat
        PaTime         suggestedLatency
        void*          hostApiSpecificStreamInfo
    ctypedef struct PaDeviceInfo:
        pass

# functions
    PaError             Pa_Initialize()
    PaError             Pa_Terminate ()
    PaError             Pa_OpenStream(
                            PaStream **stream,
                            const PaStreamParameters *inputParameters,
                            const PaStreamParameters *outputParameters,
                            double sampleRate,
                            unsigned long framesPerBuffer,
                            PaStreamFlags streamFlags,
                            PaStreamCallback *streamCallback,
                            void *userData)
    PaError             Pa_StartStream(PaStream *stream)
    PaError             Pa_IsStreamActive(PaStream *stream)
    PaError             Pa_CloseStream (PaStream *stream)
    const PaDeviceInfo* Pa_GetDeviceInfo(PaDeviceIndex device)
    const char *        Pa_GetErrorText(PaError errorCode)
    void                Pa_Sleep(long msec)

# --------------- Program Variables ---------------
# pseudo macro defines
cdef enum:
    SAMPLE_RATE       = 32000  # seems less noisy than 44100
    FRAMES_PER_SAMPLE = 530    # 530 is the lowest we can go
    NUM_CHANNELS      = 2
    CUTOFF_FREQ       = 400    # for the frequency filter in Hz
    BUFFER_SIZE       = FRAMES_PER_SAMPLE * NUM_CHANNELS

    suggestedLatency  = 0
    paNoDevice        = -1
    paClipOff         = <PaStreamFlags>0x1

cdef:
    # audio data
    unsigned long paInt32  = <PaSampleFormat>0x00000002    
    int32_t[:] recordedSamples

    # frequency filter
    float MAGIC_NUMBER = 0.196196

ctypedef np.int32_t sample_t
DTYPE = np.int32

# --------------- Functions ---------------
# Called by PortAudio whenever it wants to record audio
cdef int paCallback(
    const void                     *inputBuffer,
    void                           *outputBuffer,
    unsigned long                   framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags           statusFlags,
    void                           *userData
):
    global recordedSamples

    cdef:
        const sample_t *rptr = <const sample_t*>inputBuffer
        sample_t       *wptr = <sample_t*>outputBuffer

    for i in range(0, BUFFER_SIZE, 2):
        recordedSamples[i] = rptr[0]  # left
        rptr += 1

        recordedSamples[i + 1] = rptr[0]  # right
        rptr += 1

    for i in range(0, BUFFER_SIZE, 2):
        wptr[0] = recordedSamples[i]  # left
        wptr += 1
        
        wptr[0] = recordedSamples[i + 1]  # right
        wptr += 1

    return paContinue

cdef PaError cleanup(
    PaError err
):
    Pa_Terminate()

    if err != paNoError:
        err_str = Pa_GetErrorText(err)

        print("An error occured while using the portaudio stream")
        print(f"Error number: {err}")
        print(f"Error message: {err_str}")
        err = 1  # always return 0 or 1, but no other return codes

    return err

# from http://stackoverflow.com/questions/13728392/moving-average-or-running-mean
cdef running_mean(
    np.ndarray[sample_t, ndim=2] matrix,
    uint16_t                     window
):
    # no return type needed for Python objects
    cdef np.ndarray[sample_t] cumSum = np.cumsum(np.insert(matrix, 0, 0), dtype=DTYPE)

    return (cumSum[window:] - cumSum[:-window]) / window

# no return type needed for Python objects
cdef filter_audio():
    cdef:
        float    freqRatio
        uint16_t windowSize
        np.ndarray[sample_t] data = np.zeros(FRAMES_PER_SAMPLE * NUM_CHANNELS, dtype=DTYPE)


    '''if we have to do a copy, maybe using 2 small buffers would be better'''

    # copy just the samples we want
    # data = recordedSamples[playIndex:playIndex + FRAMES_PER_SAMPLE * NUM_CHANNELS]
    # create matrix with interleaved channels
    # i.e. sample N of channel 2 follows sample N of channel 1
    np.reshape(data, (FRAMES_PER_SAMPLE, NUM_CHANNELS))
    data = data.T

    # get window size
    # from http://dsp.stackexchange.com/questions/9966/what-is-the-cut-off-frequency-of-a-moving-average-filter
    freqRatio = CUTOFF_FREQ / SAMPLE_RATE
    windowSize = <uint16_t>(sqrt(MAGIC_NUMBER + pow(freqRatio, 2)) / freqRatio)

    # moving average
    running_mean(data, windowSize)

    return

cpdef int main():
    global             recordedSamples

    cdef:
        PaStreamParameters inputParameters
        PaStreamParameters outputParameters
        PaStream*          stream
        PaError            err = paNoError

    cdef np.ndarray[sample_t] npArr = np.zeros(BUFFER_SIZE * 2, dtype=DTYPE)  # twice as large cause it seg faults otherwise. Don't know why

    recordedSamples = npArr

    err = Pa_Initialize()
    if (err != paNoError):
        err = cleanup(err)
        return err

    # input parameters
    inputParameters.device = 2  # i2s microphone
    if (inputParameters.device == paNoDevice):
        print("Error: No default input device.")
        err = cleanup(err)
        return err

    inputParameters.channelCount              = 2
    inputParameters.sampleFormat              = paInt32
    inputParameters.suggestedLatency          = suggestedLatency  # hard-coded since dereferencing doesn't exist here
    inputParameters.hostApiSpecificStreamInfo = NULL

    # output parameters
    outputParameters.device = 0  # local (headphone jack)
    if (outputParameters.device == paNoDevice):
        print("Error: No default output device.")
        err = cleanup(err)
        return err

    outputParameters.channelCount              = 2
    outputParameters.sampleFormat              = paInt32
    outputParameters.suggestedLatency          = suggestedLatency  # hard-coded since dereferencing doesn't exist here
    outputParameters.hostApiSpecificStreamInfo = NULL

    # open stream
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_SAMPLE,
              paClipOff,  # we won't output out of range samples so don't bother clipping them
              paCallback,
              NULL)

    if (err != paNoError):
        err = cleanup(err)
        return err

    # start stream
    err = Pa_StartStream(stream)
    if (err != paNoError):
        err = cleanup(err)
        return err

    print()
    print("=== Now recording ===")
    print()

    # check if streams have finished and sleep(ms)
    # want higher number to give more CPU time to processing
    while (Pa_IsStreamActive(stream) == 1):
        Pa_Sleep(2000)

    # cleanup
    err = Pa_CloseStream(stream)

    err = cleanup(err)

    return err
