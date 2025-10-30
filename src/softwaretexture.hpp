#pragma once
#include "rawsurface.hpp"

namespace nthp {
        namespace texture {
                


                class SoftwareTexture {
                public:
                        SoftwareTexture();
                        SoftwareTexture(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer);

                        // Read ST data from a valid ST file. Does NOT generate a valid rendering texture as no palette is given.
                        // Must use 'regenerateTexture()' with a palette to generate.
                        SoftwareTexture(const char* filename);

                        void init();
                        virtual int generateTexture(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer);
                        inline SDL_Texture* getTexture() { return texture; }

                        void regenerateTexture(nthp::texture::Palette* palette, SDL_Renderer* renderer);
                        
                        static constexpr uint8_t STheaderSignature = 0b11011001;
                        struct software_texture_header {
                                uint8_t signature;
                                uint32_t x;
                                uint32_t y;
                        };

                        struct STdata {
                                software_texture_header header;
                                NTHPST_COLOR_WIDTH* pixelData;
                        };

                        // Set the bits here for the color mask; the alpha mask is just the inverse.
                        static constexpr NTHPST_COLOR_WIDTH NTHPST_COLORMASK = 0b1111111111110000;


                        static constexpr NTHPST_COLOR_WIDTH NTHPST_ALPHAMASK = (~NTHPST_COLORMASK);
                        static constexpr NTHPST_COLOR_WIDTH alphaLevelSize = UINT8_MAX / NTHPST_ALPHAMASK;
                        static constexpr NTHPST_COLOR_WIDTH alphaBitCount = 4;

                        NTHPST_COLOR_WIDTH* getPixelData() { return pixelData; }
                        const size_t getPixelDataSize() { return dataSize; }
			

                        void createEmptyTexture(const size_t dataSize);
                        const software_texture_header getMetaData() { return metadata; }
                        inline void manual_metadata_override(const software_texture_header _ovr) { metadata = _ovr; dataSize = metadata.x * metadata.y; }

                        // Cleans up texture data (SDL and rawST) and resets the object to its initialized state.
                        inline void purgeTextureData() {
                                PRINT_DEBUG("Purging texture @ [%p]...\t", this);
                                if(dataSize > 0) free(pixelData);
                                if(texture != nullptr) SDL_DestroyTexture(texture);

                                dataSize = 0;
                                metadata.x = 0;
                                metadata.y = 0;

                                NOVERB_PRINT_DEBUG("done.\n");
                        }

                        // Destroys the texture's raw ST data, leaving the compiled SDL texture; use if memory
                        // is becomming an issue, or a texture is persistant and will never need to be regenerated.
                        inline void cleanSTData() {
                                if(dataSize > 0) free(pixelData);
                                dataSize = 0;
                        }

                        ~SoftwareTexture();


                        
                        NTHPST_COLOR_WIDTH* pixelData;
                        size_t dataSize;
                        software_texture_header metadata;
                        SDL_Texture* texture;
                        
                protected:

                        constexpr NTHPST_COLOR_WIDTH getPixelColor(NTHPST_COLOR_WIDTH pixel)        { return ((pixel & NTHPST_COLORMASK) >> alphaBitCount); }
                        constexpr NTHPST_COLOR_WIDTH getPixelAlphaLevel(NTHPST_COLOR_WIDTH pixel)        { return (pixel & NTHPST_ALPHAMASK); }
                        constexpr NTHPST_COLOR_WIDTH getTrueAlpha(NTHPST_COLOR_WIDTH alphaLevel)    { return (alphaLevel * alphaLevelSize); }

                };
                namespace tools {
                

                #if USE_SDLIMG == 1
                        extern int generatePaletteFromImage(const char* inputImageFile, const char* outputFile);
                        extern int generateSoftwareTextureFromImage(const char* inputImageFile, nthp::texture::Palette* palette, const char* outputFile);
                #endif
                
                constexpr bool JOIN_WIDTH = false;
                constexpr bool JOIN_HEIGHT = true;
                
                extern nthp::texture::SoftwareTexture::STdata readTextureData(const char* textureFile);
                extern int writeTextureData(nthp::texture::SoftwareTexture::STdata data, const char* outputFile);

                extern const nthp::texture::SoftwareTexture::STdata joinSoftwareTextures(nthp::texture::SoftwareTexture::STdata textureA, nthp::texture::SoftwareTexture::STdata textureB, bool joinMethod);

                extern void destroySTdata(nthp::texture::SoftwareTexture::STdata* data);
                extern constexpr NTHPST_COLOR_WIDTH encodePixel(const NTHPST_COLOR_WIDTH colorIndex, const NTHPST_COLOR_WIDTH alphaLevel);
                }

                struct Frame {
                        void init() { texture = nullptr; src = { 0,0,0,0 }; }

                        SDL_Texture*    texture;
                        SDL_Rect        src;
                };


                
        }
}
