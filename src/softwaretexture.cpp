#include "softwaretexture.hpp"


nthp::texture::SoftwareTexture::SoftwareTexture() {
        pixelData = nullptr;
        texture = nullptr;
        dataSize = 0;
}




nthp::texture::SoftwareTexture::SoftwareTexture(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer) {
        pixelData = nullptr;
        texture = nullptr;
        dataSize = 0;
        
        if(this->generateTexture(filename, palette, coreRenderer)) {
                PRINT_DEBUG_ERROR("Failed to generate SoftwareTexture.\n");
                return;
        }
}

nthp::texture::SoftwareTexture::SoftwareTexture(const char* filename) {
        pixelData = nullptr;
        texture = nullptr;
        dataSize = 0;
        
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
        PRINT_DEBUG("Destroying SoftwareTexture [%p]...\t", this);

        purgeTextureData();
}





// SDLIMAGE TOOLS GO HERE
#if USE_SDLIMG == 1

// Approximates a PNG or JPEG image as a softwareTexture given any palette. 
int nthp::texture::tools::generateSoftwareTextureFromImage(const char* inputImageFile, nthp::texture::Palette* palette, const char* outputFile) {
        SDL_Surface* conv = NULL;
        if(palette == NULL) {
                PRINT_DEBUG("[%u] Invalid palette for approximation.\n", SDL_GetTicks());
                return -10;
        }
        {
                nthp::texture::rawSurface img(IMG_Load(inputImageFile));

                if(img.getSurface() == NULL) {
                        PRINT_DEBUG("Unable to generate softwareTexture from image; Unable to allocate.\n");
                        PRINT_DEBUG("[%u] %s\n", SDL_GetTicks(), SDL_GetError());
                        return -1;
                }

                conv = SDL_ConvertSurfaceFormat(img.getSurface(), SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA32, 0);
                if(conv == NULL) {
                        PRINT_DEBUG("Unable to generate softwareTexture from image; Unable to convert.\n");
                        PRINT_DEBUG("[%u] %s\n", SDL_GetTicks(), SDL_GetError());
                        return -2;
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
                pixelData[i] = (smallestElement << 4) | (baseImage.getPixel(i).A / nthp::texture::SoftwareTexture::alphaLevelSize);
                smallestElement = 0;
                progress = ((double)i / (double)surfaceSize) * (double)100;
        }
        

        std::fstream file(outputFile, std::ios::out | std::ios::binary);
        if(file.fail()) {
                PRINT_DEBUG("Unable to generate softwareTexture from image; File not accessible.\n");
                return -3;
        }

        file.write((char*)&header, sizeof(header));
        file.write((char*)pixelData.getData(), pixelData.getBinarySize());

        file.close();
        NOVERB_PRINT_DEBUG("\tDone. Output target [%s].\n", outputFile);

        return 0;
}

#endif


// Joins two textures into a single ST file; pass the constexpr tools::JOIN_WIDTH and tools::JOIN_HEIGHT for the ordering.
int nthp::texture::tools::joinSoftwareTextures(const char* textureFileA, const char* textureFileB, const bool joinMethod, const char* outputFile) {
        PRINT_DEBUG("Joining texture file [%s] with [%s]; output @ [%s]...\n", textureFileA, textureFileB, outputFile);
        nthp::texture::SoftwareTexture a;
        nthp::texture::SoftwareTexture b;

        if(a.generateTexture(textureFileA, NULL, NULL) || b.generateTexture(textureFileB, NULL, NULL)) {
                PRINT_DEBUG("Failed to join textures.\n");
                return 1;
        }

        nthp::texture::SoftwareTexture output;
        nthp::texture::SoftwareTexture::software_texture_header header;
        output.createEmptyTexture(a.dataSize + b.dataSize);

        header.signature = nthp::texture::SoftwareTexture::STheaderSignature;
        

        switch(joinMethod) {
        case JOIN_WIDTH:
                {
                        header.x = a.metadata.x + b.metadata.x;
                        if(a.metadata.y > b.metadata.y)
                                header.y = a.metadata.y;
                        else
                                header.y = b.metadata.y;
                        
                        
                        uint32_t aPosition = 0;
                        uint32_t bPosition = 0;
                        size_t outputPosition = 0;
                        bool operationComplete = false;
                        
                        do {
                                memcpy(output.pixelData + outputPosition, a.pixelData + aPosition, a.metadata.x * sizeof(NTHPST_COLOR_WIDTH));
                                outputPosition += a.metadata.x;
                                aPosition += a.metadata.x;

                                memcpy(output.pixelData + outputPosition, b.pixelData + bPosition, b.metadata.x * sizeof(NTHPST_COLOR_WIDTH));
                                outputPosition += b.metadata.x;
                                bPosition += b.metadata.x;

                        } while(outputPosition < output.dataSize);

                }
                break;
        case JOIN_HEIGHT:
                {
                        header.y = a.metadata.y + b.metadata.y;
                        if(a.metadata.x > b.metadata.x)
                                header.x = a.metadata.x;
                        else
                                header.x = b.metadata.x;
                
                        memcpy(output.pixelData, a.pixelData, a.dataSize * sizeof(NTHPST_COLOR_WIDTH));
                        memcpy(output.pixelData + a.dataSize, b.pixelData, b.dataSize * sizeof(NTHPST_COLOR_WIDTH));
                        
                }
                break;

        }


        std::fstream file(outputFile, std::ios::out | std::ios::binary);
        if(file.fail()) {
                PRINT_DEBUG("Unable to output joined texture; File not accessible.\n");
                return 1;
        }

        file.write((char*)&header, sizeof(header));
        file.write((char*)output.pixelData, output.dataSize * sizeof(NTHPST_COLOR_WIDTH));

        file.close();

        PRINT_DEBUG("Joined textures successfully.\n");
        return 0;
}








#undef SUPRESS_DEBUG_OUTPUT