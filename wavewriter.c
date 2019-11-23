/* Nick Nagy

A lot of this file is adapted and modified from http://blog.acipo.com/handling-endianness-in-c/ and http://blog.acipo.com/generating-wave-files-in-c/.
This file is for reading sample data and saving it to a WAVE file. It will eventually be used for other functions such as audio playback and manipulation

*/

#include <stdio.h>
#include <math>

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS 2
#define BYTE_RATE (SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE) / 8
#define MAX_BYTES 100000
#define MAX_SECONDS MAX_BYTES / BYTE_RATE

unsigned char isBigEndian;

typedef struct header {
    char id[4] = "RIFF";
    unsigned int fileSize = 44;
    char headerFormat[4] = "WAVE";
    char chunkMarkerFormat[4] = "fmt ";
    unsigned int formatDataLength = 16; // # bytes from id to chunkMarkerFormat
    unsigned short int formatType = 1; // PCM
    unsigned short int numChannels = NUM_CHANNELS;
    unsigned int sampleRate = SAMPLE_RATE;
    unsigned int byteRate = BYTE_RATE;
    unsigned int monoOrStereo = bitsPerSample * numChannels / 8;
    unsigned int bitsPerSample = BITS_PER_SAMPLE;
    char dataChunkHeader[4] = "data";
    unsigned int dataSize;
} header;

typedef struct wave {
    header h;
    char * data = (char *)malloc(MAX_BYTES*sizeof(char));
    long long int index = 0;
} wave;

void reverseEndian(const long long int size, void* n) {
    char temp;
    for (long long int i = 0; i < size/2; i++) {
        temp = ((char *)n)[i];
        ((char *)n)[i] = ((char *)n)[size-(i+1)];
        ((char *)n)[size - (i+1)] = temp;
    }
}

void reverseEndianHeader(header * h) {
    reverseEndian(sizeof(int), (void*)&h->fileSize);
    reverseEndian(sizeof(int), (void*)&h->formatDataLength);
    reverseEndian(sizeof(short int), (void*)&h->formatType);
    reverseEndian(sizeof(short int), (void*)&h->numChannels);
    reverseEndian(sizeof(int), (void*)&h->sampleRate);
    reverseEndian(sizeof(int), (void*)&h->byteRate);
    reverseEndian(sizeof(int), (void*)&h->monoOrStereo);
    reverseEndian(sizeof(int), (void*)&h->bitsPerSample);
    reverseEndian(sizeof(int), (void*)&h->dataSize);
}

void readNextData(wave * w, const float * nextData, const float maxFloat) {
    unsigned int i;
    if (w->dataSize <= (MAX_BYTES - (BITS_PER_SAMPLE/8)) {
        switch(BITS_PER_SAMPLE) {
            case 32:
                {
                    for (i = 0; i < NUM_CHANNELS; i++) {
                        long int sample = (long int)(((pow(2,32-1)-1)/maxFloat)*samples[i]);
                        if (isBigEndian) reverseEndian(4, (void*)&sample);
                        char * sampleChars = (char *)&sample;
                        w->data[index] = sampleChars[0];
                        w->data[index+1] = sampleChars[1];
                        w->data[index+2] = sampleChars[2];
                        w->data[index+3] = sampleChars[3];
                        w->index+=4;
                        w->h.dataSize+=4;
                    }
                }
                break;
            case 16:
                {
                    for (i = 0; i < NUM_CHANNELS; i++) {
                        int sample = (int)((32767/maxFloat)*samples[i]);
                        if (isBigEndian) reverseEndian(2, (void*)&sample);
                        char * sampleChars = (char *)&sample;
                        w->data[w->index] = sampleChars[0];
                        w->data[w->index+1] = sampleChars[1];
                        w->index += 2;
                        w->h.dataSize+=2;
                    }
                }
                break;
            default: // 8-bit
                {
                    for (i = 0; i < NUM_CHANNELS; i++) {
                        short int sample = (short int)(127 + (127.0/maxFloat)*samples[i]);
                        if (isBigEndian) reverseEndian(1, (void*)&sample);
                        w->data[w->index] = ((char*)&sample)[0];
                        w->index++;
                        w->h.dataSize++;
                    }
                }
                break;
        }
    }
} 

void clear(wave * w) {
    free(w->data);
    w->data = (char *)malloc(MAX_BYTES*sizeof(char));
    w->index = 0;
    w->h.dataSize = 0;
}

void saveToFile(wave * w, const char * filename) {
    if (isBigEndian) reverseEndianHeader(&(w->h));
    FILE * file = fopen(filename, "wb");
    fwrite(&(w->h), sizeof(header), 1, file);
    fwrite((void*)w->data), sizeof(char), w->h.dataSize, file);
    if (isBigEndian) reverseEndianHeader(&(w->h));
}

int main() {
    char test = 1;
    char * p = (char *)&test;
    isBigEndian = (p[0]==0);

    printf("You can save a WAV file of up to %d seconds in length", MAX_SECONDS);
    wave * w;
    free(w);
    return 0;
}