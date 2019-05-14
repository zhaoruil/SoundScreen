# to quit -> CTRL + \

# --------------- Program Includes ---------------
import numpy as np
cimport numpy as np
from scipy.signal import butter, lfilter
# from cython cimport view
from libc.math cimport sqrt, pow
from libc.stdint cimport uint16_t, uint32_t, int32_t

from libc.stdio cimport printf

# can't be in portaudio.h section or else Pa_OpenStream() can't see it
ctypedef int PaStreamCallback(
                const void *input,
                void *output,
                unsigned long frameCount,
                const PaStreamCallbackTimeInfo *timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData)

cdef extern from "<Python.h>":
    # structs & enums
    ctypedef enum PyGILState_STATE:
        PyGILState_LOCKED
        PyGILState_UNLOCKED
    ctypedef struct PyInterpreterState:
        pass
    ctypedef struct PyThreadState:
        PyInterpreterState *interp
    
    # functions
    void             Py_Initialize()
    void             PyEval_InitThreads()
    int              PyEval_ThreadsInitialized()
    PyThreadState*   PyEval_SaveThread()
    void             PyEval_RestoreThread(PyThreadState *state)
    PyGILState_STATE PyGILState_Ensure()
    void             PyGILState_Release(PyGILState_STATE state)

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

cdef extern from "<semaphore.h>" nogil:
    ctypedef struct sem_t:
        pass
    int sem_init(sem_t *sem, int pshared, unsigned int value)
    int sem_wait(sem_t *sem)
    int sem_post(sem_t *sem)
    int sem_destroy(sem_t *sem)   

# --------------- Program Variables ---------------
# pseudo macro defines
cdef enum:
    SAMPLE_RATE       = 48000  # higher sampling rate = less latency
    FRAMES_PER_SAMPLE = 530    # 530 is the lowest we can go
    NUM_CHANNELS      = 2
    CUTOFF_FREQ       = 400    # only for low pass filter
    CUTOFF_FREQ_LOW   = 100    # Hz
    CUTOFF_FREQ_HIGH  = 600    # Hz
    BUFFER_SIZE       = FRAMES_PER_SAMPLE * NUM_CHANNELS

    suggestedLatency  = 0
    paNoDevice        = -1
    paClipOff         = <PaStreamFlags>0x1

cdef:
    # audio data
    unsigned long paInt32  = <PaSampleFormat>0x00000002    
    #int32_t[::iview.continguous] recordedSamples
    int32_t[::1] recordedSamples

    # frequency filter
    float MAGIC_NUMBER = 0.196196

    # concurrency
    sem_t readDone
    sem_t writeDone

ctypedef np.int32_t sample_t
DTYPE = np.int32

# --------------- Functions ---------------
# Called by PortAudio whenever it wants to record audio
cdef int recordCallback(
    const void                     *inputBuffer,
    void                           *outputBuffer,
    unsigned long                   framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags           statusFlags,
    void                           *userData
):
    global recordedSamples

    cdef:
        const sample_t  *rptr = <const sample_t*>inputBuffer
        uint32_t         i    = 0  # typed index = faster array access

    # wait for recording audio to finish
    sem_wait(&readDone)
    
    printf("start record\n")

    # Storing audio data into recordedSamples (data to be filtered)
    while i < BUFFER_SIZE:
        recordedSamples[i]   = rptr[0]  # left
        rptr += 1

        recordedSamples[i+1] = rptr[0]  # right
        rptr += 1
        i    += 2
    
    # signal to be played is ready/done
    sem_post(&writeDone)

    printf("end record\n")

    return paContinue

# Called by PortAudio whenever it wants to play audio
cdef int playCallback(
    const void                     *inputBuffer,
    void                           *outputBuffer,
    unsigned long                   framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags           statusFlags,
    void                           *userData
):
    global recordedSamples

    cdef:
        sample_t *wptr = <sample_t*>outputBuffer
        uint32_t  i    = 0
        PyGILState_STATE gstate

    # wait for write to finish
    sem_wait(&writeDone)
    
    printf("play start\n")
    
    # acquire the GIL, call Python code, then release it
    gstate = PyGILState_Ensure()

    #filter_audio()
    print("filter")
    
    PyGILState_Release(gstate)

    printf("gil released\n")

    while i < BUFFER_SIZE:
        wptr[0] = recordedSamples[i]  # left
        wptr += 1
        
        wptr[0] = recordedSamples[i+1]  # right
        wptr += 1
        i    += 2

    # signal read is done
    sem_post(&readDone)

    printf("end play\n")

    return paContinue

cdef PaError cleanup(
    PaError err
):
    Pa_Terminate()

    sem_destroy(&readDone)
    sem_destroy(&writeDone)

    if err != paNoError:
        err_str = Pa_GetErrorText(err)

        print("An error occured while using the portaudio stream")
        print(f"Error number: {err}")
        print(f"Error message: {err_str}")
        err = 1  # always return 0 or 1, but no other return codes

    return err

# from http://stackoverflow.com/questions/13728392/moving-average-or-running-mean
# no return type needed for Python objects
# OLD FILTER
'''cdef running_mean(
    int32_t[:, :] matrix,
    uint16_t      window
):
    cumSum = np.cumsum(np.insert(matrix, 0, 0), dtype=DTYPE)

    return (cumSum[window:] - cumSum[:-window]) / window'''

cdef butter_bandpass(
    uint16_t order=5
    ):
    cdef:
        float nyq = 0.5 * SAMPLE_RATE
        float low = CUTOFF_FREQ_LOW / nyq
        float high = CUTOFF_FREQ_HIGH / nyq
    
    b, a = butter(order, [low, high], btype='bandpass')
    
    return b, a

cdef butter_bandpass_filter(
    np.ndarray[sample_t, ndim=2] data, 
    uint16_t order=5
):
    b, a = butter_bandpass(order=order)
    
    print("there")
    
    y = lfilter(b, a, data)
    
    print(y)

    #return y

cdef filter_audio():
    global recordedSamples

    '''cdef:
        float                freqRatio
        uint16_t             windowSize
        np.ndarray[sample_t, ndim=2] data
        #np.ndarray[sample_t, ndim=2] tmpCy'''
    
    #tmpPy = np.asarray(recordedSamples)

    #tmpPy = np.zeros(2, dtype=DTYPE)
    print("hey")

    # create matrix with interleaved channels (i.e. sample N of channel 2 follows sample N of channel 1)
    '''tmpCy = tmpPy.reshape(FRAMES_PER_SAMPLE, NUM_CHANNELS)
    data = tmpCy.T

    # get window size
    # from http://dsp.stackexchange.com/questions/9966/what-is-the-cut-off-frequency-of-a-moving-average-filter
    freqRatio = CUTOFF_FREQ / SAMPLE_RATE
    windowSize = <uint16_t>(sqrt(MAGIC_NUMBER + pow(freqRatio, 2)) / freqRatio)

    # moving average OLD FILTER
    # recordedSamples = running_mean(data, windowSize).astype(DTYPE)
    
    # butterworth bandpass filter
    #recordedSamples = butter_bandpass_filter(data, order=2).astype(DTYPE)

    print("here")

    butter_bandpass_filter(data, order=2)'''

    return

cpdef int main():
    global recordedSamples, playbackSamples

    cdef:
        PaStreamParameters  inputParameters
        PaStreamParameters  outputParameters
        PaStream*           inputStream
        PaStream*           outputStream
        PaError             err = paNoError

        PyThreadState      *threadState
        
    
    # initialize Python threads
    Py_Initialize() 
    PyEval_InitThreads()

    threadState = PyEval_SaveThread()

    printf("after save thread\n")

    if (sem_init(&readDone, 0, 1) < 0 and sem_init(&writeDone, 0, 0) < 0):
        printf("Creating semaphore failed to initialize.\n")
        err = cleanup(err)
        return err

    cdef np.ndarray[sample_t] npArr = np.zeros(BUFFER_SIZE, dtype=DTYPE, order='C')
    recordedSamples = npArr

    err = Pa_Initialize()
    if (err != paNoError):
        err = cleanup(err)
        return err

    # input parameters
    inputParameters.device = 2  # i2s microphone
    if (inputParameters.device == paNoDevice):
        printf("Error: No default input device.\n")
        err = cleanup(err)
        return err

    inputParameters.channelCount              = 2
    inputParameters.sampleFormat              = paInt32
    inputParameters.suggestedLatency          = suggestedLatency  # hard-coded since dereferencing doesn't exist here
    inputParameters.hostApiSpecificStreamInfo = NULL

    # output parameters
    outputParameters.device = 0  # local (headphone jack)
    if (outputParameters.device == paNoDevice):
        printf("Error: No default output device.\n")
        err = cleanup(err)
        return err

    outputParameters.channelCount              = 2
    outputParameters.sampleFormat              = paInt32
    outputParameters.suggestedLatency          = suggestedLatency  # hard-coded since dereferencing doesn't exist here
    outputParameters.hostApiSpecificStreamInfo = NULL

    # open input stream
    err = Pa_OpenStream(
              &inputStream,
              &inputParameters,
              NULL,  # no output
              SAMPLE_RATE,
              FRAMES_PER_SAMPLE,
              paClipOff,  # we won't output out of range samples so don't bother clipping them
              recordCallback,
              NULL)

    if (err != paNoError):
        err = cleanup(err)
        return err

    # open output stream
    err = Pa_OpenStream(
              &outputStream,
              NULL,  # no input
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_SAMPLE,
              paClipOff,  # we won't output out of range samples so don't bother clipping them
              playCallback,
              NULL)
    if (err != paNoError):
        err = cleanup(err)
        return err

    # start streams
    err = Pa_StartStream(inputStream)
    if (err != paNoError):
        err = cleanup(err)
        return err

    err = Pa_StartStream(outputStream)
    if (err != paNoError):
        err = cleanup(err)
        return err

    printf("\n=== Now recording ===\n")

    # check if streams have finished and sleep(ms)
    # want higher number to give more CPU time to processing
    while (Pa_IsStreamActive(inputStream) == 1 or Pa_IsStreamActive(outputStream) == 1):
        Pa_Sleep(1000)

    # cleanup
    PyEval_RestoreThread(threadState)
    
    err = Pa_CloseStream(inputStream)
    err = Pa_CloseStream(outputStream)

    err = cleanup(err)

    return err
