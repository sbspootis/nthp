#pragma once
#include "global.hpp"

namespace nthp {
namespace script {
       
        // The base unit of script execution. A Node contains an 8bit ID, defining instruction type.
        // It's a union (as you can see); when writing the Node to a file (or when writing a fixed size is important) write
        // the 'file_p' member, followed by the Node data (Node.access.data; write "Node.access.size" bytes).
        // When using the Node (data or otherwise) in code, use the 'access' member. 
        union Node {
        public:
                struct n_file_t {
                        uint8_t ID;
                        uint8_t size;
                } 

                file_p;
                struct n_binary_t {
                        uint8_t ID;
                        uint8_t size;
                        char* data;
                }
                
                access;

                Node() { access = {0,0,NULL}; }
                Node(uint8_t _ID, uint8_t dataSize) { access.ID = _ID; access.size = dataSize; }

        };

        inline void cleanNodeSet(std::vector<Node>& set) {
                for(size_t i = 0; i < set.size(); ++i) {
                        if(set[i].access.size) free(set[i].access.data);
                }
                set.clear();
        }
        inline void cleanNodeSet(nthp::script::Node* set, size_t size) {
                for(size_t i = 0; i < size; ++i) {
                        if(set[i].access.size) free(set[i].access.data);
                }
                if(size) { delete[] set; }
        }

        
                        // Use this to work with script triggers, but read and write from files with the
                        // 'trigger_w' 
        struct scriptTriggerComplex {
                uint16_t ID : 3;
                uint16_t GPR : 5;
                uint16_t MEM : 8;
        };

        constexpr size_t NodeSize = sizeof(Node);
        typedef nthp::fixed_t stdVarWidth;

        template<class T>
        struct P_Reference {
                T value;
                uint8_t metadata = 0; // See below
                uint8_t offset = 0;
                
                // Using some shift/mask/cast shit instead of bit-field for P_Reference
                // metadata. Bit-fields do not mix with files.

        };

        // PTR DESCRIPTOR STRUCTURE:
        // Vars that are assigned a ptr descriptor will have the following layout:
        //      - lower order bit half is the data location within a block.
        //      - higher order bit half is the block location within the stack.

        // The amount of block memory available is dependent on the fixed point system width,
        // as stdrefs are used as pointers.
        // Ex. FIXED_POINT_WIDTH = 16-bit
        //              bits [0-7] are data location within a block
        //              bits [8-15] are block location.
        //
        // Higher fixed point widths give access to more data and blocks to allocate. In this case,
        // a maxium of 255 blocks can be allocated, with a maximum size of 255 entries per block.
        //
        // Blocks are indexed starting at 1, as block 0 is the GLOBAL list, which cannot be changed and is defined by the
        // compiler. If a PTR VAR points to another VAR, the bits [8-15] will be 0, and bits [0-7] will be the index of the
        // VAR in the GLOBAL list. This means the same structure can be used to reference dynamic block data and static VARs.
        //
        // Data offsets (from STRUCTs in script) are applied exclusively to block memory access; even if a GLOBAL is assigned a
        // structure, points to another GLOBAL, and is accessed with a member, the offset will be ignored, even though it's syntaxically correct and the compiler
        // won't make a fuss.
        
        struct BlockMemoryEntry {
                typedef enum {
                        TYPELESS = 0,
                        TEXTURE,
                        FRAME,
                        ENTITY,
                        SETPIECE,
                        GLOBAL,

                        bmTypeSize
                } bmType;

                nthp::script::stdVarWidth* data;
                size_t size;
                size_t sizeSpecial;
        
                int type = bmType::TYPELESS;
                char isFree;
        };





        namespace internal_constants {
                constexpr FIXED_TYPE blockMemoryBitAllocation = FIXED_POINT_WIDTH / 2;
                constexpr FIXED_TYPE blockMemoryDataMask = (((FIXED_TYPE)1) << blockMemoryBitAllocation) - ((FIXED_TYPE)1); // Lower-order bit mask
                constexpr FIXED_TYPE blockMemoryBlockMask = ~(blockMemoryDataMask);             // Higher-order bit mask
        }

        typedef struct {
                FIXED_TYPE block        : nthp::script::internal_constants::blockMemoryBitAllocation;
                FIXED_TYPE address      : nthp::script::internal_constants::blockMemoryBitAllocation;
        } PtrDescriptor_st;

        constexpr int GLOBAL_LIST = 0;
        constexpr PtrDescriptor_st NULL_REF = {0,0};

        constexpr FIXED_TYPE constructPtrDescriptor(FIXED_TYPE block, FIXED_TYPE address) {
                using namespace nthp::script::internal_constants;
                const FIXED_TYPE result = ((block << blockMemoryBitAllocation) & blockMemoryBlockMask) | (address & blockMemoryDataMask);

                return result;
        }

        constexpr PtrDescriptor_st parsePtrDescriptor(FIXED_TYPE bin) {
                using namespace nthp::script::internal_constants;
                const PtrDescriptor_st ret = {(bin >> blockMemoryBitAllocation), (bin & blockMemoryDataMask)};

                return ret;
        }


        typedef enum __P_REF_FLAGS_BITS {
                IS_REFERENCE,
                IS_NEGATED,
                IS_PTR,          
                IS_NODE_STRING_PTR,
                IS_STRING,
                IS_OFFSET_DYNAMIC,
                IS_VALID
        } flagBits;


        #define PR_METADATA_SET(varRef, flag)           ((varRef.metadata) |= ( 1 <<( (int) flag )))
        #define PR_METADATA_CLEAR(varRef, flag)         ((varRef.metadata) &= ~( 1 <<( (int) flag )))
        #define PR_METADATA_GET(varRef, flag)           ((varRef.metadata >> ((int) flag )) & 1)

}
}