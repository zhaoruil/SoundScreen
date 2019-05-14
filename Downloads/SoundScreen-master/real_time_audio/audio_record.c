// gcc audio_record.c -o audio_record -lportaudio -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <portaudio.h>
#include <semaphore.h>

// program defines
#define SAMPLE_RATE       (48000) // higher means faster
#define FRAMES_PER_SAMPLE (1024)   // 530 is the lowest we can go
#define NUM_CHANNELS      (2)
#define BUFFER_SIZE       (FRAMES_PER_SAMPLE * NUM_CHANNELS * 2)
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

/*! This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
*/
static int recordCallback
(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
)
{
    paData       *data = (paData*)userData;
    const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
    uint32_t      widx = data->recordIndex;
    SAMPLE       *wptr = &data->recordedSamples[widx];

    // wait for read to finish
    sem_wait(&readDone);
    
    // pthread_mutex_lock(&lock);
    for (uint32_t i = 0; i < framesPerBuffer; i++)
    {
        wptr[widx++ % BUFFER_SIZE] = *rptr++;  // left        
        wptr[widx++ % BUFFER_SIZE] = *rptr++;  // right
    }
    // pthread_mutex_unlock(&lock);
    
    // signal write is done
    sem_post(&writeDone);
    
    data->recordIndex = widx % BUFFER_SIZE;
    
    return paContinue;
}

/*! This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */
static int playCallback
( 
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
)
{   
    paData  *data = (paData*)userData;
    uint32_t ridx = data->playIndex; 
    SAMPLE  *rptr = &data->recordedSamples[ridx];
    SAMPLE  *wptr = (SAMPLE*)outputBuffer;

    // wait for write to finish
    sem_wait(&writeDone);
    
    // pthread_mutex_lock(&lock);
    for (uint32_t i = 0; i < framesPerBuffer; i++ )
    {
        *wptr++ = rptr[ridx++ % BUFFER_SIZE];  // left
        // printf("read: %d\n", i);
        
        *wptr++ = rptr[ridx++ % BUFFER_SIZE];  // right
        // printf("read: %d\n", rptr[ridx++]);
    }
    // pthread_mutex_unlock(&lock);

    // signal read is done
    sem_post(&readDone);

    data->playIndex = ridx % BUFFER_SIZE;

    return paContinue;
}

/*******************************************************************/
int main(void)
{
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    PaStream*          inputStream;
    PaStream*          outputStream;
    paData             data;
    PaError            err = paNoError;
    
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
    
    // twice as large cause it seg faults otherwise. Don't know why
    data.recordedSamples = (SAMPLE *) calloc(BUFFER_SIZE * 2, sizeof(SAMPLE));  
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
    outputParameters.device = 0;  // local (headphone jack)
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
        Pa_Sleep(1000);
    }
    if(err < 0)
    {
        goto done;
    }

    // cleanup
    err = Pa_CloseStream(inputStream);
    err = Pa_CloseStream(outputStream);

done:
    Pa_Terminate();

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
