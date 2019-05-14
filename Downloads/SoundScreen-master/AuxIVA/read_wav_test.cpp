/* http://www.cplusplus.com/forum/beginner/31584/#msg171056
 * http://www.cplusplus.com/forum/beginner/166954/
*/

#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>

using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::string;

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
    uint16_t        NumOfChan;      // Number of channels 1=Mono 2=Sterio
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
	const char* outputPath;
    string input;
	string output;
    if (argc < 3)
    {
        cout << "Input wav file name: ";
        cin >> input;
        cin.get();
        filePath = input.c_str();
		
		cout << "Output wav file name: ";
		cin >> output;
		cin.get();
		outputPath = output.c_str();
    }
    else
    {
        filePath = argv[1];
		outputPath = argv[2];
        cout << "Input wav file name: " << filePath << endl;
		cout << "Output wav file name: " << outputPath << endl;
    }

    FILE* wavFile = fopen(filePath, "rb");
	FILE* outFile = fopen(outputPath, "wb");
	
    if (wavFile == nullptr)
    {
        fprintf(stderr, "Unable to open file: %s\n", filePath);
		
        return -1;
    }
	else if (outFile == nullptr)
	{
		fprintf(stderr, "Unable to open file: %s\n", outputPath);
		
		return -1;
	}

    //Read the header
    size_t bytesRead = fread(&wavHeader, 1, headerSize, wavFile);
    cout << "Header Read " << bytesRead << " bytes." << endl;
    if (bytesRead > 0)
    {
        //Read the data
        uint16_t bytesPerSample = wavHeader.bitsPerSample / 8;      //Number of bytes per sample
        uint64_t numSamples = wavHeader.ChunkSize / bytesPerSample; //How many samples are in the wav file?
        static const uint16_t BUFFER_SIZE = 4096;
        int8_t* buffer = new int8_t[BUFFER_SIZE];
		
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
		
		// write header to new file
		size_t bytesWritten = fwrite(&wavHeader, 1, headerSize, outFile);
		if (bytesWritten < 1)
		{
			fprintf(stderr, "Unable to write header\n");
			
			return -1;
		}
		else if (bytesWritten != bytesRead)
		{
			fprintf(stderr, "Header bytes read != header bytes written: %d, %d\n", bytesRead, bytesWritten);
			
			return -1;
		}
		
        while ((bytesRead = fread(buffer, sizeof(buffer[0]), BUFFER_SIZE / sizeof(buffer[0]), wavFile)) > 0)
        {
            // write to another file
			bytesWritten = fwrite(buffer, sizeof(buffer[0]), bytesRead, outFile);
			if (bytesWritten != bytesRead)
			{
				fprintf(stderr, "Data bytes read != data bytes written: %d, %d\n", bytesRead, bytesWritten);
				
				return -1;
			}
        }
        delete[] buffer;
        buffer = nullptr;
    }
    fclose(wavFile);
	fclose(outFile);
	
	cout << "Success! :D....hopefully..." << endl;
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