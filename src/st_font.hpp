#pragma once
#include "gtexture.hpp"
#include "core.hpp"


namespace nthp {
        namespace texture {
                namespace text {

                class characterMap {
                public:
                        characterMap();
                        void init() { charWidth = 0; memset(map, 0, 127); };

                        int exportToFile(const char* output);
                        int import(const char* file);

                        uint32_t charWidth;
                        int8_t map[127];
                };




                class Font {
                public:
                        Font();
                        void init();
                        int importFontSet(const char* texture, const char* mapFile, nthp::texture::Palette* palette, SDL_Renderer* renderer);
                        nthp::texture::Frame getCharFrame(char value);
                        

                        void clean();
                        ~Font();

                        nthp::texture::gTexture fontTextureData;
                        SDL_Rect* frameList;
                        characterMap map;

                        size_t frameCount;
                };

                }
        }
}