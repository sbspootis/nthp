#pragma once

#include "global.hpp"



namespace nthp {
        namespace audio {

                extern std::vector<std::string> audioDeviceNames;


                class MusicChannel {
                public:
                        MusicChannel() { musicData = NULL; }
                        MusicChannel(const char* file) {
                                load(file);
                        }

                        int load(const char* file) {
                                if(musicData != NULL) { Mix_FreeMusic(musicData); }
                                musicData = Mix_LoadMUS(file);
                                
                                if(musicData == NULL) {
                                        PRINT_DEBUG_ERROR("Failed to load music data [%s]. %s\n", file, SDL_GetError());

                                        return 1;
                                }

                                return 0;
                        }

                        int start() {
                                if(Mix_PlayMusic(musicData, -1) == -1) {
                                        PRINT_DEBUG_ERROR("Failed to start music track at [%p]. %s\n", this, SDL_GetError());
                                        return 1;
                                }
                                return 0;
                        }

                        int stop() {
                                Mix_HaltMusic();

                                return 0;
                        }

                        int pauseMusic() {
                                if(Mix_PlayingMusic()) {
                                        Mix_PauseMusic();
                                }

                                return 0;
                        }

                        int resumeMusic() {
                                if(Mix_PlayingMusic()) {
                                        Mix_ResumeMusic();
                                }

                                return 0;
                        }


                        ~MusicChannel() {
                                if(Mix_PlayingMusic()) {
                                        Mix_HaltMusic();
                                }

                                Mix_FreeMusic(musicData);
                                musicData = NULL;
                        }

                        Mix_Music* musicData;

                };

                class SoundChannel {
                public:
                        SoundChannel() { soundData = NULL; channel = -1; isAllocated = false; }
                        SoundChannel(const char* file) {
                                isAllocated = false;
                                load(file);
                        }
                        
                        // Returns 1 on failure.
                        int load(const char* file) {
                                if(isAllocated) { Mix_FreeChunk(soundData); }
                                soundData = Mix_LoadWAV(file);
                                if(soundData == NULL) {
                                        PRINT_DEBUG_ERROR("Unable to load sound [%s].\n", file);
                                        return 1;
                                }

                                isAllocated = true;
                                return 0;
                        }

                        // Returns -1 on failure.
                        int playSound() {
                                channel = Mix_PlayChannel(channel, soundData, 0);
                                return 0;
                        }

                        int stopSound() {
                                if(!(channel < 0))
                                        Mix_HaltChannel(channel);

                                return 0;
                        }

                        void free() {
                                Mix_FreeChunk(soundData);
                                soundData = NULL;
                                isAllocated = false;
                                channel = -1;
                        }


                        ~SoundChannel() {
                                free();
                        }
                        Mix_Chunk* soundData;
                        bool isAllocated;

                        int channel;
                };



                struct defaultAudioSystem {
                        nthp::audio::MusicChannel* music;
                        nthp::audio::SoundChannel* soundEffects;
                        
                        size_t soundSize;
                        size_t musicSize;

                        int channelCount;
                };


        }

}