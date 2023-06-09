#include <iostream>
#include <cmath>
#include <portaudio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 64
#define TABLE_SIZE 200

typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
} paTestData;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    paTestData *data = (paTestData*) userData;
    float *out = (float*) outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    for( i=0; i<framesPerBuffer; i++ )
    {
    *out++ = data->sine[data->left_phase];  /* left */
    *out++ = data->sine[data->right_phase];  /* right */
    data->left_phase = (data->left_phase + 1) % TABLE_SIZE;
    data->right_phase = (data->right_phase + 3) % TABLE_SIZE;
    }


    return paContinue;
}

int main()
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int i;

    // initialize sinusoidal wavetable
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = 0.1 * (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;

    err = Pa_Initialize();
    if( err != paNoError ) return -1;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) return -1;

    err = Pa_StartStream( stream );
    if( err != paNoError ) return -1;

    // sleep for several seconds
    Pa_Sleep(5*1000);

    err = Pa_StopStream( stream );
    if( err != paNoError ) return -1;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) return -1;

    Pa_Terminate();

    return 0;
}

