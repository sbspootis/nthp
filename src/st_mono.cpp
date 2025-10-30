#include "st_mono.hpp"


void nthp::texture::MonochromeTexture::init() {
        pixelData = nullptr;
        header.x = 0;
        header.y = 0;
        header.signature = 0;

        totalWidthCount = 0;
        totalPixelCount = 0;
        color = 0;     
}


nthp::texture::MonochromeTexture::MonochromeTexture() {
        init();
}

nthp::texture::MonochromeTexture::MonochromeTexture(uint32_t x, uint32_t y) {
        init();

        if(createEmtpyTexture(x, y)) { PRINT_DEBUG_ERROR("Failed to construct new MST @ [%p].\n", this); }
}

nthp::texture::MonochromeTexture::MonochromeTexture(const char* filename) {
        init();

        if(importFromFile(filename)) { PRINT_DEBUG_ERROR("Failed to construct new MST @ [%p].\n", this); }
}

int nthp::texture::MonochromeTexture::importFromFile(const char* filename) {
        PRINT_DEBUG("Importing MST file [%s]...\n", filename);

        std::fstream file;
        file.open(filename, std::ios::in | std::ios::binary);


        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to import monochrome ST file [%s].\n", filename);
                return 1;
        }

        file.read((char*)&header, sizeof(header));
        if(header.signature != nthp::texture::MonochromeTexture::MST_HeaderSignature) {
                PRINT_DEBUG_ERROR("Unable to import monochrome ST file [%s]; invalid file format.\n", filename);
                return 1;
        }

        totalPixelCount = (header.x * header.y);
        totalWidthCount = (totalPixelCount / NTHP_MONOCHROME_BITWIDTH) + 1;

        pixelData = (nthp::texture::monochromeBitWidth*)malloc(totalWidthCount * sizeof(nthp::texture::monochromeBitWidth));
        if(pixelData == NULL) {
                PRINT_DEBUG_ERROR("Unable to allocate monochrome texture data.\n");
                return 1;
        }

        nthp::texture::monochromeBitWidth currentPixelWidth;
        size_t bitsRead = 0;

        for(size_t widths = 0; widths < totalWidthCount; ++widths) {
                file.read((char*)&currentPixelWidth, sizeof(currentPixelWidth));
                pixelData[widths] = currentPixelWidth;

                bitsRead += NTHP_MONOCHROME_BITWIDTH;
                if(bitsRead >= totalPixelCount) { break; }
        }


        file.close();


        PRINT_DEBUG("Imported MST data from file [%s] successfully.\n", filename);
        return 0;
}



int nthp::texture::MonochromeTexture::regenerateTexture(nthp::texture::Palette* palette, NTHPST_COLOR_WIDTH colorIndex, SDL_Renderer* renderer) {
        PRINT_DEBUG("Creating new MST Texture... ");
        nthp::texture::rawSurface stSurface(header.x, header.y);

        const nthp::texture::Pixel setPixel = palette->pullColorSetWithAlpha(colorIndex, 255);
        const nthp::texture::Pixel emptyPixel = palette->pullColorSetWithAlpha(0, 0);

        size_t totalBits = 0;

        for(size_t widths = 0; widths < totalWidthCount; ++widths) {
                for(size_t bits = 0; bits < NTHP_MONOCHROME_BITWIDTH; ++bits) {
                        if(((pixelData[widths]) >> bits) & 1) { stSurface.setPixel(totalBits, setPixel); }
                        else { stSurface.setPixel(totalBits, emptyPixel); }

                        ++totalBits;
                }
                if(totalBits >= totalPixelCount) { break; }
        }

        texture = SDL_CreateTextureFromSurface(renderer, stSurface.getSurface());
        if(texture == NULL) {
                PRINT_DEBUG_ERROR("Unable to create compiled SDL texture; %s\n", SDL_GetError());
                return 1;
        }

        NOVERB_PRINT_DEBUG("Done.\n");
        return 0;
}


int nthp::texture::MonochromeTexture::createEmtpyTexture(uint32_t x, uint32_t y) {
        header.signature = nthp::texture::MonochromeTexture::MST_HeaderSignature;
        header.x = x;
        header.y = y;

        totalPixelCount = (header.x * header.y);
        totalWidthCount = (totalPixelCount / NTHP_MONOCHROME_BITWIDTH) + 1;


        pixelData = (nthp::texture::monochromeBitWidth*)malloc(totalWidthCount * sizeof(nthp::texture::monochromeBitWidth));
        if(pixelData == NULL) {
                PRINT_DEBUG_ERROR("Unable to allocate monochrome texture data.\n");
                return 1;
        }

        memset(pixelData, 0, totalWidthCount * sizeof(nthp::texture::monochromeBitWidth));

        PRINT_DEBUG("Created empty MST data in MST @ [%p].\n", this);
        return 0;
}


int nthp::texture::MonochromeTexture::generateTexture(const char* filename, nthp::texture::Palette* palette, NTHPST_COLOR_WIDTH colorIndex, SDL_Renderer* renderer) {
        if(importFromFile(filename)) { return 1; }
        if(regenerateTexture(palette, colorIndex, renderer)) { return 1; }

        return 0;
}

void nthp::texture::MonochromeTexture::purgeMSTData() {
        if((pixelData != nullptr) && (totalPixelCount)) {
                free(pixelData);
        }
}


void nthp::texture::MonochromeTexture::clean() {
        purgeMSTData();
        SDL_DestroyTexture(texture);

        totalPixelCount = 0;
        totalWidthCount = 0;
        header.signature = 0;
        header.x = 0;
        header.y = 0;
}


nthp::texture::MonochromeTexture::~MonochromeTexture() {
        clean();
}