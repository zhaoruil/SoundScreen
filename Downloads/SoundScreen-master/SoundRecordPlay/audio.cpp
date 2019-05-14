#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

#include "pulse/simple.h"
#include "pulse/error.h"
using namespace std;
#define BUFSIZE 2048
int error;

/* The Sample format to use */


class AudioCapt
{

public: 
    AudioCapt();
    //void RecordSound(char *argv[],pa_simple *s_in,pa_sample_spec &ss,pa_simple *s_out,uint8_t buf[],ssize_t r);
    void RecordSound(char *argv[],pa_simple *&s_in,pa_sample_spec &ss,pa_simple *&s_out,uint8_t buf[],ssize_t r);

    //void PlaybackSound(char*argv[],pa_simple *s_out,pa_sample_spec &ss,uint8_t buf[],ssize_t r);
    void PlaybackSound(char*argv[],pa_simple *&s_out,pa_sample_spec &ss,uint8_t buf[],ssize_t r);

    
};




void AudioCapt::RecordSound(char*argv[],pa_simple *&s_in,pa_sample_spec &ss,pa_simple *&s_out,uint8_t buf[],ssize_t r)
{

//printf("Audio Capturing \n");
  if (!(s_in = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
}

    if (pa_simple_read(s_in, buf, sizeof(uint8_t)*BUFSIZE, &error) < 0) {
        fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
}
printf("Buffer :::: %d\n",buf[0]);

}

void AudioCapt::PlaybackSound(char*argv[],pa_simple *&s_out,pa_sample_spec &ss,uint8_t buf[],ssize_t r)
{
//printf("Audio PlayBack \n");
//printf("Play Buffer::: %d\n",buf[0]);
if (!(s_out = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
}
//printf("Output Stream::: %d\n",s_out);

    /* ... and play it (Modified) */
   if (pa_simple_write(s_out, buf, sizeof(uint8_t)*BUFSIZE, &error) < 0) {
       fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
}

/* Make sure that every single sample was played */
if (pa_simple_drain(s_out, &error) < 0) {
    fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
}
}

int main(int argc, char * argv[])
{

pa_sample_spec ss;
ss.format = PA_SAMPLE_S16LE;
ss.rate = 44100;
    ss.channels = 1;

pa_simple *s_in, *s_out = NULL;

AudioCapt *m_pMyObject;

for(;;)
{ 
uint8_t buf[BUFSIZE];
    ssize_t r;

/*make the bottom 2 lines of this into threads to run them in parallel*/
m_pMyObject->RecordSound(argv,s_in,ss,s_out,buf,r);
m_pMyObject->PlaybackSound(argv,s_out,ss,buf,r);
}   
return 0;
}
