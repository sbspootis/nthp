#pragma once
#include "softwaretexture.hpp"


namespace nthp {
        namespace texture {

        #ifndef NTHP_MONOCHROME_BITWIDTH
                #define NTHP_MONOCHROME_BITWIDTH        (8)
        #endif

        #if NTHP_MONOCHROME_BITWIDTH == 64
                typedef uint64_t monochromeBitWidth;
        #else
                #if NTHP_MONOCHROME_BITWIDTH == 32
                        typedef uint32_t monochromeBitWidth;
                #else
                        #if NTHP_MONOCHROME_BITWIDTH == 16
                                typedef uint16_t monochromeBitWidth;
                        #else
                                #if NTHP_MONOCHROME_BITWIDTH == 8
                                        typedef uint8_t monochromeBitWidth;
                                #else
                                        #undef NTHP_MONOCHROME_BITWIDTH
                                        #define NTHP_MONOCHROME_BITWIDTH        8

                                        typedef uint8_t monochromeBitWidth;
                                #endif
                        #endif
                #endif
        #endif

                // A 1bbp ST texture format; Uses a single palette color for every pixel.
                class MonochromeTexture {
                public:
                        struct MST_Header {
                                uint8_t signature;
                                uint8_t width;
                                uint32_t x;
                                uint32_t y;
                        };
                        static constexpr uint8_t MST_HeaderSignature = 0b01101100;

                        MonochromeTexture();
                        MonochromeTexture(uint32_t x, uint32_t y);
                        MonochromeTexture(const char* filename);

                        void init();


                        int importFromFile(const char* filename);
                        int createEmtpyTexture(uint32_t x, uint32_t y);
                        int exportToFile(const char* filename);

                        // Calls importFromFile() and regenerateTexture() to create a valid SDL Texture.
                        int generateTexture(const char* filename, nthp::texture::Palette* palette, NTHPST_COLOR_WIDTH colorIndex, SDL_Renderer* renderer);
                        
                        // Uses cached MST data to create an SDL texture with any given palette and color.
                        int regenerateTexture(nthp::texture::Palette* palette, NTHPST_COLOR_WIDTH colorIndex, SDL_Renderer* renderer);

                        void clean();
                        
                        ~MonochromeTexture();

                        MST_Header header;
                        nthp::texture::monochromeBitWidth* pixelData;

                        // Number of total bits for pixel data.
                        size_t totalPixelCount;

                        // Number of encoded widths for pixel data.
                        size_t totalWidthCount;

                        NTHPST_COLOR_WIDTH color;
                
                        SDL_Texture* texture;
                };
        }
}