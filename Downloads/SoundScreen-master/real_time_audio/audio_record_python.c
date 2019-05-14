// gcc audio_record_python.c -o arecord_py -lpthread -lportaudio $(python3-config --cflags) $(python3-config --ldflags) -I /usr/local/lib/python3.5/dist-packages/numpy/core/include

// ------------------- PROGRAM INCLUDES -------------------
#include <Python.h>
#include <numpy/arrayobject.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <portaudio.h>
#include <semaphore.h>
#include <signal.h>

// ------------------- PROGRAM DEFINES -------------------
#define SAMPLE_RATE       (48000) // higher means faster
#define FRAMES_PER_SAMPLE (4096)   // 530 is the lowest we can go. If we want higher, switch the code to use bit manipulation instead of modulus
#define NUM_CHANNELS      (2)
#define BUFFER_SIZE       (FRAMES_PER_SAMPLE * NUM_CHANNELS)
#define LATENCY           (0)

// data type
#define PA_SAMPLE_TYPE  paInt32
typedef int32_t SAMPLE;

// concurrency
sem_t readDone;
sem_t writeDone;
// pthread_mutex_t lock;

typedef struct
{
    uint32_t  recordIndex;
    uint32_t  playIndex;
    SAMPLE   *recordedSamples;
} paData;

// Python objects
PyObject *pName;
PyObject *pModule;
PyObject *pFunc;

void cleanPython(void);

// ------------------- FUNCTIONS -------------------

/*! Signal catcher to gracefully exit on Ctrl+C by deallocating everything
*/
void sigCatch(int sig)
{
    cleanPython();
    exit(0);
}

/*! Make a numpy array
*/
PyObject* makeNPArray
(
    int32_t *array
)
{
    npy_intp dim = BUFFER_SIZE;
    return PyArray_SimpleNewFromData(1, &dim, NPY_INT32, (void*)array);
}

/*! This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
*/
static int recordCallback
(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
)
{
    paData       *data = (paData*)userData;
    const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
    uint32_t      widx = 0; //data->recordIndex;
    SAMPLE       *wptr = &data->recordedSamples[widx];
    
    // wait for read to finish
    sem_wait(&readDone);

    // pthread_mutex_lock(&lock);
    for (uint32_t i = 0; i < FRAMES_PER_SAMPLE; i++)
    {
        //wptr[widx++ % BUFFER_SIZE] = *rptr++;  // left
        //wptr[widx++ % BUFFER_SIZE] = *rptr++;  // right
	wptr[widx++ & (BUFFER_SIZE - 1)] = *rptr++;
	wptr[widx++ & (BUFFER_SIZE - 1)] = *rptr++;
    }

    // pthread_mutex_unlock(&lock);

    // signal write is done
    sem_post(&writeDone);

    //data->recordIndex = widx % BUFFER_SIZE;
    //data->recordIndex = widx & (BUFFER_SIZE - 1);

    //exit(0);
    return paContinue;
}

/*! This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */
static int playCallback
(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
)
{
    paData           *data = (paData*)userData;
    uint32_t          ridx = 0; //data->playIndex;
    SAMPLE           *rptr = &data->recordedSamples[ridx];
    SAMPLE           *wptr = (SAMPLE*)outputBuffer;
    PyObject         *pArgs;
    PyObject         *pValue;
    PyGILState_STATE  gState;
    
    // wait for write to finish
    sem_wait(&writeDone);

    // call Python
    gState = PyGILState_Ensure();  // acquire GIL
    
    pArgs = PyTuple_New(5);
    
    pValue = makeNPArray(rptr);
    //printf("setting item 1\n");
    PyTuple_SetItem(pArgs, 0, pValue);
    //printf("setting item 2\n");
    // PyTuple_SetItem(pArgs, 1, PyLong_FromLong(100L));
    //printf("setting item 3\n");
    // PyTuple_SetItem(pArgs, 2, PyLong_FromLong(500L));

    PyTuple_SetItem(pArgs, 1, PyLong_FromLong(FRAMES_PER_SAMPLE));
    PyTuple_SetItem(pArgs, 2, PyLong_FromLong(2L));
    PyTuple_SetItem(pArgs, 3, PyLong_FromLong(4L));
    PyTuple_SetItem(pArgs, 4, PyLong_FromLong(48000L));

    if (!PyCallable_Check(pFunc))
    {
	printf("pFunc not callable\n");
    }

    pValue = PyObject_CallObject(pFunc, pArgs);  // call function

    // testing only
    if (pValue == NULL)
    {
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr, "Call failed\n");

        return paAbort;
    }

    /*FILE* f1 = fopen("c_out_pre.txt", "w");
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
	fprintf(f1, "%d\n", rptr[i]);
    }
    fclose(f1);*/

    rptr = PyArray_DATA(pValue);
   
    /*FILE* f1 = fopen("c_out_aft.txt", "w");
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        fprintf(f1, "%d\n", rptr[i]);
    }
    fclose(f1);*/

    Py_DECREF(pArgs);
    Py_DECREF(pValue);  // decrement reference count from PyObject creation

    PyGILState_Release(gState);  // release GIL
    
    /*f1 = fopen("c_out_rel.txt", "w");
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
	fprintf(f1, "%d\n", rptr[i]);
    }
    fclose(f1);*/
    
    //exit(0);

    // pthread_mutex_lock(&lock);
    for (uint32_t i = 0; i < FRAMES_PER_SAMPLE; i++ )
    {
        //*wptr++ = rptr[ridx++ % BUFFER_SIZE];  // left
        //*wptr++ = rptr[ridx++ % BUFFER_SIZE];  // right
	*wptr++ = rptr[ridx++ & (BUFFER_SIZE - 1)];
	*wptr++ = rptr[ridx++ & (BUFFER_SIZE - 1)];
    }
    // pthread_mutex_unlock(&lock);

    // signal read is done
    sem_post(&readDone);

    //data->playIndex = ridx % BUFFER_SIZE;
    //data->playIndex = ridx & (BUFFER_SIZE - 1);

    return paContinue;
}

void cleanPython()
{
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);

    Py_Finalize();
}

int main(void)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream          *inputStream;
    PaStream          *outputStream;
    paData             data;
    PaError            err = paNoError;

    // initialize & setup Python
    Py_Initialize();
    PyEval_InitThreads();  // initialize threads & acquire the GIL	

    PyRun_SimpleString("import os, sys");
    PyRun_SimpleString("sys.path.append(os.getcwd())");

    //pName   = PyUnicode_DecodeFSDefault("bandpass_filter");
    pName   = PyUnicode_DecodeFSDefault("freqfilter");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);  // need to manually decrement references to Python objects

    import_array();  // numpy types
    
    if (pModule == NULL)
    {
        PyErr_Print();
        fprintf(stderr, "Failed to load bandpass_filter.py\n");
        return -1;
    }

    //pFunc = PyObject_GetAttrString(pModule, "bp_filter");
    pFunc = PyObject_GetAttrString(pModule, "filter_audio");
    if (!pFunc || !PyCallable_Check(pFunc))
    {
        if (PyErr_Occurred())
        {
            PyErr_Print();
        }
        fprintf(stderr, "Cannot find function bp_filter\n");
    }

    Py_BEGIN_ALLOW_THREADS;  // release the GIL
    
    // setup signal handler
    signal(SIGINT, sigCatch);

    // initialize semaphores
    if (sem_init(&readDone, 0, 1) < 0 && sem_init(&writeDone, 0, 0) < 0)
    {
        fprintf(stderr, "Creating semaphore failed to initialize.\n");
        goto done;
    }

    /*if (pthread_mutex_init(&lock, NULL) < 0)
    {
        printf("Creating mutex failed to initialize.\n");
        goto done;
    }*/

    data.recordIndex     = 0;
    data.playIndex       = 0;
    data.recordedSamples = (SAMPLE*)calloc(BUFFER_SIZE, sizeof(SAMPLE));  // twice as large cause it seg faults otherwise. Don't know why
    if (data.recordedSamples == NULL)
    {
        fprintf(stderr, "Could not allocate record array.\n");
        goto done;
    }

    err = Pa_Initialize();
    if (err != paNoError)
    {
        goto done;
    }

    // input parameters
    inputParameters.device = 2;  // i2s microphone
    if (inputParameters.device == paNoDevice)
    {
        fprintf(stderr,"Error: No default input device.\n");
        goto done;
    }
    inputParameters.channelCount              = 2;
    inputParameters.sampleFormat              = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency          = LATENCY;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // output parameters
    outputParameters.device = 0;  // 3.5 mm out
    if (outputParameters.device == paNoDevice)
    {
        fprintf(stderr,"Error: No default output device.\n");
        goto done;
    }
    outputParameters.channelCount              = 2;
    outputParameters.sampleFormat              = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency          = LATENCY;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    // open input stream
    err = Pa_OpenStream(
              &inputStream,
              &inputParameters,
              NULL,  // no output
              SAMPLE_RATE,
              FRAMES_PER_SAMPLE,
              paClipOff,  // we won't output out of range samples so don't bother clipping them
              recordCallback,
              &data);
    if (err != paNoError)
    {
        goto done;
    }

    // open output stream
    err = Pa_OpenStream(
              &outputStream,
              NULL,  // no input
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_SAMPLE,
              paClipOff,  // we won't output out of range samples so don't bother clipping them
              playCallback,
              &data);
    if (err != paNoError)
    {
        goto done;
    }

    // start streams
    err = Pa_StartStream(inputStream);
    if (err != paNoError)
    {
        goto done;
    }

    err = Pa_StartStream(outputStream);
    if (err != paNoError)
    {
        goto done;
    }

    printf("\n=== Now recording ===\n");

    // check if streams have finished and sleep(ms)
    // want higher number to give more CPU time to processing
    while ((err = Pa_IsStreamActive(inputStream))  == 1 ||
           (err = Pa_IsStreamActive(outputStream)) == 1)
    {
        Pa_Sleep(100);
    }
    if (err < 0)
    {
        goto done;
    }

done:
    // cleanup
    err = Pa_CloseStream(inputStream);
    err = Pa_CloseStream(outputStream);
    Pa_Terminate();

    Py_END_ALLOW_THREADS;  // acquire the GIL before exiting
    cleanPython();

    sem_destroy(&readDone);
    sem_destroy(&writeDone);
    // pthread_mutex_destroy(&lock);

    if (data.recordedSamples)  // sure it is NULL or valid
    {
        free(data.recordedSamples);
    }
    if (err != paNoError)
    {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;  // always return 0 or 1, but no other return codes
    }

    return err;
}
