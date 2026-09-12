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

                                void setFont(nthp::texture::text::Font* newFont);
                                
                                void setPosition(nthp::worldPosition newPosition);
                                void setCharacterRenderSize(nthp::vectFixed size);
                                int renderText(nthp::EngineCore* coreTarget);   // requires the use of the core's rendering because it's done in chunks.
                                int abs_renderText(nthp::EngineCore* coreTarget);

                                nthp::vectFixed position;
                                nthp::fixed_t kerning;

                                char* stringTarget;
                        private:
                                nthp::entity::gEntity renderCursor;
                                nthp::texture::text::Font* font;
                                nthp::texture::Frame renderFrameChar;
                        };










                }
        }
}