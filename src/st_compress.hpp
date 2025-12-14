#pragma once


#include "global.hpp"
#include "softwaretexture.hpp"


// A compression node's blockSize width in BYTES; The maximum number of 'C_DATA_WIDTH's
// that can be compressed into a single node.
#define C_BLOCK_WIDTH   1



namespace nthp {
        namespace texture {
                namespace compression {
#if C_BLOCK_WIDTH == 1
                                typedef uint8_t c_BlockWidth;
                                constexpr c_BlockWidth c_maxBlockSize = UINT8_MAX;
#else
        #if C_BLOCK_WIDTH == 2
                                typedef uint16_t c_BlockWidth;
                                constexpr c_BlockWidth c_maxBlockSize = UINT16_MAX;
        #else
                #if C_BLOCK_WIDTH == 4
                                typedef uint32_t c_BlockWidth;
                                constexpr c_BlockWidth c_maxBlockSize = UINT32_MAX;
                #else
                        #if C_BLOCK_WIDTH == 8
                                typedef uint64_t c_BlockWidth;
                                constexpr c_BlockWidth c_maxBlockSize = UINT64_MAX;
                        
                        #else
                                typedef uint8_t c_BlockWidth;
                                constexpr c_BlockWidth c_maxBlockSize = UINT8_MAX;
                        #endif
                #endif
        #endif
#endif

        typedef NTHPST_COLOR_WIDTH c_DataWidth;

       
                struct Node {
                        Node() { }
                        Node(c_BlockWidth w, c_DataWidth d) { blockSize = w; data = d; }

                        c_BlockWidth blockSize;
                        c_DataWidth data;
                };

                // The header in a decompressed softwareTexture and a compressed one is the same, but the signature
                // value is different. That's why it's a typedef, not a whole other thing yo.
                struct CST_header {
                        uint8_t signature;
                        uint32_t x;
                        uint32_t y;
                        uint64_t nodeCount;
                };

                static const uint8_t CSTHeaderSignature = 0b11101101;


                // Does not deal in real softwareTexture files. When loading from a file, must use a compressed st file (.cst),
                // not a raw softwareTexture (.st).


                extern int compressSoftwareTextureFile(const char* inputFile, const char* output);
                extern int compressSoftwareTexture(nthp::texture::SoftwareTexture* texture, const char* output);

                extern nthp::texture::SoftwareTexture* decompressTexture(const char* filename);


                }
                
                // For convenience. Checks the header of the given file to deduce if it's compressed or not,
                // decompresses (or just loads as a texture directly), and generates a valid software texture.
                // NOTE: Texture must be 'regenerated' with a palette to be valid for rendering. Works "out of the box"
                //       with both compressed and decompressed textures.
                extern nthp::texture::SoftwareTexture* auto_generateTexture(const char* filename);
        }

}
