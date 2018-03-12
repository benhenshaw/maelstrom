// Maelstrom
// Simple audio looping station.
// By Benedict Henshaw, 2018

// TODO:
//     + Error handling.
//     + Better key mapping.
//     + Overall volume control.
//     + Individual sound volume controls.
//     + Runtime settings.

// The maximum number of sounds.
#define SOUND_COUNT 64

// How many samples of audio can be recorded into a sound.
#define SOUND_SAMPLE_COUNT (48000)

// How many samples the buffer at a time. Affects latency and stability.
#define CHUNK_SAMPLE_COUNT 64

// Hear the recording as it comes in.
#define LOOPBACK

#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef float f32;

SDL_AudioDeviceID audio_output_device;
SDL_AudioDeviceID audio_input_device;

// Global volume control.
f32 volume = 0.75f;

// Sound states.
#define PLAYING 1
#define RECORDING 2

typedef struct {
    f32 samples[SOUND_SAMPLE_COUNT];
    f32 volume;
    int index;
    int state;
} Sound;

Sound sounds[SOUND_COUNT];

void audio_callback(void * data, u8 * stream, int byte_count) {
    memset(stream, 0, byte_count);

    static f32 recording_samples[CHUNK_SAMPLE_COUNT];
    SDL_DequeueAudio(audio_input_device, recording_samples, byte_count);

#ifdef LOOPBACK
    memcpy(stream, recording_samples, byte_count);
#endif

    int sample_count = byte_count / sizeof(f32);
    f32 * samples = (f32 *)stream;

    for (int s = 0; s < SOUND_COUNT; ++s) {
        if (sounds[s].state == PLAYING) {
            for (int i = 0; i < sample_count; ++i) {
                samples[i] += sounds[s].samples[sounds[s].index] * sounds[s].volume * volume;
                sounds[s].index = (sounds[s].index + 1) % SOUND_SAMPLE_COUNT;
            }
        } else if (sounds[s].state == RECORDING) {
            for (int i = 0; i < CHUNK_SAMPLE_COUNT; ++i) {
                sounds[s].samples[sounds[s].index] = recording_samples[i];
                sounds[s].index = (sounds[s].index + 1) % SOUND_SAMPLE_COUNT;
            }
        }
    }
}

int main(int argument_count, char ** arguments) {
    setbuf(stdout, 0);

    SDL_Init(SDL_INIT_EVERYTHING);

    int width  = 640;
    int height = 480;

    SDL_Window * window = SDL_CreateWindow("",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, 0);

    u32 * pixels = SDL_GetWindowSurface(window)->pixels;

    for (int s = 0; s < SOUND_COUNT; ++s) {
        sounds[s].state = PLAYING;
        sounds[s].volume = 0.9f;
    }

    SDL_AudioSpec spec = {
        .freq     = 48000,
        .format   = AUDIO_F32,
        .channels = 1,
        .callback = audio_callback,
        .samples  = CHUNK_SAMPLE_COUNT
    };
    audio_output_device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

    spec.callback = NULL;
    audio_input_device = SDL_OpenAudioDevice(NULL, 1, &spec, NULL, 0);

    SDL_PauseAudioDevice(audio_output_device, 0);
    SDL_PauseAudioDevice(audio_input_device, 0);

    memset(pixels, 0, width * height * sizeof(u32));

    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            } else if (event.type == SDL_KEYDOWN) {
                if (!event.key.repeat) {
                    int sc = event.key.keysym.scancode;
                    if (sc >= 0 && sc <= SOUND_COUNT) {
                        SDL_LockAudioDevice(audio_output_device);
                        sounds[sc].state = RECORDING;
                        SDL_UnlockAudioDevice(audio_output_device);
                    }
                }
            } else if (event.type == SDL_KEYUP) {
                if (!event.key.repeat) {
                    int sc = event.key.keysym.scancode;
                    if (sc >= 0 && sc <= SOUND_COUNT) {
                        SDL_LockAudioDevice(audio_output_device);
                        sounds[sc].state = PLAYING;
                        SDL_UnlockAudioDevice(audio_output_device);
                    }
                }
            }
        }

        SDL_LockAudioDevice(audio_output_device);

        int x = ((f32)sounds[0].index / (f32)SOUND_SAMPLE_COUNT) * width;
        for (int s = 0; s < SOUND_COUNT; ++s) {
            int y = (height / 2) + sounds[s].samples[sounds[s].index] * (height / 2);
            pixels[x + y * width] = ~0;
        }

        SDL_UnlockAudioDevice(audio_output_device);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (pixels[x + y * width] > 0) pixels[x + y * width] -= 1000;
            }
        }

        SDL_Delay(1);
        SDL_UpdateWindowSurface(window);
    }
}
