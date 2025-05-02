#include "st_compress.hpp"


namespace nthp { namespace text {
                        
        // NTHP FONT SYSTEM; More of an exercise than anything else, not particularly practical.
        // A font will contain:
        //      - A set of 256 monochrome textures, indexed with 8-bits
        //      - Each character (texture) is composed of 64 bits
        //      - Text Color is chosen from a palette on 'construct()' call.

#define FONT_PIXEL_WORD 64
#define FONT_ENTRY_SIZE 256
        // Monochrome pixel width 
        typedef uint64_t monoPixel;

constexpr size_t fontByteLength = sizeof(monoPixel) * FONT_ENTRY_SIZE;

        class Font {
        public:
                Font();
                Font(const char* fontFile);

                int importFont(const char* file);
                int exportFont(const char* file);


                monoPixel fontData[256];

        };


}
        
}