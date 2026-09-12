#include "st_font.hpp"


int nthp::texture::text::characterMap::exportToFile(const char* output) {
        std::fstream file;
        file.open(output, std::ios::out | std::ios::binary);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Failed to open output file [%s] for characterMap [%p].\n", output, this);
                return 1;
        }

        file.write((char*)&charWidth, sizeof(charWidth));
        file.write((char*)map, sizeof(map));

        file.close();

        return 0;
}


int nthp::texture::text::characterMap::import(const char* input) {
        std::fstream file;
        file.open(input, std::ios::in | std::ios::binary);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to open character map [%s]; File not found.\n", input);
                return 1;
        }

        file.read((char*)&charWidth, sizeof(charWidth));
        file.read((char*)map, sizeof(map));

        return 0;
}








nthp::texture::text::Font::Font() {
        frameList = nullptr;
        frameCount = 0;
}


int nthp::texture::text::Font::importFontSet(const char* texture, const char* mapFile, nthp::texture::Palette* palette, SDL_Renderer* renderer) {
        if(map.import(mapFile)) { return 1; }
        if(fontTextureData.autoLoadTextureFile(texture, palette, renderer)) { return 1; }

        SDL_Rect format;
        format.w = map.charWidth;
        format.h = fontTextureData.getTextureData().getMetaData().y;
        format.y = 0;

        frameCount = fontTextureData.getTextureData().getMetaData().x / map.charWidth;

        try {
                frameList = new SDL_Rect[frameCount];
        }
        catch(std::bad_alloc) {
                PRINT_DEBUG_ERROR("Unable to allocate frame data for font [%p].\n", this);
                return 1;
        }

        for(size_t i = 0; i < frameCount; ++i) {
                format.x = i * format.w;
                frameList[i] = format;
        }

        return 0;
}


nthp::texture::Frame nthp::texture::text::Font::getCharFrame(char value) {
        nthp::texture::Frame output;
        output.src = frameList[map.map[value]];
        output.texture = fontTextureData.getTextureData().getTexture();

        return output;
}





nthp::texture::text::Font::~Font() {
        if(frameCount) {
                delete[] frameList;
                frameCount = 0;
        }
}