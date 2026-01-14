#pragma once
#include "global.hpp"
#include "position.hpp"
#include "audiosystem.hpp"

namespace nthp {


        #define DEFAULT_RENDER_COLOR 144, 144, 144, 255

        struct RenderPacket {
                SDL_Texture* texture;
                SDL_Rect* srcRect;
                SDL_Rect dstRect;

                enum C_OPERATE { INVALID = 0, VALID, ABSOLUTE } state;
        };
        constexpr nthp::RenderPacket generateRenderPacket(SDL_Texture* texture, SDL_Rect* srcRect, SDL_Rect dst, nthp::RenderPacket::C_OPERATE s) {
                return {texture, srcRect, dst, s};
        }

        constexpr auto INVALID_RENDERPACKET = generateRenderPacket(NULL, NULL, {0,0,0,0}, nthp::RenderPacket::C_OPERATE::INVALID);




        class EngineCore {
        public:
                EngineCore() { window = nullptr; renderer = nullptr; running = false; };
                EngineCore(nthp::RenderRuleSet settings, const char* title, bool fullscreen, bool softwareRendering);

                int init(nthp::RenderRuleSet settings, const char* title, bool fullscreen, bool softwareRendering);

                void handleEvents();
                void handleEvents(void (*handler)(SDL_Event*));


                int render(nthp::RenderPacket packet);
                void clear();
                void display();
                void stop();

                void setWindowRenderSize(int x, int y);
                void setVirtualRenderScale(nthp::fixed_t x, nthp::fixed_t y);

                inline bool isRunning() { return running; }

                nthp::RenderRuleSet p_coreDisplay;
                SDL_Event eventList;

                inline SDL_Window* getWindow() { return window; }
                inline SDL_Renderer* getRenderer() { return renderer; }

                inline bool getInitSuccess() { return initSuccess; }

                int cleanup();

                void startTextInput();
                void stopTextInput();

                nthp::audio::defaultAudioSystem audioSystem;

                // Reinitialize the audio system with a new device specified. AudioDeviceID specifies an index in nthp::audio::audioDeviceNames,
                // which is generated on core_init. NOTE that any generated chunks and music may be invalid after this call, as the new device may
                // require different encoding. Destroy and reload all loaded audio after calling this to avoid errors.
                void reloadAudioSystem(unsigned int audioDeviceID) {
                        Mix_CloseAudio();
                        Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 4096, nthp::audio::audioDeviceNames[audioDeviceID].c_str(), SDL_AUDIO_ALLOW_ANY_CHANGE);
                }

                void setSoundVolume(unsigned int soundID, unsigned int newVolume) {
                        Mix_VolumeChunk(audioSystem.soundEffects[soundID].soundData, newVolume);
                }

                void setMusicVolume(unsigned int newVolume) {
                        Mix_VolumeMusic(newVolume);
                }



                ~EngineCore();
        private:
                SDL_Window* window;
                SDL_Renderer* renderer;
        
                bool running;
                bool initSuccess;

        };

        extern nthp::EngineCore core;


}

