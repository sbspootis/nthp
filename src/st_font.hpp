#pragma once
#include "gtexture.hpp"
#include "core.hpp"


namespace nthp {
        namespace texture {
                namespace text {

                class Font {
                public:
                        Font();

                        int loadFontTexture(const char* filename, SDL_Renderer* renderer, unsigned int cWidth, unsigned int cHeight);
                        nthp::texture::SoftwareTexture& getTexture() { return fontSet.getTextureData(); }
                        
                        nthp::texture::Frame getCharacterFrame(const char code);

                        SDL_Rect getCharacterRect(const char code);

                        SDL_Rect characterMap[96];
                        
                         
                        nthp::texture::gTexture fontSet;
                        unsigned int characterWidth;
                        unsigned int characterHeight;
                };

                }
        }
}