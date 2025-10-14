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

                ~EngineCore();
        private:
                SDL_Window* window;
                SDL_Renderer* renderer;
        
                bool running;
                bool initSuccess;

        };

        extern nthp::EngineCore core;


}

