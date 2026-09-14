#pragma once
#include "global.hpp"
#include "core.hpp"
#include "e_entity.hpp"
#include "st_font.hpp"


namespace nthp {
        namespace texture {
                namespace text {

                        class RenderText {
                        public:
                                RenderText();
                                void init();

                                void setFont(nthp::texture::text::Font* newFont);
                                
                                void setPosition(nthp::worldPosition newPosition);
                                void setCharacterRenderSize(nthp::vectFixed size);
                                int renderText(nthp::EngineCore* coreTarget);   // requires the use of the core's rendering because it's done in chunks.
                                int abs_renderText(nthp::EngineCore* coreTarget);
                                void setStringTarget(char* newTarget);

                                nthp::vectFixed position;
                                nthp::vectFixed renderSize;
                                nthp::fixed_t kerning;

                                char* stringTarget;
                        
                        private:
                                nthp::RenderPacket renderCursor;
                                nthp::texture::text::Font* font;
                                SDL_Rect src;
                        };










                }
        }
}