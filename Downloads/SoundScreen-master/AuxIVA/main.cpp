/* 
 *	https://stackoverflow.com/questions/13660777/c-reading-the-data-part-of-a-wav-file
 *	compile: g++ -std=c++11 read_wav.cpp
 *	run: ./a.out <input wav> <output file>
 *	
 *	Useful links:
 *		http://itpp.sourceforge.net/devel/fastica_8cpp_source.html (code for transforms and stuff?)
 *		https://github.com/teradepth/iva/blob/master/matlab/ivabss.m (MATLAB code for something)
 *		http://soundfile.sapp.org/doc/WaveFormat/ (WAVE file format)
 *		http://soundfile.sapp.org/ (sound file reading library)
 */

#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include "auxiva.h"

// how much do we want to read each time?
// @ 44.1 kHz, per channel, we read 1323 samples/30 ms & 882 samples/20 ms; 882 * 2 = 1764 rounded to nearest 2^x?
#define SET_UP_READ_SIZE 4096	// FFT_WINDOW size
#define SET_UP_BUFFER_SIZE (READ_SIZE / 2)

#define READ_SIZE 1024	// WINDOW_SHIFT size
#define BUFFER_SIZE (READ_SIZE / 2)

#define SAMPLING_RATE 44100

using namespace std; 

typedef struct  WAV_HEADER
{
    /* RIFF Chunk Descriptor */
    uint8_t         RIFF[4];        // RIFF Header Magic header
    uint32_t        ChunkSize;      // RIFF Chunk Size
    uint8_t         WAVE[4];        // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t         fmt[4];         // FMT header
    uint32_t        Subchunk1Size;  // Size of the fmt chunk
    uint16_t        AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t        NumOfChan;      // Number of channels 1=Mono 2=Stereo
    uint32_t        SamplesPerSec;  // Sampling Frequency in Hz
    uint32_t        bytesPerSec;    // bytes per second
    uint16_t        blockAlign;     // 2=16-bit mono, 4=16-bit stereo
    uint16_t        bitsPerSample;  // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t         Subchunk2ID[4]; // "data"  string
    uint32_t        Subchunk2Size;  // Sampled data length
} wav_hdr;

// Function prototypes
int getFileSize(FILE* inFile);

int main(int argc, char* argv[])
{
    wav_hdr wavHeader;
    int headerSize = sizeof(wav_hdr), filelength = 0;

    const char* filePath;
	const char* fileToSave;
    string input;
    if (argc <= 1)
    {
        cout << "Input wave file name: ";
        cin >> input;
        cin.get();
        filePath = input.c_str();
    }
    else
    {
        filePath = argv[1];
        cout << "Input wave file name: " << filePath << endl;
		
		fileToSave = argv[2];
		cout << "Output file name: " << fileToSave << endl;
    }

    FILE* wavFile = fopen(filePath, "r");
	FILE *fout = fopen(fileToSave, "w");
    if (wavFile == nullptr)
    {
        fprintf(stderr, "Unable to open wave file: %s\n", filePath);
        return 1;
    }

    //Read the header
    size_t bytesRead = fread(&wavHeader, 1, headerSize, wavFile);
    cout << "Header Read " << bytesRead << " bytes." << endl;
    if (bytesRead > 0)
    {
        //Read the data
        uint16_t bytesPerSample = wavHeader.bitsPerSample / 8;      //Number of bytes per sample
        uint64_t numSamples = wavHeader.ChunkSize / bytesPerSample; //How many samples are in the wav file?
        
		// 8-bit audio is unsigned, 16-bit+ is signed
		int16_t* set_up_buffer[SET_UP_READ_SIZE] __attribute__ ((aligned (4)));	// alignment necessary?
		int16_t* set_up_l_buf[SET_UP_BUFFER_SIZE] __attribute__ ((aligned (4)));	// https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Type-Attributes.html#Type%20Attributes
		int16_t* set_up_r_buf[SET_UP_BUFFER_SIZE] __attribute__ ((aligned (4)));
		
		int16_t* buffer[READ_SIZE] __attribute__ ((aligned (4)));	// alignment necessary?
		int16_t* l_buf[BUFFER_SIZE] __attribute__ ((aligned (4)));	// https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Type-Attributes.html#Type%20Attributes
		int16_t* r_buf[BUFFER_SIZE] __attribute__ ((aligned (4)));
		
		int16_t* l_res[BUFFER_SIZE] __attribute__ ((aligned (4)));
		int16_t* r_res[BUFFER_SIZE] __attribute__ ((aligned (4)));
		
		// read FFT_WINDOW samples first time to fill buffers
		// fread reads READ_SIZE number of sizeof(buffer[0])-sized elements from the source
		if (fread(set_up_buffer, sizeof(set_up_buffer[0]), SET_UP_READ_SIZE, wavFile) > 0)
		{
			// is data little or big endian? 
			// big endian:
			memcpy(set_up_l_buf, set_up_buffer, 2 * SET_UP_BUFFER_SIZE);	// memcpy copies at a byte level
			memcpy(set_up_r_buf, set_up_buffer[SET_UP_BUFFER_SIZE], 2 * SET_UP_BUFFER_SIZE);
			
			// little endian (something similar to this?):
				// buffer[] needs to be 2x size & int8_t
			/*for( i = 0; i < READ_SIZE; i += 4)
			{
				l_buf[i] = buffer[i] | (buffer[i+1] << 8);
				r_buf[i] = buffer[i+2] | (buffer[i+3] << 8);
			}*/
			
			// pass to AuxIVA
			unmix(true, l_buf, r_buf);
		}
		else
		{
			cerr < "ERROR: error setting up AuxIVA; can't read first block of 4096" << endl;
			return -1;
		}
		
		// read WINDOW_SHIFT samples every time after
        while ((fread(buffer, sizeof(buffer[0]), READ_SIZE, wavFile)) > 0)
        {
			// is data little or big endian? 
			// big endian:
			memcpy(l_buf, buffer, 2 * BUFFER_SIZE);	// memcpy copies at a byte level
			memcpy(r_buf, buffer[BUFFER_SIZE], 2 * BUFFER_SIZE);
			
			// little endian (something similar to this?):
				// buffer[] needs to be 2x size & int8_t
			/*for( i = 0; i < READ_SIZE; i += 4)
			{
				l_buf[i] = buffer[i] | (buffer[i+1] << 8);
				r_buf[i] = buffer[i+2] | (buffer[i+3] << 8);
			}*/
			
            /** PASS TO AUX_IVA **/
			unmix(false, l_buf, r_buf, l_res, r_res);
        }
        
        filelength = getFileSize(wavFile);

        cout << "File is                    :" << filelength << " bytes." << endl;
        cout << "RIFF header                :" << wavHeader.RIFF[0] << wavHeader.RIFF[1] << wavHeader.RIFF[2] << wavHeader.RIFF[3] << endl;
        cout << "WAVE header                :" << wavHeader.WAVE[0] << wavHeader.WAVE[1] << wavHeader.WAVE[2] << wavHeader.WAVE[3] << endl;
        cout << "FMT                        :" << wavHeader.fmt[0] << wavHeader.fmt[1] << wavHeader.fmt[2] << wavHeader.fmt[3] << endl;
        cout << "Data size                  :" << wavHeader.ChunkSize << endl;

        // Display the sampling Rate from the header
        cout << "Sampling Rate              :" << wavHeader.SamplesPerSec << endl;
        cout << "Number of bits used        :" << wavHeader.bitsPerSample << endl;
        cout << "Number of channels         :" << wavHeader.NumOfChan << endl;
        cout << "Number of bytes per second :" << wavHeader.bytesPerSec << endl;
        cout << "Data length                :" << wavHeader.Subchunk2Size << endl;
        cout << "Audio Format               :" << wavHeader.AudioFormat << endl;
        // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM

        cout << "Block align                :" << wavHeader.blockAlign << endl;
        cout << "Data string                :" << wavHeader.Subchunk2ID[0] << wavHeader.Subchunk2ID[1] << wavHeader.Subchunk2ID[2] << wavHeader.Subchunk2ID[3] << endl;
    }
    fclose(wavFile);
    return 0;
}

// find the file size
int getFileSize(FILE* inFile)
{
    int fileSize = 0;
    fseek(inFile, 0, SEEK_END);

    fileSize = ftell(inFile);

    fseek(inFile, 0, SEEK_SET);
    return fileSize;
}