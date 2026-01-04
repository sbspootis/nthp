#include "softwaretexture.hpp"

void nthp::texture::SoftwareTexture::init() {
        pixelData = nullptr;
        texture = nullptr;
        dataSize = 0;
}


nthp::texture::SoftwareTexture::SoftwareTexture() {
        init();
}




nthp::texture::SoftwareTexture::SoftwareTexture(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer) {
        init();
        
        if(this->generateTexture(filename, palette, coreRenderer)) {
                PRINT_DEBUG_ERROR("Failed to generate SoftwareTexture.\n");
                return;
        }
}

nthp::texture::SoftwareTexture::SoftwareTexture(const char* filename) {
        init();
        
        if(generateTexture(filename, NULL, NULL)) {
                PRINT_DEBUG_ERROR("Failed to generate SoftwareTexture.\n");
                return;
        }
}



// Generates a texture from a valid ST file with the specified palette. Returns a 1 on failure. Safe to run
// when a valid texture is already generated.
int nthp::texture::SoftwareTexture::generateTexture(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer) {
        if(dataSize != 0) { SDL_DestroyTexture(texture); free(pixelData); dataSize = 0; }
        PRINT_DEBUG("Creating SoftwareTexture...\n");
        
        std::fstream file;
        file.open(filename, std::ios::in | std::ios::binary);
        PRINT_DEBUG("Opening ST File [%s]...\n", filename);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to create SoftwareTexture; file not found.\n");
                pixelData = nullptr;
                dataSize = 0;

                return 1;
        }


        file.read((char*)&metadata, sizeof(metadata));

        if(metadata.signature != STheaderSignature) {
                PRINT_DEBUG_ERROR("Unable to create SoftwareTexture; Invalid file format.\n");
                pixelData = nullptr;
                dataSize = 0;

                return 1;
        }

        dataSize = metadata.x * metadata.y;
        pixelData = (NTHPST_COLOR_WIDTH*)malloc(dataSize * sizeof(NTHPST_COLOR_WIDTH));

        if(pixelData == NULL) {
                FATAL_PRINT(nthp::FATAL_ERROR::Memory_Fault, "Unable to allocate pixelData buffer.");
        }

        file.read((char*)pixelData, (dataSize * sizeof(NTHPST_COLOR_WIDTH)));

        if(palette != NULL) {
                nthp::texture::rawSurface stSurface(metadata.x, metadata.y);


                // Loops through and generates the texture using the given palette.
                for(size_t i = 0; i < dataSize; ++i) {
                        stSurface.setPixel(i, palette->pullColorSetWithAlpha(getPixelColor(pixelData[i]), getTrueAlpha(getPixelAlphaLevel(pixelData[i]))));
                }
                texture = SDL_CreateTextureFromSurface(coreRenderer, stSurface.getSurface());
                
                return 0;
        }
        PRINT_DEBUG_WARNING("SoftwareTexture [%p] generated with NULL palette; Use 'regenerateTexture()' with a valid palette to compile into valid texture.\n", this);

        return 0;
}


// Uses saved pixeldata to redraw a texture with a given palette. Always safe to use, regardless
// of the texture already being generated or not. Very slow, look for something better.
void nthp::texture::SoftwareTexture::regenerateTexture(nthp::texture::Palette* palette, SDL_Renderer* renderer) {
        if(dataSize != 0) 
                SDL_DestroyTexture(texture);
        else {
                PRINT_DEBUG_ERROR("Unable to regenerate software texture [%p]; Texture not loaded.\n", this);
        }

        nthp::texture::rawSurface stSurface(metadata.x, metadata.y);

        for(size_t i = 0; i < dataSize; ++i)
                stSurface.setPixel(i, palette->pullColorSetWithAlpha(getPixelColor(pixelData[i]), getTrueAlpha(getPixelAlphaLevel(pixelData[i]))));


        texture = SDL_CreateTextureFromSurface(renderer, stSurface.getSurface());

        PRINT_DEBUG("Regenerated texture [%p] with palette [%p].\n", this, palette);
}


void nthp::texture::SoftwareTexture::createEmptyTexture(const size_t dataSize) {
        pixelData = (NTHPST_COLOR_WIDTH*)malloc(dataSize * sizeof(NTHPST_COLOR_WIDTH));
        this->dataSize = dataSize;
        memset(pixelData, 0, dataSize * sizeof(NTHPST_COLOR_WIDTH));
}

nthp::texture::SoftwareTexture::~SoftwareTexture() {

        purgeTextureData();
}





// SDLIMAGE TOOLS GO HERE
#if USE_SDLIMG == 1

// Approximates a PNG or JPEG image as a softwareTexture given any palette. 
int nthp::texture::tools::generateSoftwareTextureFromImage(const char* inputImageFile, nthp::texture::Palette* palette, const char* outputFile) {
        SDL_Surface* conv = NULL;
        if(palette == NULL) {
                PRINT_DEBUG_ERROR("[%u] Invalid palette for approximation.\n", SDL_GetTicks());
                return 1;
        }
        {
                nthp::texture::rawSurface img(IMG_Load(inputImageFile));

                if(img.getSurface() == NULL) {
                        PRINT_DEBUG_ERROR("Unable to generate softwareTexture from image; Unable to allocate.\n");
                        PRINT_DEBUG("[%u] %s\n", SDL_GetTicks(), SDL_GetError());
                        return 1;
                }

                conv = SDL_ConvertSurfaceFormat(img.getSurface(), SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA32, 0);
                if(conv == NULL) {
                        PRINT_DEBUG_ERROR("Unable to generate softwareTexture from image; Unable to convert surface format.\n");
                        PRINT_DEBUG_ERROR("[%u] %s\n", SDL_GetTicks(), SDL_GetError());
                        return 1;
                }
        }
        nthp::texture::rawSurface baseImage(conv);
        nthp::texture::SoftwareTexture::software_texture_header header;

        header.signature = nthp::texture::SoftwareTexture::STheaderSignature;
        header.x = baseImage.getSurface()->w;
        header.y = baseImage.getSurface()->h;

        size_t surfaceSize = baseImage.getSurface()->w * baseImage.getSurface()->h;
        nthp::sArray<NTHPST_COLOR_WIDTH> pixelData(surfaceSize);
        
        

        struct pixelScore {
                int score;
        };

        pixelScore scores[nthp::texture::PaletteFileSize];
        uint16_t smallestElement = 0;    // This is a pointer.
        double alphaLevelCalculation = 0;

        uint8_t alphaLevel;
        
        NOVERB_PRINT_DEBUG("\tGENERATING NEW ST TEXTURE FROM FILE %s...\n", inputImageFile);
        double progress = 0;

        // Outer loop for cycling through baseImage pixels.
        for(size_t i = 0; i < surfaceSize; ++i) {
                

                // This loop generates a score for each palette colour relative to a given pixel [i]. The palette colour with the lowest
                // score has the least deviation from the original pixel, and is chosen to replace the original colour in the softwareTexture.
                for(size_t j = 0; j < nthp::texture::PaletteFileSize; ++j) {
                        scores[j].score = abs((int32_t)baseImage.getPixel(i).R - (int32_t)palette->colorSet[j].R) + 
                                        abs((int32_t)baseImage.getPixel(i).G - (int32_t)palette->colorSet[j].G) +
                                        abs((int32_t)baseImage.getPixel(i).B - (int32_t)palette->colorSet[j].B);
                        
                        if(scores[j].score < scores[smallestElement].score) smallestElement = j;
                }

                // By this point, the smallest score is stored at index colorset[smallestElement].
                alphaLevelCalculation = (double)baseImage.getPixel(i).A / (double)nthp::texture::SoftwareTexture::alphaLevelSize;
                if(alphaLevelCalculation - floor(alphaLevelCalculation) > 0.3) { alphaLevelCalculation = ceil(alphaLevelCalculation); }
                alphaLevel = alphaLevelCalculation;

                pixelData[i] = (smallestElement << nthp::texture::SoftwareTexture::alphaBitCount) | alphaLevel;
                smallestElement = 0;
                progress = ((double)i / (double)surfaceSize) * (double)100;
        }
        

        std::fstream file(outputFile, std::ios::out | std::ios::binary);
        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to generate softwareTexture from image; File not accessible.\n");
                return 1;
        }

        file.write((char*)&header, sizeof(header));
        file.write((char*)pixelData.getData(), pixelData.getBinarySize());

        file.close();
        NOVERB_PRINT_DEBUG("\tDone. Output target [%s].\n", outputFile);

        return 0;
}

#endif


constexpr NTHPST_COLOR_WIDTH nthp::texture::tools::encodePixel(const NTHPST_COLOR_WIDTH colorIndex, const NTHPST_COLOR_WIDTH alphaLevel) {
        return ((colorIndex << nthp::texture::SoftwareTexture::alphaBitCount) | (alphaLevel & nthp::texture::SoftwareTexture::NTHPST_ALPHAMASK));
}


nthp::texture::SoftwareTexture::STdata nthp::texture::tools::readTextureData(const char* textureFile) {
        PRINT_DEBUG("Importing raw ST file [%s]...", textureFile);
        nthp::texture::SoftwareTexture::STdata ret;
        ret.header.signature = 0;

        std::fstream file(textureFile, std::ios::in | std::ios::binary);
        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to open texture file [%s].\n", textureFile);
                return ret;
        }

        file.read((char*)&ret.header, sizeof(ret.header));

        const size_t dataSize = (ret.header.x * ret.header.y);
        ret.pixelData = (NTHPST_COLOR_WIDTH*)malloc(dataSize * sizeof(NTHPST_COLOR_WIDTH));

        file.read((char*)ret.pixelData, (dataSize * sizeof(NTHPST_COLOR_WIDTH)));
        file.close();


        ret.header.signature = nthp::texture::SoftwareTexture::STheaderSignature;

        NOVERB_PRINT_DEBUG("done.\n");
        return ret;
}

int nthp::texture::tools::writeTextureData(nthp::texture::SoftwareTexture::STdata data, const char* outputFile) {
        PRINT_DEBUG("Writing texture binary to file [%s]... ", outputFile);
        std::fstream file(outputFile, std::ios::out | std::ios::binary);

        file.write((char*)&data.header, sizeof(data.header));
        const size_t dataSize = data.header.x * data.header.y;
        file.write((char*)data.pixelData, dataSize * sizeof(NTHPST_COLOR_WIDTH));

        file.close();

        NOVERB_PRINT_DEBUG("done.\n");
        return 0;
}


const nthp::texture::SoftwareTexture::STdata nthp::texture::tools::joinSoftwareTextures(nthp::texture::SoftwareTexture::STdata a, nthp::texture::SoftwareTexture::STdata b, bool joinMethod) {
        using namespace nthp::texture;
        
        SoftwareTexture::STdata ret;
        ret.header.signature = 0;

        if(a.header.signature != SoftwareTexture::STheaderSignature || b.header.signature != SoftwareTexture::STheaderSignature) {
                PRINT_DEBUG_ERROR("Unable to join textures; invalid texture(s).\n");
                return ret;
        }

        const size_t a_dataSize = a.header.x * a.header.y;
        const size_t b_dataSize = b.header.x * b.header.y;

        

        PRINT_DEBUG("TextureDimensions ; ax=%zu ay=%zu; bx=%zu by=%zu\n", a.header.x, a.header.y, b.header.x, b.header.y);
        const auto emtpyPixel = nthp::texture::tools::encodePixel(0, 0);
        
        switch(joinMethod) {
                case JOIN_WIDTH:
                        {
                                ret.header.x = a.header.x + b.header.x;
                                if(a.header.y > b.header.y)
                                        ret.header.y = a.header.y;
                                else
                                        ret.header.y = b.header.y;

                                const size_t o_dataSize = ret.header.x * ret.header.y;
                                const size_t oBinary_dataSize = o_dataSize * sizeof(NTHPST_COLOR_WIDTH);
                                ret.pixelData = (NTHPST_COLOR_WIDTH*)malloc(oBinary_dataSize + 1);
                                PRINT_DEBUG("Allocated %zu bytes for texture join.\n", oBinary_dataSize);

                                
                                
                                uint32_t aPosition = 0;
                                uint32_t bPosition = 0;
                                size_t outputPosition = 0;
                                bool operationComplete = false;
                                

                                size_t aPasses = 0;
                                size_t bPasses = 0;
                                
                                do {
                                        // Check to ensure there is still texture data to write.
                                        if(aPosition < a_dataSize) {
                                                memcpy(ret.pixelData + outputPosition, a.pixelData + aPosition, a.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                                outputPosition += a.header.x;
                                                aPosition += a.header.x;
                                                ++aPasses;
                                        }
                                        else {
                                                // Writes an empty, invisible pixel when the output texture is incomplete, and all the pixel data
                                                // has been copied.
                                                memset(ret.pixelData + outputPosition, emtpyPixel, a.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                                outputPosition += a.header.x;
                                        }

                                        if(bPosition < b_dataSize) {
                                                memcpy(ret.pixelData + outputPosition, b.pixelData + bPosition, b.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                                outputPosition += b.header.x;
                                                bPosition += b.header.x;
                                                ++bPasses;
                                        }
                                        else {
                                                // Writes an empty, invisible pixel when the output texture is incomplete, and all the pixel data
                                                // has been copied.
                                                memset(ret.pixelData + outputPosition, emtpyPixel, b.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                                outputPosition += b.header.x;
                                        }

                                } while(outputPosition < o_dataSize);

                                PRINT_DEBUG("Wrote total %zu bytes.\n%zu out of textureA\n%zu out of textureB\n", outputPosition * sizeof(NTHPST_COLOR_WIDTH), aPosition * sizeof(NTHPST_COLOR_WIDTH), bPosition * sizeof(NTHPST_COLOR_WIDTH));

                        }
                        break;
                case JOIN_HEIGHT:
                        {
                                ret.header.y = a.header.y + b.header.y;
                                if(a.header.x > b.header.x)
                                        ret.header.x = a.header.x;
                                else
                                        ret.header.x = b.header.x;

                                const size_t o_dataSize = ret.header.x * ret.header.y;
                                const size_t oBinary_dataSize = o_dataSize * sizeof(NTHPST_COLOR_WIDTH);
                                ret.pixelData = (NTHPST_COLOR_WIDTH*)malloc(oBinary_dataSize + 1);
                                PRINT_DEBUG("Allocated %zu bytes for texture join.\n", oBinary_dataSize);

                                size_t bEntryPoint = 0;
                                
                                if(a.header.x < ret.header.x) {
                                        const auto line_difference = ret.header.x - a.header.x;
                                        size_t aPosition = 0;
                                        size_t oPosition = 0;
                                        do {
                                                memcpy(ret.pixelData + oPosition, a.pixelData + aPosition, a.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                                aPosition += a.header.x;
                                                oPosition += a.header.x;
                                                
                                                memset(ret.pixelData + oPosition, 0, line_difference);
                                                oPosition += line_difference;

                                        } while(aPosition < a_dataSize);

                                        bEntryPoint = oPosition;
                                }
                                else {
                                        memcpy(ret.pixelData + bEntryPoint, a.pixelData, a_dataSize * sizeof(NTHPST_COLOR_WIDTH));
                                        bEntryPoint = a_dataSize;
                                }

                                if(b.header.x < ret.header.x) {
                                        const auto line_difference = ret.header.x - b.header.x;
                                        size_t bPosition = 0;
                                        size_t oPosition = 0;
                                        do {
                                                memcpy(ret.pixelData + oPosition, b.pixelData + bPosition, b.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                                bPosition += b.header.x;
                                                oPosition += b.header.x;
                                                
                                                memset(ret.pixelData + oPosition, 0, line_difference);
                                                oPosition += line_difference;

                                        } while(bPosition < b_dataSize);

                                        bEntryPoint = oPosition;
                                }
                                else {
                                        memcpy(ret.pixelData + bEntryPoint, b.pixelData, b_dataSize * sizeof(NTHPST_COLOR_WIDTH));
                                }
                                                                
                        }
                        break;

        }
        PRINT_DEBUG("Copied binary successfully.\n");


        ret.header.signature = SoftwareTexture::STheaderSignature;
        return ret;
}


void nthp::texture::tools::destroySTdata(nthp::texture::SoftwareTexture::STdata* data) {
        using namespace nthp::texture;
        if((data->header.signature == SoftwareTexture::STheaderSignature) && (data->header.x > 0) && (data->header.y > 0)) {
                free(data->pixelData);
                data->header.x = 0;
                data->header.y = 0;
                data->header.signature = 0;

                return;
        }
}





#undef SUPRESS_DEBUG_OUTPUT