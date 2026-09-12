#pragma once
#include "gtexture.hpp"
#include "core.hpp"


namespace nthp {
        namespace texture {
                namespace text {

                class characterMap {
                public:
                        characterMap() { charWidth = 0; memset(map, 0, CHAR_MAX); }

                        int exportToFile(const char* output);
                        int import(const char* file);

                        uint32_t charWidth;
                        uint8_t map[CHAR_MAX];
                };




                class Font {
                public:
                        Font();
                        int importFontSet(const char* texture, const char* mapFile, nthp::texture::Palette* palette, SDL_Renderer* renderer);
                        nthp::texture::Frame getCharFrame(char value);
                        

                        ~Font();

                        nthp::texture::gTexture fontTextureData;
                        SDL_Rect* frameList;
                        characterMap map;

                        size_t frameCount;
                };

                }
        }
}