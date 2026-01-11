#define SUPPRESS_DEBUG_OUTPUT

#include "gtexture.hpp"
#include "core.hpp"


void nthp::texture::gTexture::init() {
        texture.init();
}

nthp::texture::gTexture::gTexture() {
        init();
}



nthp::texture::gTexture::gTexture(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer) {
        init();
        if(autoLoadTextureFile(filename, palette, coreRenderer)) {

        }
}


int nthp::texture::gTexture::autoLoadTextureFile(const char* filename, nthp::texture::Palette* palette, SDL_Renderer* coreRenderer) {
        nthp::texture::SoftwareTexture::software_texture_header header;
        {
                std::fstream file(filename, std::ios::in | std::ios::binary);

                if(file.fail()) {
                        PRINT_DEBUG_ERROR("Unable to generate texture [%s]; File not accessible.\n", filename);
                        return 1;
                }

                
                file.read((char*)&header, sizeof(header));

                file.close();
        }

        do {
                if(header.signature == nthp::texture::SoftwareTexture::STheaderSignature) {
                        nthp::texture::SoftwareTexture::STdata load = nthp::texture::tools::readTextureData(filename);
                        if(load.header.signature != nthp::texture::SoftwareTexture::STheaderSignature) { return 1; }

                        this->texture.pixelData = load.pixelData;
                        this->texture.metadata = load.header;
                        this->texture.dataSize = load.header.x * load.header.y;
                        this->texture.regenerateTexture(palette, coreRenderer);
                        break;
                }
                if(header.signature == nthp::texture::compression::CSTHeaderSignature) {
                        decompressFile(filename);
                        texture.regenerateTexture(palette, coreRenderer);
                        break;
                }
               
        
                PRINT_DEBUG_ERROR("Unable to generate texture [%s]; Invalid file format.\n");
                return 1;
        } while(0);

        return 0;
}

int nthp::texture::gTexture::decompressFile(const char* filename) {
        std::fstream file(filename, std::ios::in | std::ios::binary);
        PRINT_DEBUG("Attempting decompression of cst file [%s]...\n", filename);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to decompress texture [%s]; Unable to open file.\n", filename);
                return 1;
        }

        nthp::texture::compression::CST_header header;
        file.read((char*)&header, sizeof(header));

        if(header.signature != nthp::texture::compression::CSTHeaderSignature) {
                PRINT_DEBUG_ERROR("Unable to decompress texture [%s]; Inavlid file format.\n", filename);
                return 1;
        }

        const size_t pixelDataSize = header.x * header.y;
        PRINT_DEBUG("Detected Header data; NodeCount = %llu\n", header.nodeCount);

        texture.createEmptyTexture(pixelDataSize);
        {
                nthp::texture::SoftwareTexture::software_texture_header conv_header;
                conv_header.signature = nthp::texture::SoftwareTexture::STheaderSignature;
                conv_header.x = header.x;
                conv_header.y = header.y;
                texture.manual_metadata_override(conv_header);
        }

        nthp::texture::compression::Node currentNode;
        size_t currentAbsolutePosition = 0;

        for(size_t i = 0; i < header.nodeCount; ++i) {
                file.read((char*)&currentNode, sizeof(currentNode));
                PRINT_DEBUG("Expanding node [%zu] : s=%u d=%u...", i, currentNode.blockSize, currentNode.data);

                for(nthp::texture::compression::c_BlockWidth nEI = 0; nEI < currentNode.blockSize; ++nEI) {
                        texture.getPixelData()[currentAbsolutePosition] = currentNode.data;
                        ++currentAbsolutePosition;
                }
                NOVERB_PRINT_DEBUG("\t\t\tDone.\n");
        }

        file.close();

        PRINT_DEBUG("Successfully decompressed softwareTexture; Output at [%p].\nTotal iterations: %llu.\n", texture, currentAbsolutePosition);
        return 0;
}


void nthp::texture::gTexture::clean() {
        texture.purgeTextureData();
}