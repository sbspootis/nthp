#include "st_font.hpp"


nthp::texture::text::Font::Font() {
        characterWidth = 0;
        characterHeight = 0;
}

int nthp::texture::text::Font::loadFontTexture(const char* filename, SDL_Renderer* renderer, const unsigned int cWidth, const unsigned int cHeight) {
        if(fontSet.autoLoadTextureFile(filename, NULL, renderer)) {
                PRINT_DEBUG_ERROR("Unable to import font set from file [%s].\n", filename);
                return 1;
        }

        characterWidth = cWidth;
        characterHeight = cHeight;

        const size_t elementSize = fontSet.getTextureData().metadata.x / cWidth;
        
        // Must accomodate exactly 96 characters in the texture, joined by width.
        // (ASCII 32-127). 
        if(elementSize != 96) {
                PRINT_DEBUG_ERROR("Font set [%s] not formatted correctly; must contain exactly 96 (ACSII 32-127) characters joined by width.\n", filename);
                return 1;
        }
        SDL_Rect src;
        src.w = cWidth;
        src.h = cHeight;
        src.y = 0;
        for(size_t i = 0; i < 96; ++i) {
                src.x = i * cWidth;
                characterMap[i] = src;
        }

        return 0;
}


nthp::texture::Frame nthp::texture::text::Font::getCharacterFrame(const char code) {
        nthp::texture::Frame frame;
        frame.src = getCharacterRect(code);
        frame.texture = fontSet.getTextureData().getTexture();

        return frame;
}


SDL_Rect nthp::texture::text::Font::getCharacterRect(const char code) {
        return characterMap[(code - 32) & 127];
}