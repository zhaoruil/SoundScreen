#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "kiss_fft.c"
#include "tools/kiss_fftr.c"
#include "wave.h"

using namespace std;

namespace little_endian_io
{
  template <typename Word>
  std::ostream& write_word( std::ostream& outs, Word value, unsigned size = sizeof( Word ) )
  {
    for (; size; --size, value >>= 8)
      outs.put( static_cast <char> (value & 0xFF) );
    return outs;
  }
}

using namespace little_endian_io;
/**
 * Read and parse a wave file
 *
 **/

using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::string;

#define TRUE 1
#define FALSE 0
#define SAMPLES 1024

#ifndef M_PI
#define M_PI 3.14159265358979324
#endif

// WAVE header structure

unsigned char buffer4[4];
unsigned char buffer2[2];

char* seconds_to_time(float seconds);


void TestFftReal(kiss_fft_scalar* scal, kiss_fft_cpx* comp_num, int is_inverse_fft)
{
  kiss_fftr_cfg cfg;

  if ((cfg = kiss_fftr_alloc(SAMPLES, is_inverse_fft, NULL, NULL)) != NULL)
  {
    size_t i;
    if (!is_inverse_fft) {
        const kiss_fft_scalar* con_scal = scal;
        kiss_fftr(cfg, scal, comp_num);
    } else if (is_inverse_fft){
        const kiss_fft_cpx* con_comp_num = comp_num;
        kiss_fftri(cfg, con_comp_num, scal);
    }
    free(cfg);

    /*if(is_inverse_fft){
        for (i = 0; i < SAMPLES; i++)
        {
          printf(" scalar[%2zu] = %+f    ",
                 i, scal[i]);
          if (i < SAMPLES / 2 + 1)
            printf("out[%2zu] = %+f , %+f",
               i, comp_num[i].r, comp_num[i].i);
          printf("\n");
      }
  }*/

  }
  else
  {
    printf("not enough memory?\n");
    exit(-1);
  }

}

/**
 * Convert seconds into hh:mm:ss format
 * Params:
 *  seconds - seconds value
 * Returns: hms - formatted string
 **/
char* seconds_to_time(float raw_seconds) {
  char *hms;
  int hours, hours_residue, minutes, seconds, milliseconds;
  hms = (char*) malloc(100);

  sprintf(hms, "%f", raw_seconds);

  hours = (int) raw_seconds/3600;
  hours_residue = (int) raw_seconds % 3600;
  minutes = hours_residue/60;
  seconds = hours_residue % 60;
  milliseconds = 0;

  // get the decimal part of raw_seconds to get milliseconds
  char *pos;
  pos = strchr(hms, '.');
  int ipos = (int) (pos - hms);
  char decimalpart[15];
  memset(decimalpart, ' ', sizeof(decimalpart));
  strncpy(decimalpart, &hms[ipos+1], 3);
  milliseconds = atoi(decimalpart);


  sprintf(hms, "%d:%d:%d.%d", hours, minutes, seconds, milliseconds);
  return hms;
}


 FILE *ptr;
 char *filename;
 struct HEADER header;

int main(int argc, char **argv) {

 filename = (char*) malloc(sizeof(char) * 1024);
 if (filename == NULL) {
   printf("Error in malloc\n");
   exit(1);
 }

 // get file path
 char cwd[1024];
 if (getcwd(cwd, sizeof(cwd)) != NULL) {

    strcpy(filename, cwd);

    // get filename from command line
    if (argc < 2) {
      printf("No wave file specified\n");
      return 0;
    }

    strcat(filename, "/");
    strcat(filename, argv[1]);
    printf("%s\n", filename);
 }

 // open file
 printf("Opening  file..\n");
 ptr = fopen(filename, "rb");
 if (ptr == NULL) {
    printf("Error opening file\n");
    exit(1);
 }

 int read = 0;

 // read header parts

 read = fread(header.riff, sizeof(header.riff), 1, ptr);
 printf("(1-4): %s \n", header.riff);

 read = fread(buffer4, sizeof(buffer4), 1, ptr);
 printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 // convert little endian to big endian 4 byte int
 header.overall_size  = buffer4[0] |
                        (buffer4[1]<<8) |
                        (buffer4[2]<<16) |
                        (buffer4[3]<<24);

 printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size, header.overall_size/1024);

 read = fread(header.wave, sizeof(header.wave), 1, ptr);
 printf("(9-12) Wave marker: %s\n", header.wave);

 read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, ptr);
 printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

 read = fread(buffer4, sizeof(buffer4), 1, ptr);
 printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 // convert little endian to big endian 4 byte integer
 header.length_of_fmt = buffer4[0] |
                            (buffer4[1] << 8) |
                            (buffer4[2] << 16) |
                            (buffer4[3] << 24);
 printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

 read = fread(buffer2, sizeof(buffer2), 1, ptr); printf("%u %u \n", buffer2[0], buffer2[1]);

 header.format_type = buffer2[0] | (buffer2[1] << 8);
 char format_name[10] = "";
 if (header.format_type == 1)
   strcpy(format_name,"PCM");
 else if (header.format_type == 6)
  strcpy(format_name, "A-law");
 else if (header.format_type == 7)
  strcpy(format_name, "Mu-law");

 printf("(21-22) Format type: %u %s \n", header.format_type, format_name);

 read = fread(buffer2, sizeof(buffer2), 1, ptr);
 printf("%u %u \n", buffer2[0], buffer2[1]);

 header.channels = buffer2[0] | (buffer2[1] << 8);
 printf("(23-24) Channels: %u \n", header.channels);

 read = fread(buffer4, sizeof(buffer4), 1, ptr);
 printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 header.sample_rate = buffer4[0] |
                        (buffer4[1] << 8) |
                        (buffer4[2] << 16) |
                        (buffer4[3] << 24);

 printf("(25-28) Sample rate: %u\n", header.sample_rate);

 read = fread(buffer4, sizeof(buffer4), 1, ptr);
 printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 header.byterate  = buffer4[0] |
                        (buffer4[1] << 8) |
                        (buffer4[2] << 16) |
                        (buffer4[3] << 24);
 printf("(29-32) Byte Rate: %u , Bit Rate:%u\n", header.byterate, header.byterate*8);

 read = fread(buffer2, sizeof(buffer2), 1, ptr);
 printf("%u %u \n", buffer2[0], buffer2[1]);

 header.block_align = buffer2[0] |
                    (buffer2[1] << 8);
 printf("(33-34) Block Alignment: %u \n", header.block_align);

 read = fread(buffer2, sizeof(buffer2), 1, ptr);
 printf("%u %u \n", buffer2[0], buffer2[1]);

 header.bits_per_sample = buffer2[0] |
                    (buffer2[1] << 8);
 printf("(35-36) Bits per sample: %u \n", header.bits_per_sample);

 read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, ptr);
 printf("(37-40) Data Marker: %s \n", header.data_chunk_header);

 read = fread(buffer4, sizeof(buffer4), 1, ptr);
 printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 header.data_size = buffer4[0] |
                (buffer4[1] << 8) |
                (buffer4[2] << 16) |
                (buffer4[3] << 24 );
 printf("(41-44) Size of data chunk: %u \n", header.data_size);


 // calculate no.of samples
 long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
 printf("Number of samples:%lu \n", num_samples);

 long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
 printf("Size of each sample:%ld bytes\n", size_of_each_sample);

 // calculate duration of file
 float duration_in_seconds = (float) header.overall_size / header.byterate;
 printf("Approx.Duration in seconds=%f\n", duration_in_seconds);
 printf("Approx.Duration in h:m:s=%s\n", seconds_to_time(duration_in_seconds));



 // read each sample from data chunk if PCM
 if (header.format_type == 1) { // PCM
    printf("Dump sample data? Y/N?");
    char c = 'n';
    scanf("%c", &c);
    if (c == 'Y' || c == 'y') {
        long i =0;
        char data_buffer[size_of_each_sample*SAMPLES];
        int  size_is_correct = TRUE;

        // make sure that the bytes-per-sample is completely divisible by num.of channels
        long bytes_in_each_channel = (size_of_each_sample / header.channels);
        if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
            printf("Error: %ld x %ud <> %ld\n", bytes_in_each_channel, header.channels, size_of_each_sample);
            size_is_correct = FALSE;
        }

        if (size_is_correct) {
                    // the valid amplitude range for values based on the bits per sample
            long low_limit = 0l;
            long high_limit = 0l;

            switch (header.bits_per_sample) {
                case 8:
                    low_limit = -128;
                    high_limit = 127;
                    break;
                case 16:
                    low_limit = -32768;
                    high_limit = 32767;
                    break;
                case 32:
                    low_limit = -2147483648;
                    high_limit = 2147483647;
                    break;
            }


            ofstream wav( "example.wav", ios::binary );
            wav << "RIFF----WAVEfmt ";     // (chunk size to be filled in later)
            write_word( wav,     16, 4 );  // no extension data
            write_word( wav,      1, 2 );  // PCM - integer samples
            write_word( wav,      2, 2 );  // two channels (stereo file)
            write_word( wav,  16000, 4 );  // samples per second (Hz)
            write_word( wav, 64000, 4 );  // (Sample Rate * BitsPerSample * Channels) / 8
            write_word( wav,      4, 2 );  // data block size (size of two integer samples, one for each channel, in bytes)
            write_word( wav,     16, 2 );  // number of bits per sample (use a multiple of 8)
            // Write the data chunk header
            size_t data_chunk_pos = wav.tellp();
            wav << "data----";  // (chunk size to be filled in later)


            float ifft[num_samples/2];
            printf("\n\n.Valid range for data values : %ld to %ld \n", low_limit, high_limit);
            for (i =1; i <= num_samples/SAMPLES; i++) {

                read = fread(data_buffer, 2, SAMPLES, ptr);
                if (read <= SAMPLES) {
                    //printf("==========Sample %ld - %ld/ %ld=============\n", i*1024, i*1024+read, num_samples);
                    // dump the data read
                    unsigned int  xchannels = 0;
                    int data_in_channel[SAMPLES] = {0};
                    for (xchannels = 0; xchannels < header.channels; xchannels ++ ) {
                        //printf("Channel#%d : ", (xchannels+1));
                        // convert data from little endian to big endian based on bytes in each channel sample
                        for (int a = 0; a < read; a = a+2) {
                            if (bytes_in_each_channel == 4) {
                                data_in_channel[a] =   data_buffer[0] |
                                                    (data_buffer[1]<<8) |
                                                    (data_buffer[2]<<16) |
                                                    (data_buffer[3]<<24);
                            }
                            else if (bytes_in_each_channel == 2) {
                                data_in_channel[a] = data_buffer[a+0] |
                                                    (data_buffer[a+1] << 8);
                            }
                            else if (bytes_in_each_channel == 1) {
                                data_in_channel[a] = data_buffer[0];
                            }

                            //printf("%d ", data_in_channel[a]);

                            // check if value was in range
                            if (data_in_channel[a] < low_limit || data_in_channel[a] > high_limit)
                                printf("**value out of range\n");

                            //printf(" | ");
                        }

                        kiss_fft_scalar in[SAMPLES];
                        kiss_fft_cpx out[SAMPLES / 2 + 1];
                        for(int b=0; b<SAMPLES; b++)
                        {
                            in[b] = data_in_channel[b];
                        }

                        printf("Computing FFT\n");
                        TestFftReal(in, out, 0);
                        printf("Finished computing FFT\n");

                        /*Inverse Fourier Transform to WAV File Code*/

                        printf("Computing Inverse FFT\n");
                        // feeding fourier transform output into inverse
                        kiss_fft_scalar inverse_out[SAMPLES];
                        TestFftReal(inverse_out, out, 1);
                        printf("Finished inverse\n");
                        //ifft[512*i] = out;

                        for (int n = 0; n < SAMPLES; n++)
                        {
                            write_word( wav, (int)inverse_out[n], 2 );
                        }
                    }
                    printf("\n");
                }
                else {
                    printf("Error reading file. %d bytes\n", read);
                    break;
                }

            }
            size_t file_length = wav.tellp();

            // Fix the data chunk header to contain the data size
            wav.seekp( data_chunk_pos + 4 );
            write_word( wav, file_length - data_chunk_pos + 8 );

            // Fix the file header to contain the proper RIFF chunk size, which is (file size - 8) bytes
            wav.seekp( 0 + 4 );
            write_word( wav, file_length - 8, 4 );

     } // if (c == 'Y' || c == 'y') {
 }
}


 printf("Closing file..\n");
 fclose(ptr);

  // cleanup before quitting
 free(filename);
 return 0;

}
