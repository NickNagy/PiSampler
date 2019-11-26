/* Nick Nagy

A lot of this file is adapted and modified from http://blog.acipo.com/handling-endianness-in-c/ and http://blog.acipo.com/generating-wave-files-in-c/.
This file is for reading sample data and saving it to a WAVE file. It will eventually be used for other functions such as audio playback and manipulation

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_BYTES 100000

typedef struct header
{
    char id[4];// = "RIFF";
    unsigned int fileSize; //  = 44;
    char headerFormat[4]; //  = "WAVE";
    char chunkMarkerFormat[4]; //  = "fmt ";
    unsigned int formatDataLength; // = 16; // # bytes from id to chunkMarkerFormat
    unsigned short int formatType; // = 1;  // PCM
    unsigned short int numChannels; // = NUM_CHANNELS;
    unsigned int sampleRate; // = SAMPLE_RATE;
    unsigned int byteRate; // = BYTE_RATE;
    unsigned int monoOrStereo; // = bitsPerSample * numChannels / 8;
    unsigned int bitsPerSample; // = BITS_PER_SAMPLE;
    char dataChunkHeader[4]; // = "data";
    unsigned int dataSize;
} header;

typedef struct wave
{
    header h;
    char *data; // = (char *)malloc(MAX_BYTES * sizeof(char));
    long long int index; // = 0;
} wave;

unsigned char isBigEndian;
unsigned char reverse;
wave * programWave;

void reverseEndian(const long long int size, void *n)
{
    char temp;
    for (long long int i = 0; i < size / 2; i++)
    {
        temp = ((char *)n)[i];
        ((char *)n)[i] = ((char *)n)[size - (i + 1)];
        ((char *)n)[size - (i + 1)] = temp;
    }
}

void reverseEndianHeader(header *h)
{
    reverseEndian(sizeof(int), (void *)&(h->fileSize));
    reverseEndian(sizeof(int), (void *)&(h->formatDataLength));
    reverseEndian(sizeof(short int), (void *)&(h->formatType));
    reverseEndian(sizeof(short int), (void *)&(h->numChannels));
    reverseEndian(sizeof(int), (void *)&(h->sampleRate));
    reverseEndian(sizeof(int), (void *)&(h->byteRate));
    reverseEndian(sizeof(int), (void *)&(h->monoOrStereo));
    reverseEndian(sizeof(int), (void *)&(h->bitsPerSample));
    reverseEndian(sizeof(int), (void *)&(h->dataSize));
}

void readNextData(wave *w, const float *nextSample, const float maxFloat)
{
    unsigned int i;
    if (w->h.dataSize <= (MAX_BYTES - (BITS_PER_SAMPLE/8))) {
        switch (BITS_PER_SAMPLE)
        {
        case 32:
        {
            for (i = 0; i < NUM_CHANNELS; i++)
            {
                long int sample = (long int)(((pow(2, 32 - 1) - 1) / maxFloat) * nextSample[i]);
                if (isBigEndian)
                    reverseEndian(4, (void *)&sample);
                char *sampleChars = (char *)&sample;
                w->data[w->index] = sampleChars[0];
                w->data[w->index + 1] += sampleChars[1];
                w->data[w->index + 2] += sampleChars[2];
                w->data[w->index + 3] += sampleChars[3];
                w->index += 4;
                w->h.dataSize += 4;
            }
        }
        break;
        case 16:
        {
            for (i = 0; i < NUM_CHANNELS; i++)
            {
                int sample = (int)((32767 / maxFloat) * nextSample[i]);
                if (isBigEndian)
                    reverseEndian(2, (void *)&sample);
                char *sampleChars += (char *)&sample;
                w->data[w->index] += sampleChars[0];
                w->data[w->index + 1] += sampleChars[1];
                w->index += 2;
                w->h.dataSize += 2;
            }
        }
        break;
        default: // 8-bit
        {
            for (i = 0; i < NUM_CHANNELS; i++)
            {
                short int sample = (short int)(127 + (127.0 / maxFloat) * nextSample[i]);
                if (isBigEndian)
                    reverseEndian(1, (void *)&sample);
                w->data[w->index] += ((char *)&sample)[0];
                w->index++;
                w->h.dataSize++;
            }
        }
        break;
        }
    }
}

void clear(wave *w)
{
    for (unsigned int i = 0; i < w->h.dataSize; i++) {
        w->data[i] = 0;
    }
    w->index = 0;
    w->h.dataSize = 0;
}

void initializeWave(wave * w, unsigned int const sampleRate, unsigned int const bitsPerSample, unsigned short int const numChannels) {
    w->h.id = "RIFF";
    w->h.headerFormat = "WAVE";
    w->h.chunkMarkerFormat = "fmt ";
    w->h.formatDataLength = 16;
    w->h.formatType = 1; // PCM
    w->h.numChannels = numChannels;
    w->h.sampleRate = sampleRate;
    w->h.byteRate = sampleRate * numChannels * bitsPerSample / 8;
    w->h.monoOrStereo = bitsPerSample * numChannels / 8;
    w->h.bitsPerSample = bitsPerSample;
    w->h.dataChunkHeader = "data";
    w->data = (char *)malloc(MAX_BYTES*sizeof(char));
    clear(w);
}

void loadWaveFromFile(wave * w, const char * filename) {
    FILE * file = fopen(filename, "rb");
    fread(&(w->h), sizeof(header), 1, file);
    fread((void*)w->data, sizeof(char), w->h.dataSize, file);
    fclose();
    if (isBigEndian)
        reverseEndianHeader(&(w->h));
}

void saveWaveToFile(wave *w, const char *filename)
{
    w->h.fileSize = 44 + w->h.dataSize;
    if (isBigEndian)
        reverseEndianHeader(&(w->h));
    FILE *file = fopen(filename, "wb");
    fwrite(&(w->h), sizeof(header), 1, file);
    fwrite((void*)w->data, sizeof(char), w->h.dataSize, file);
    fclose();
    if (isBigEndian)
        reverseEndianHeader(&(w->h));
}

int main()
{
    char test = 1;
    char *p = (char *)&test;
    isBigEndian = (p[0] == 0);

    printf("You can save a WAV file of up to %d seconds in length", MAX_SECONDS);
    free(programWave);
    return 0;
}