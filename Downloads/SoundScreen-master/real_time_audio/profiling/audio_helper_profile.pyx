# to quit -> CTRL + \

# --------------- Program Includes ---------------
import numpy as np
cimport numpy as np
from libc.math cimport sqrt, pow
from libc.stdint cimport uint16_t, uint32_t, int32_t
from libc.stdio cimport fprintf, FILE, fopen, fclose
# profiling
from libc.time cimport clock_t, CLOCKS_PER_SEC, clock

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
    SAMPLE_RATE       = 32000  # seems less noisy than 44100
    FRAMES_PER_SAMPLE = 530    # 530 is the lowest we can go
    NUM_CHANNELS      = 2
    CUTOFF_FREQ       = 400    # for the frequency filter in Hz
    BUFFER_SIZE       = FRAMES_PER_SAMPLE * NUM_CHANNELS

    suggestedLatency  = 0
    paNoDevice        = -1
    paClipOff         = <PaStreamFlags>0x1

    MAX_ITR           = 1000   # profiling

cdef:
    # audio data
    unsigned long paInt32  = <PaSampleFormat>0x00000002    
    int32_t[:] recordedSamples

    # frequency filter
    float MAGIC_NUMBER = 0.196196

    # concurrency
    sem_t readDone
    sem_t writeDone

    # profiling
    uint32_t itr = 0
    clock_t  start
    clock_t  end
    FILE    *out

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
    global recordedSamples, start

    cdef:
        const sample_t *rptr = <const sample_t*>inputBuffer

    # profile
    start = clock()

    # wait for read to finish
    sem_wait(&readDone)

    for i in range(0, BUFFER_SIZE, 2):
        recordedSamples[i] = rptr[0]  # left
        rptr += 1

        recordedSamples[i + 1] = rptr[0]  # right
        rptr += 1

    # filter_audio()
    
    # signal write is done
    sem_post(&writeDone)

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
    global recordedSamples, out, start, end, itr, MAX_ITR

    cdef:
        sample_t *wptr = <sample_t*>outputBuffer

    # wait for write to finish
    sem_wait(&writeDone)

    for i in range(0, BUFFER_SIZE, 2):
        wptr[0] = recordedSamples[i]  # left
        wptr += 1
        
        wptr[0] = recordedSamples[i + 1]  # right
        wptr += 1

    # signal read is done
    sem_post(&readDone)

    end = clock()
    fprintf(out, "%u\n", end - start);

    itr += 1
    if itr == MAX_ITR:
        fclose(out)
        exit()

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
cdef running_mean(
    np.ndarray[sample_t, ndim=2] matrix,
    uint16_t                     window
):
    # no return type needed for Python objects
    cdef np.ndarray[sample_t] cumSum = np.cumsum(np.insert(matrix, 0, 0), dtype=DTYPE)

    return (cumSum[window:] - cumSum[:-window]) / window

# no return type needed for Python objects
cdef filter_audio():
    global recordedSamples
    
    cdef:
        float    freqRatio
        uint16_t windowSize
        np.ndarray[sample_t] data = np.zeros(FRAMES_PER_SAMPLE * NUM_CHANNELS, dtype=DTYPE)

    # create matrix with interleaved channels
    # i.e. sample N of channel 2 follows sample N of channel 1
    np.reshape(recordedSamples, (FRAMES_PER_SAMPLE, NUM_CHANNELS))
    recordedSamples = recordedSamples.T

    # get window size
    # from http://dsp.stackexchange.com/questions/9966/what-is-the-cut-off-frequency-of-a-moving-average-filter
    freqRatio = CUTOFF_FREQ / SAMPLE_RATE
    windowSize = <uint16_t>(sqrt(MAGIC_NUMBER + pow(freqRatio, 2)) / freqRatio)

    # moving average
    recordedSamples = running_mean(recordedSamples, windowSize)

    return

cpdef int main():
    global recordedSamples, out

    cdef:
        PaStreamParameters inputParameters
        PaStreamParameters outputParameters
        PaStream*          inputStream
        PaStream*          outputStream
        PaError            err = paNoError

    # profiling
    out = fopen("profiling/profile_cython.txt", "w")
    fprintf(out, "sys clock: %d\n\n", CLOCKS_PER_SEC);

    '''for i in range(MAX_ITR):
        start = clock()
        end = clock()

        fprintf(out, "%u\n", end - start);

    fclose(out)
    exit()'''

    if (sem_init(&readDone, 0, 1) < 0 and sem_init(&writeDone, 0, 0) < 0):
        print("Creating semaphore failed to initialize.")
        err = cleanup(err)
        return err

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

    print()
    print("=== Now recording ===")
    print()

    # check if streams have finished and sleep(ms)
    # want higher number to give more CPU time to processing
    while (Pa_IsStreamActive(inputStream) == 1 or Pa_IsStreamActive(outputStream) == 1):
        Pa_Sleep(1000)

    # cleanup
    err = Pa_CloseStream(inputStream)
    err = Pa_CloseStream(outputStream)

    err = cleanup(err)

    return err
