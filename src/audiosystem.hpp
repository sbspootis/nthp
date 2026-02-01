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
                        }

                        Mix_Music* musicData;

                };

                class SoundChannel {
                public:
                        SoundChannel() { soundData = NULL; channel = -1; }
                        SoundChannel(const char* file) {
                                isAllocated = false;
                                load(file);
                        }
                        
                        // Returns 1 on failure.
                        int load(const char* file) {
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
                                if(channel != -1) {
                                        if(Mix_Playing(channel)) { return 0; }
                                        else { channel = Mix_PlayChannel(-1, soundData, 0); }
                                }
                                else {
                                        channel = Mix_PlayChannel(-1, soundData, 0);
                                }
                                return 0;
                        }

                        void free() {
                                Mix_FreeChunk(soundData);
                                soundData = NULL;
                                isAllocated = false;
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
                };


        }

}