// gcc audio_record.h -lsndfile -lportaudio
// https://codereview.stackexchange.com/questions/84536/recording-audio-continuously-in-c/84633
// Alternative: https://codereview.stackexchange.com/questions/39521/recording-audio-in-c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <portaudio.h>
#include <sndfile.h>

#define FRAMES_PER_BUFFER 2048

typedef struct
{
    uint16_t formatType;
    uint8_t numberOfChannels;
    uint32_t sampleRate;
    size_t size;
    float *recordedSamples;
} AudioData;

typedef struct
{
    float *snippet;
    size_t size;
} AudioSnippet;

AudioData initAudioData(uint32_t sampleRate, uint16_t channels, int type)
{
    AudioData data;
    data.formatType = type;
    data.numberOfChannels = channels;
    data.sampleRate = sampleRate;
    data.size = 0;
    data.recordedSamples = NULL;
    return data;
}

/*! 
 * 1. RMS - avg (without calling fabs())
 * 2. HW acceleration?
 */
float avg(float *data, size_t length)
{
    float sum = 0;
    for (size_t i = 0; i < length; i++)
    {
        sum += fabs(data[i]));
    }
    return sum / length;
}

long storeFLAC(AudioData *data, const char *fileName)
{

    uint8_t err = SF_ERR_NO_ERROR;
    SF_INFO sfinfo =
    {
        .channels = data->numberOfChannels,
        .samplerate = data->sampleRate,
        .format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16
    };

    SNDFILE *outfile = sf_open(fileName, SFM_WRITE, &sfinfo);
    if (!outfile) return -1;

    // Write the entire buffer to the file
    long wr = sf_writef_float(outfile, data->recordedSamples, data->size / sizeof(float));
    err = data->size - wr;

    // Force write to disk and close file
    sf_write_sync(outfile);
    sf_close(outfile);
    puts("Wrote to file!!!!");
    return err;
}

/*! 
 * Given that you put emphasis on not dropping any audio data at all, I advise that you make this application threaded.
 * One thread would drive the reading of data into a circular buffer, and would perform tasks with negligible 
 * computational complexity (presumably, an RMS will take sufficiently little time as to never be the bottleneck). 
 * If this thread detects speech, it can message a worker thread to start reading this buffer at the given offset and 
 * encode it into a FLAC file. Of course, if the worker thread is too slow, eventually the reading thread will exhaust 
 * the circular buffer and will have to block, possibly dropping frames.
*/
int main(void)
{
    PaError 	  		err 	 = paNoError;
    PaStream 	 	   *stream   = NULL;
    PaDeviceIndex 		inDevId  = 2; // Pa_GetDefaultInputDevice();
    PaDeviceIndex 	    outDevId = 0; // Pa_GetDefaultOutputDevice();
    /*! Change to clock_gettime() */	
    time_t 				talking  = 0;
    time_t 				silence  = 0;
    
    const PaDeviceInfo *info 	 = Pa_GetDeviceInfo(inDevId);    
    AudioData 			data 	 = initAudioData(44100, info->maxInputChannels, paFloat32);
    
    AudioSnippet sampleBlock =
    {
        .snippet = NULL,
        .size = FRAMES_PER_BUFFER * sizeof(float) * data.numberOfChannels
    };
    
    sampleBlock.snippet = malloc(sampleBlock.size);
	
    PaStreamParameters inputParameters =
    {
        .device = inDevId,
        .channelCount = data.numberOfChannels,
        .sampleFormat = data.formatType,
        .suggestedLatency = info->defaultHighInputLatency,
        .hostApiSpecificStreamInfo = NULL
    };
    
    PaStreamParameters outputParameters =
    {
        .device = outDevId,
        .channelCount = data.numberOfChannels,
        .sampleFormat = data.formatType,
        .suggestedLatency = info->defaultHighInputLatency,
        .hostApiSpecificStreamInfo = NULL
    };

	if((err = Pa_Initialize()) == 0 &&
	   (err = Pa_OpenStream(&stream, &inputParameters, NULL, data.sampleRate, 
			FRAMES_PER_BUFFER, paClipOff, NULL, NULL)) == 0 &&
	   (err = Pa_StartStream(stream)) == 0)
	{
		free(sampleBlock.snippet);
		Pa_Terminate();
		return err;
	}
	
    for (int i = 0;;)
    {
        err = Pa_ReadStream(stream, sampleBlock.snippet, FRAMES_PER_BUFFER);
        if (err) 
        {
			goto done;
		}
		
        else if(avg(sampleBlock.snippet, FRAMES_PER_BUFFER) > 0.000550) // talking
        {
            printf("You're talking! %d\n", i);
            i++;
            time(&talking);
            data.recordedSamples = realloc(data.recordedSamples, sampleBlock.size * i);
            data.size = sampleBlock.size * i;
			
			/*! 
			 * You're memcpy()'ing a new block of data into your homebrew resizable vector, on top of any memcpy()'s inside realloc(). 
			 * Leaving aside the fact that every 23 ms of speech you'll potentially realloc() and copy the entire array of data 
			 * gathered so far, I seriously query the need for even moving any data at all. It should be possible for you to craft 
			 * yourself either a double buffer or, more generally, a circular buffer, and avoid any copying at all.

			 * And then on top of that, look at your conditional statement again. If data.recordedSamples is indeed NULL (and thus, 
			 * you've already leaked memory by loosing your last reference to that block of memory), why are you free()'ing said NULL 
			 * pointer, then setting said NULL pointer to NULL again? It is completely ineffectual.
			 *
             * Double buffering:
			 * while (keep_going)
			 * {
			 *		read_buffer(buf1);
			 *      swap(buf1, buf2);
			 *		write_nonblock(buf2);
			 * }
			 */
            if (data.recordedSamples) 
            {
				memcpy((char*)data.recordedSamples + ((i - 1) * sampleBlock.size), sampleBlock.snippet, sampleBlock.size);
			}
            else
            {
                free(data.recordedSamples);
                data.recordedSamples = NULL;
                data.size = 0;
            }
        }
        else //silence
        {
            double test = difftime(time(&silence), talking);
            if (test >= 1.5 && test <= 10)
            {
                char buffer[100];
                snprintf(buffer, 100, "file:%d.flac", i);
                storeFLAC(&data, buffer);
                talking = 0;
                free(data.recordedSamples);
                data.recordedSamples = NULL;
                data.size = 0;
            }
        }
    }

done:
    free(sampleBlock.snippet);
    Pa_Terminate();
    return err;
}
