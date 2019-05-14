// gcc audio_test.c -o audio_test -lportaudio
// https://stackoverflow.com/questions/44645466/portaudio-real-time-audio-processing-for-continuous-input-stream

#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>

#define SAMPLE_RATE  (44100)
#define FRAME_SIZE   (1024)  // The lower the value, the worse the sound quality
#define NUM_CHANNELS (2)
#define BUFFER_SIZE (FRAME_SIZE * NUM_CHANNELS * 2)  // *2 since 1 for in buffer and 1 for out buffer

#define PA_SAMPLE_TYPE  paInt32
typedef int SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"

typedef struct
{
    int     readIdx;
    int     writeIdx;
	SAMPLE *buffer;
} paData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int streamCallback
( 
	const void 					   *inBuffer,
	void 	 					   *outBuffer,
	unsigned long 					framesPerBuffer,
	const PaStreamCallbackTimeInfo *timeInfo,
	PaStreamCallbackFlags 			statusFlags,
	void 						   *userData 
)
{
    paData       *data    = (paData*)userData;
    const SAMPLE *inBuff  = (const SAMPLE*)inBuffer;
    SAMPLE       *outBuff = (SAMPLE*)outBuffer;
    // write input buffer to our buffer to send for processing
    // memcpy(&data->buffer[writeIdx], inputBuffer, framesPerBuffer * NUM_CHANNELS); // assuming framesPerBuffer = FRAME_SIZE

    // increment write/wraparound
    // data->writeIdx = (data->writeIdx + FRAME_SIZE * NUM_CHANNELS) % BUFFER_SIZE;

	/* PROCESS DATA HERE */
	
    // write output
    for (int i = 0; i < framesPerBuffer; i++)
    {
        outBuff[i] = inBuff[i];
    }
    
    // increment read/wraparound
    // data->readIdx = (data->readIdx + FRAME_SIZE * NUM_CHANNELS) % BUFFER_SIZE;

    return paContinue;
}

/*******************************************************************/
int main(void)
{
    PaStreamParameters inParams;
    PaStreamParameters outParams;
    PaStream*          stream;
    paData             data;
    PaError            err = paNoError;
	
	data.readIdx  = 0;
	data.writeIdx = 0;
	data.buffer   = (SAMPLE*)calloc(BUFFER_SIZE, sizeof(SAMPLE));
	
    if (data.buffer == NULL)
    {
        printf("Could not allocate record array.\n");
        goto done;
    }

    err = Pa_Initialize();
    if (err != paNoError)
	{
		goto done;
	}
	
	// setup input parameters
	inParams.device = 2;  // i2s microphone
    if (inParams.device == paNoDevice) 
	{
        fprintf(stderr,"Error: coudn't open intput device %d.\n", inParams.device);
        goto done;
    }
	
    inParams.channelCount 			   = 2;  // stereo input
    inParams.sampleFormat 			   = PA_SAMPLE_TYPE;
    inParams.suggestedLatency 		   = Pa_GetDeviceInfo(inParams.device)->defaultLowInputLatency;
    inParams.hostApiSpecificStreamInfo = NULL;

	// setup output parameters
	outParams.device = 0;  // local (headphone jack)
    if (outParams.device == paNoDevice)
	{
        fprintf(stderr,"Error: coudn't open output device %d.\n", outParams.device);
        goto done;
    }
	
	outParams.channelCount     			= 2;  // stereo output
    outParams.sampleFormat     			= PA_SAMPLE_TYPE;
    outParams.suggestedLatency 			= Pa_GetDeviceInfo(outParams.device)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = NULL;
	
    // stream setup */
    err = Pa_OpenStream(
              &stream,
              &inParams,
              &outParams,
              SAMPLE_RATE,
              FRAME_SIZE,
              paClipOff,      // we won't output out of range samples so don't bother clipping them
              streamCallback,
              &data);
    if (err != paNoError)
	{
		goto done;
	}

	/* START */
    err = Pa_StartStream(stream);
    if(err != paNoError)
	{
		goto done;
	}

	// check if stream has finished
    while ((err = Pa_IsStreamActive(stream)) == 1)
    {
        Pa_Sleep(100);
    }
    if (err < 0)
	{
		goto done;
	}

    // clean up
    err = Pa_CloseStream(stream);
    if(err != paNoError)
	{
		goto done;
	}

    // write recorded data to a file
#if 0
    {
        FILE  *fid;
        fid = fopen("recorded.raw", "wb");
        if( fid == NULL )
        {
            printf("Could not open file.");
        }
        else
        {
            fwrite(data.buffer, NUM_CHANNELS * sizeof(float), BUFFER_SIZE, fid);
            fclose(fid);
            printf("Wrote data to 'recorded.raw'\n");
        }
    }
#endif

done:
    Pa_Terminate();
    if (data.buffer)
	{
        free(data.buffer);
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
