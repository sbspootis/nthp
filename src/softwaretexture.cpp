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

        const size_t dataSize = ret.header.x * ret.header.y;
        ret.pixelData = new NTHPST_COLOR_WIDTH[dataSize];

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



// Joins two textures into a single ST file; pass the constexpr tools::JOIN_WIDTH and tools::JOIN_HEIGHT for the ordering.
const nthp::texture::SoftwareTexture::STdata nthp::texture::tools::joinSoftwareTextures(const char* textureFileA, const char* textureFileB, const bool joinMethod, const char* outputFile) {
        PRINT_DEBUG("Joining texture file [%s] with [%s]; output @ [%s]...\n", textureFileA, textureFileB, outputFile);
        nthp::texture::SoftwareTexture a;
        nthp::texture::SoftwareTexture b;
        nthp::texture::SoftwareTexture::STdata ret;
        ret.pixelData = NULL;
        ret.header.signature = 0;

        if(a.generateTexture(textureFileA, NULL, NULL) || b.generateTexture(textureFileB, NULL, NULL)) {
                PRINT_DEBUG("Failed to join textures.\n");
                return ret;
        }

        nthp::texture::SoftwareTexture::software_texture_header header;
        const size_t dataSize = (a.dataSize + b.dataSize) * sizeof(NTHPST_COLOR_WIDTH);

        ret.pixelData = new NTHPST_COLOR_WIDTH[dataSize];
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
                                memcpy(ret.pixelData + outputPosition, a.pixelData + aPosition, a.metadata.x * sizeof(NTHPST_COLOR_WIDTH));
                                outputPosition += a.metadata.x;
                                aPosition += a.metadata.x;

                                memcpy(ret.pixelData + outputPosition, b.pixelData + bPosition, b.metadata.x * sizeof(NTHPST_COLOR_WIDTH));
                                outputPosition += b.metadata.x;
                                bPosition += b.metadata.x;

                        } while(outputPosition < dataSize);

                }
                break;
        case JOIN_HEIGHT:
                {
                        header.y = a.metadata.y + b.metadata.y;
                        if(a.metadata.x > b.metadata.x)
                                header.x = a.metadata.x;
                        else
                                header.x = b.metadata.x;
                
                        memcpy(ret.pixelData, a.pixelData, a.dataSize * sizeof(NTHPST_COLOR_WIDTH));
                        memcpy(ret.pixelData + a.dataSize, b.pixelData, b.dataSize * sizeof(NTHPST_COLOR_WIDTH));
                        
                }
                break;

        }


        if(outputFile == NULL) {
                ret.header = header;

                PRINT_DEBUG("Joined textures successfully.\n");
                return ret;
        }
        else {
                std::fstream file(outputFile, std::ios::out | std::ios::binary);
                if(file.fail()) {
                        PRINT_DEBUG("Unable to output joined texture; File not accessible.\n");
                        return ret;
                }

                file.write((char*)&header, sizeof(header));
                file.write((char*)ret.pixelData, dataSize);

                file.close();

                PRINT_DEBUG("Joined textures successfully.\n");

                ret.header = header;
                delete[] ret.pixelData;

                return ret;
        }
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


        const size_t o_dataSize = (a_dataSize + b_dataSize);
        const size_t oBinary_dataSize = o_dataSize * sizeof(NTHPST_COLOR_WIDTH);

        PRINT_DEBUG("TextureDimensions ; ax=%zu ay=%zu; bx=%zu by=%zu\n", a.header.x, a.header.y, b.header.x, b.header.y);

        ret.pixelData = new NTHPST_COLOR_WIDTH[o_dataSize];

        switch(joinMethod) {
                case JOIN_WIDTH:
                        {
                                ret.header.x = a.header.x + b.header.x;
                                if(a.header.y > b.header.y)
                                        ret.header.y = a.header.y;
                                else
                                        ret.header.y = b.header.y;
                                
                                
                                uint32_t aPosition = 0;
                                uint32_t bPosition = 0;
                                size_t outputPosition = 0;
                                bool operationComplete = false;
                                
                                do {
                                        memcpy(ret.pixelData + outputPosition, a.pixelData + aPosition, a.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                        outputPosition += a.header.x;
                                        aPosition += a.header.x;

                                        memcpy(ret.pixelData + outputPosition, b.pixelData + bPosition, b.header.x * sizeof(NTHPST_COLOR_WIDTH));
                                        outputPosition += b.header.x;
                                        bPosition += b.header.x;

                                } while(outputPosition < o_dataSize);

                        }
                        break;
                case JOIN_HEIGHT:
                        {
                                ret.header.y = a.header.y + b.header.y;
                                if(a.header.x > b.header.x)
                                        ret.header.x = a.header.x;
                                else
                                        ret.header.x = b.header.x;
                        
                                memcpy(ret.pixelData, a.pixelData, a_dataSize * sizeof(NTHPST_COLOR_WIDTH));
                                memcpy(ret.pixelData + a_dataSize, b.pixelData, b_dataSize * sizeof(NTHPST_COLOR_WIDTH));
                                
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
                delete[] data->pixelData;
                data->header.x = 0;
                data->header.y = 0;
                data->header.signature = 0;

                return;
        }
}





#undef SUPRESS_DEBUG_OUTPUT