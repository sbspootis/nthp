#include "s_script.hpp"
using namespace nthp::script::instructions;
#define DEFINE_EXECUTION_BEHAVIOUR(instruction) static const int instruction (nthp::script::Script::ScriptDataSet* data)

nthp::texture::Palette nthp::script::activePalette;

// Immediately reserved cache for standard references.
static nthp::script::instructions::stdRef refCache[10];


static inline int ____eval_std(stdRef& ref, nthp::script::Script::ScriptDataSet* data) {
        if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_REFERENCE)) {
                {
                        const auto ptr = nthp::script::parsePtrDescriptor(ref.value);
                #ifdef DEBUG
                        if(((ptr.block) >= data->blockDataSize)) {
                                PRINT_DEBUG_ERROR("Invalid access; invalid block @ [%zu]; b=%d.\n", data->currentNode, ptr.block);
                                return 1;
                        }
                        if(ptr.address >= data->blockData[ptr.block].size) {
                                PRINT_DEBUG_ERROR("Invalid access; out of bounds @ [%zu]; b=%d ad=%d.\n", data->currentNode, ptr.block, ptr.address);
                                return 1;
                        }
                #endif
                        ref.value = data->blockData[ptr.block].data[ptr.address];
                }

                // Copy the reference in the following data node; the compiler will add a data node to the list when a dynamic offset is
                // used. The offset value stored in the current ref is the dynamic offset's position in the data node (if the flag is set).
                if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_OFFSET_DYNAMIC)) {
                        stdRef offsetEval = *(stdRef*)(data->nodeSet[data->currentNode + 1].access.data + (sizeof(stdRef) * ref.offset));
                        ____eval_std(offsetEval, data);

                        ref.offset = nthp::fixedToInt(offsetEval.value);
                }


                if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_PTR)) {
                        const auto ptr = nthp::script::parsePtrDescriptor(ref.value);
                #ifdef DEBUG
                        if(((ptr.block) >= data->blockDataSize)) {
                                PRINT_DEBUG_ERROR("Invalid access; invalid block @ [%zu]; b=%d.\n", data->currentNode, ptr.block);
                                return 1;
                        }
                        if(ptr.address >= data->blockData[ptr.block].size) {
                                PRINT_DEBUG_ERROR("Invalid access; out of bounds @ [%zu]; b=%d ad=%d.\n", data->currentNode, ptr.block, ptr.address);
                                return 1;
                        }
                #endif
                        ref.value = data->blockData[ptr.block].data[ptr.address + ref.offset];
                }
                        
                if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_NEGATED)) ref.value = -(ref.value);
        }

        return 0;
}
        

#ifdef DEBUG
        #define EVAL_STDREF(ref)        { if(____eval_std(ref, data)) { return 1; } }
#else
        #define EVAL_STDREF(ref)        ____eval_std(ref, data)
#endif


static inline int ___eval_ptr(stdRef& ref, nthp::script::stdVarWidth** target_dsc, nthp::script::Script::ScriptDataSet* data) {
        EVAL_STDREF(ref);
        const auto ptr = nthp::script::parsePtrDescriptor(ref.value);
        #ifdef DEBUG
                if(((ptr.block) >= data->blockDataSize)) {
                        PRINT_DEBUG_ERROR("Invalid access; invalid block @ [%zu]; b=%d.\n", data->currentNode, ptr.block);
                        return 1;
                }
                if(ptr.address >= data->blockData[ptr.block].size) {
                        PRINT_DEBUG_ERROR("Invalid access; out of bounds @ [%zu]; b=%d ad=%d.\n", data->currentNode, ptr.block, ptr.address);
                        return 1;
                }
                if((ptr.block + ptr.address) == 0) {
                        PRINT_DEBUG_ERROR("Invalid access; unauthorized write to null/ERRORLEVEL. @ [%zu]; b=%d ad=%d.\n", data->currentNode, ptr.block, ptr.address);
                        return 1;
                }
        #endif

        (*target_dsc) = (data->blockData[ptr.block].data + ptr.address + ref.offset);
        return 0;
}

#ifdef DEBUG
        #define EVAL_PTRREF(ref) nthp::script::stdVarWidth* target_dsc; if(___eval_ptr(ref, &target_dsc, data)) { return 1; }
#else
        #define EVAL_PTRREF(ref) nthp::script::stdVarWidth* target_dsc; ___eval_ptr(ref, &target_dsc, data)
#endif


inline char* ____eval_str(ptrRef& ref, nthp::script::Script::ScriptDataSet* data) {
        if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_NODE_STRING_PTR)) {
                return data->nodeSet[nthp::fixedToInt(ref.value) + data->currentScriptHeaderLocation].access.data;
        }
#ifdef DEBUG 
        if(____eval_std(ref, data)) { return NULL; }
#else
        EVAL_STDREF(ref);
#endif
        const auto ptr_dsc = nthp::script::parsePtrDescriptor(ref.value);
        return (char*)(data->blockData[ptr_dsc.block].data + ptr_dsc.address);
}

// Allows evaluating whole blocks as custom, non-script-dynamic types.
// Uses the pointer descriptor address + offset as the special type's index in the block, rather than binary position.
// Returns a pointer of template type to the object at position address + offset.
template<class Type>
static inline Type* eval_special(stdRef ref, nthp::script::Script::ScriptDataSet* data, const nthp::script::BlockMemoryEntry::bmType expectedType) {
#ifdef DEBUG
        if(____eval_std(ref, data)) { return NULL; }
#else
        EVAL_STDREF(ref);
#endif

        const auto ptr = nthp::script::parsePtrDescriptor(ref.value);
#ifdef DEBUG
        if(data->blockData[ptr.block].type != expectedType) {
                PRINT_DEBUG_ERROR("Expected type [%02zX] in eval_special @ [%zu]; b=%d, type=[%02zX]\n", expectedType, data->currentNode, ptr.block, data->blockData[ptr.block].type);
                return NULL;
        }
#endif
        Type* const target = (Type*)(data->blockData[ptr.block].data);

        // If IGNORE_SPECIAL_OFFSET is zero, multiply the offset by 1, otherwise it zeros the offset value.
        return (target + (ptr.address + (ref.offset * (!PR_METADATA_GET(ref, nthp::script::flagBits::IGNORE_SPECIAL_OFFSET)))));
}


#define EVAL_ENTREF(ref)                eval_special<nthp::entity::gEntity>(ref, data, nthp::script::BlockMemoryEntry::bmType::ENTITY)
#define EVAL_TEXTUREREF(ref)            eval_special<nthp::texture::gTexture>(ref, data, nthp::script::BlockMemoryEntry::bmType::TEXTURE)
#define EVAL_FRAMEREF(ref)              eval_special<nthp::texture::Frame>(ref, data, nthp::script::BlockMemoryEntry::bmType::FRAME)
#define EVAL_SETPIECEREF(ref)           eval_special<nthp::entity::staticSetpiece>(ref, data, nthp::script::BlockMemoryEntry::bmType::SETPIECE)

#define EVAL_STRREF(ref)        ____eval_str(ref, data)


#ifdef DEBUG
        volatile nthp::vectGeneric nthp::script::debug::debugInstructionCall = nthp::vectGeneric(-1, -1);
        bool nthp::script::debug::suspendExecution = false;
#endif



DEFINE_EXECUTION_BEHAVIOUR(EXIT) {
        data->isSuspended = true;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(HEADER) {
        data->currentScriptHeaderLocation = data->currentNode;

        
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LABEL) {
        return 0;
}

// Fastest instruction jumping; matched by compiler within the same unit.
DEFINE_EXECUTION_BEHAVIOUR(GOTO) {
        data->currentNode = (*(uint32_t*)data->nodeSet[data->currentNode].access.data) + data->currentScriptHeaderLocation;
        --data->currentNode;

        return 0;
}

// Jump allows direct switching to any node in the program. Unless specifically planned, must use GETINDEX
// as an anchor to determine which instructions to jump to.
DEFINE_EXECUTION_BEHAVIOUR(JUMP) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        data->currentNode = nthp::fixedToInt(refCache[0].value);
        data->currentScriptHeaderLocation = nthp::script::Script::findInstructionHeader(data->nodeSet, nthp::fixedToInt(refCache[0].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SUSPEND) {
        data->isSuspended = true;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(RETURN) {
        --data->stackPointer;
        const nthp::script::Script::ReturnStackEntry ret = data->returnStack[data->stackPointer];

        data->currentScriptHeaderLocation = ret.sourceHeaderLocation;
        data->currentNode = ret.sourceDestination;

        --data->currentNode;
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(GETINDEX) {
        refCache[0] = *(ptrRef*)data->nodeSet[data->currentNode].access.data;
        EVAL_PTRREF(refCache[0]);

        *target_dsc = nthp::intToFixed(data->currentNode);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DATA) {
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(INC) {
        refCache[0] = *(ptrRef*)data->nodeSet[data->currentNode].access.data;
        EVAL_PTRREF(refCache[0]);

        *target_dsc = (*target_dsc) + nthp::intToFixed(1);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DEC) {
        refCache[0] = *(ptrRef*)data->nodeSet[data->currentNode].access.data;
        EVAL_PTRREF(refCache[0]);

       
        *target_dsc = (*target_dsc) - nthp::intToFixed(1);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LSHIFT) {
        refCache[0] =  *(ptrRef*)data->nodeSet[data->currentNode].access.data;
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));

        EVAL_PTRREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        
        *target_dsc = (*target_dsc) << nthp::fixedToInt(refCache[1].value);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(RSHIFT) {
        refCache[0] =  *(ptrRef*)data->nodeSet[data->currentNode].access.data;
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));
        
        
        EVAL_PTRREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        
        *target_dsc = (*target_dsc) >> nthp::fixedToInt(refCache[1].value);

      
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(ADD) {
        refCache[0] = *(stdRef*)data->nodeSet[data->currentNode].access.data;                                           // a
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                        // b
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));       // output


        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);


        *target_dsc = (refCache[0].value + refCache[1].value);
       

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SUB) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));


        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);


        *target_dsc = (refCache[0].value - refCache[1].value);

       
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUL) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));


        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);


        *target_dsc = nthp::f_fixedProduct(refCache[0].value, refCache[1].value);
      
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DIV) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));


        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        *target_dsc = nthp::f_fixedQuotient(refCache[0].value, refCache[1].value);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SQRT) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::f_sqrt(refCache[0].value);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(POW) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                         // Base
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                        // Exponent
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));       // Output

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        nthp::script::stdVarWidth math = refCache[0].value;
        for(int i = 1; i < nthp::fixedToInt(refCache[1].value); ++i) { math = nthp::f_fixedProduct(math, refCache[0].value); }
        
        (*target_dsc) = math;
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ABS) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // Value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // output


        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);


        *target_dsc = std::abs(refCache[0].value);
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(MOD) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                         // Value
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                        // Divisor
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));       // Output

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        (*target_dsc) = nthp::intToFixed(nthp::fixedToInt(refCache[0].value) % nthp::fixedToInt(refCache[1].value));
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(RAND) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                         // a (min)
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                        // b (max)
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));       // output

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        (*target_dsc) = nthp::intToFixed((rand() % (nthp::fixedToInt(refCache[1].value - refCache[0].value))) + nthp::fixedToInt(refCache[0].value));
        return 0;
}


// Like hell I'm writing a fixed point trigonometry set myself. Just take the slower
// trig up the ass, not worth my time.
DEFINE_EXECUTION_BEHAVIOUR(SIN) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::doubleToFixed(std::sin(nthp::fixedToDouble(refCache[0].value)));  // two conversions! Why even use fixed point in the first place?!
        return 0;
}



DEFINE_EXECUTION_BEHAVIOUR(COS) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::doubleToFixed(std::cos(nthp::fixedToDouble(refCache[0].value)));  // two conversions! Why even use fixed point in the first place?!
        return 0;
}



DEFINE_EXECUTION_BEHAVIOUR(TAN) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::doubleToFixed(std::tan(nthp::fixedToDouble(refCache[0].value)));  // two conversions! Why even use fixed point in the first place?!
        return 0;
}



DEFINE_EXECUTION_BEHAVIOUR(ASIN) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::doubleToFixed(std::asin(nthp::fixedToDouble(refCache[0].value)));  // two conversions! Why even use fixed point in the first place?!
        return 0;
}



DEFINE_EXECUTION_BEHAVIOUR(ACOS) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::doubleToFixed(std::acos(nthp::fixedToDouble(refCache[0].value)));  // two conversions! Why even use fixed point in the first place?!
        return 0;
}



DEFINE_EXECUTION_BEHAVIOUR(ATAN) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        *target_dsc = nthp::doubleToFixed(std::atan(nthp::fixedToDouble(refCache[0].value)));  // two conversions! Why even use fixed point in the first place?!
        return 0;
}



DEFINE_EXECUTION_BEHAVIOUR(END) {
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ELSE) {
        uint32_t endLocation = *(uint32_t*)(data->nodeSet[data->currentNode].access.data);

        data->currentNode = endLocation + data->currentScriptHeaderLocation;
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SKIP) {
        uint32_t skip_to = *(uint32_t*)(data->nodeSet[data->currentNode].access.data);

        data->currentNode = skip_to + data->currentScriptHeaderLocation;
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SKIP_END) {
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LOGIC_IF_TRUE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(uint32_t));


        EVAL_STDREF(refCache[0]);
        if(refCache[0].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }
        
        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(LOGIC_EQU) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                  // opA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                 // opB
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t)));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        if(refCache[0].value == refCache[1].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }

        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LOGIC_NOT) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                  // opA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                 // opB
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t)));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        if(refCache[0].value != refCache[1].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }

        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LOGIC_GRT) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                  // opA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                 // opB
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t)));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        if(refCache[0].value > refCache[1].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }

        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LOGIC_LST) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                  // opA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                 // opB
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t)));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        if(refCache[0].value < refCache[1].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }

        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(LOGIC_GRTE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                  // opA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                 // opB
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t)));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        if(refCache[0].value >= refCache[1].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }

        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(LOGIC_LSTE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                  // opA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                 // opB
        uint32_t endIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        uint32_t elseIndex = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t)));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        if(refCache[0].value <= refCache[1].value) return 0;
        if(elseIndex) { data->currentNode = elseIndex + data->currentScriptHeaderLocation; return 0; }

        data->currentNode = endIndex + data->currentScriptHeaderLocation;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SET) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                         // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));        // value

        EVAL_PTRREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        *target_dsc = refCache[1].value;
       
        return 0;
}


// Internal allocator function to occupy a new block list entry. Only allocates in units of nthp::script::stdVarWidth.
// Returns a PtrDescriptor_st containing the block occupied. Block will be 0 on failure.
// size_t target parameter takes an index in the block list to allocate to a specific register; if target is 0, then the allocator
// will either find a vacant register or resize the block list to add a new register on top.
const nthp::script::PtrDescriptor_st nthp::script::nthp_internal_alloc(nthp::script::Script::ScriptDataSet* data, nthp::script::stdVarWidth* target_dsc, nthp::fixed_t size, size_t target, nthp::script::BlockMemoryEntry::bmType type) {
        if(target) {
                if(!data->blockData[target].isFree) {   PRINT_DEBUG_WARNING("ALLOC_TARGET target [%zu] not vacant; block data will be erased. SPECIAL TYPES ARE NOT CLEANED PROPERLY.\n", target);
                                                        free(data->blockData[target].data); }
                data->blockData[target].data = (nthp::script::stdVarWidth*)malloc(sizeof(nthp::script::stdVarWidth) * nthp::fixedToInt(size));
                        
              
                if(data->blockData[target].data == NULL) {
                        PRINT_DEBUG_ERROR("Unable to allocate data block at [%p] (%zu).\n",data->blockData + target, target);
                        return nthp::script::NULL_REF;
                }


                data->blockData[target].size = nthp::fixedToInt(size);
                data->blockData[target].isFree = false;
                if(target_dsc != nullptr) *target_dsc = nthp::script::constructPtrDescriptor(target, 0); // Initalize the ptr to the first element in the allocated block.
                data->blockData[target].type = type;

                return nthp::script::parsePtrDescriptor(nthp::script::constructPtrDescriptor(target, 0));       
        }

        
        
        // Linear search for open blocks. If none, reallocate block memory and use
        // last entry.
        for(size_t i = 0; i < data->blockDataSize; ++i) {
                if(data->blockData[i].isFree) {
                        data->blockData[i].data = (nthp::script::stdVarWidth*)malloc(sizeof(nthp::script::stdVarWidth) * nthp::fixedToInt(size));
                        
              
                        if(data->blockData[i].data == NULL) {
                                PRINT_DEBUG_ERROR("Unable to allocate data block at [%p] (%zu).\n",data->blockData + i, i);
                                return nthp::script::NULL_REF;
                        }
        
        
                        data->blockData[i].size = nthp::fixedToInt(size);
                        data->blockData[i].isFree = false;
                        if(target_dsc != nullptr) *target_dsc = nthp::script::constructPtrDescriptor(i, 0); // Initalize the ptr to the first element in the allocated block.
                        data->blockData[i].type = type;
                        return nthp::script::parsePtrDescriptor(nthp::script::constructPtrDescriptor(i, 0));
                }
        }

        

        ++data->blockDataSize;
        nthp::script::BlockMemoryEntry* temp = (nthp::script::BlockMemoryEntry*)realloc(data->blockData, sizeof(nthp::script::BlockMemoryEntry) * data->blockDataSize);

        if(temp == NULL) {
                PRINT_DEBUG_ERROR("Unable to resize data block at [%p].\n", data->blockData);
                return nthp::script::NULL_REF;
        }
        data->blockData = temp;
        
        data->blockData[data->blockDataSize - 1].data = (nthp::script::stdVarWidth*)malloc(sizeof(nthp::script::stdVarWidth) * nthp::fixedToInt(size));
        data->blockData[data->blockDataSize - 1].size = nthp::fixedToInt(size);
        data->blockData[data->blockDataSize - 1].isFree = false;
        data->blockData[data->blockDataSize - 1].type = type;

        if(target_dsc != nullptr) *target_dsc = nthp::script::constructPtrDescriptor(data->blockDataSize - 1, 0); // Initalize the ptr to the first element in the allocated block.

        return nthp::script::parsePtrDescriptor(nthp::script::constructPtrDescriptor(data->blockDataSize - 1, 0));
}

// Runs nthp_internal_allocator to the size of <SpecialType> and initializes the new memory with SpecialType.init(). Target type must have
// a virtual init() function to be allocated with this.
template<class SpecialType>
SpecialType* nthp::script::nthp_internal_alloc_special(nthp::script::Script::ScriptDataSet* data, nthp::script::stdVarWidth* target_dsc, nthp::script::stdVarWidth entries, nthp::script::BlockMemoryEntry::bmType type) {
        

	const auto bytes = (sizeof(SpecialType) * entries);
        const auto stdEntries = nthp::intToFixed((bytes / sizeof(nthp::script::stdVarWidth)) + 1);

        const auto ptr_eval = nthp::script::nthp_internal_alloc(data, target_dsc, stdEntries, 0, type);
        if(ptr_eval.block == 0) { PRINT_DEBUG_ERROR("Special Allocation failed @ inst. [%zu].\n", data->currentNode); return nullptr; }

        SpecialType* specialBlock = (SpecialType*)(data->blockData[ptr_eval.block].data);
        if(specialBlock == NULL) { return nullptr; }

        for(size_t i = 0; i < entries; ++i) { 
                specialBlock[i].init(); 
                PRINT_DEBUG("Init special @ [%p]; type=%d\n", specialBlock + i, (int)type);
        }
        data->blockData[ptr_eval.block].sizeSpecial = entries;

        PRINT_DEBUG("Successful alloc_special() @ [%p] ; [b%ua%u], sizeSpecial=%zu.\n", specialBlock, ptr_eval.block, ptr_eval.address, data->blockData[ptr_eval.block].sizeSpecial);

        return specialBlock;
}



DEFINE_EXECUTION_BEHAVIOUR(ALLOC) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // size
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        if(nthp::script::nthp_internal_alloc(data, target_dsc, refCache[0].value, 0, nthp::script::BlockMemoryEntry::bmType::TYPELESS).block == 0) { return 1; }
        
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(NEW) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // size
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));   // output
        const uint32_t entrySize = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(ptrRef));

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        const auto out = nthp::script::nthp_internal_alloc(data, target_dsc, nthp::intToFixed(nthp::fixedToInt(refCache[0].value) * entrySize), 0, nthp::script::BlockMemoryEntry::bmType::TYPELESS);
        data->blockData[out.block].sizeSpecial = nthp::fixedToInt(refCache[0].value);
        if(out.block == 0) { return 1; }

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(FREE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target

        EVAL_PTRREF(refCache[0]);
        const auto ptr_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);
        
        // Will refuse of the block is zero (attempted free on global list)
        if(ptr_dsc.block) {
                if((!data->blockData[ptr_dsc.block].isFree)) free(data->blockData[ptr_dsc.block].data);
                data->blockData[ptr_dsc.block].data = nullptr;
                data->blockData[ptr_dsc.block].isFree = true;
                data->blockData[ptr_dsc.block].size = 0;
                return 0;
        }

        PRINT_DEBUG_ERROR("FREE at [%zu] Attempted to free global list.\n", data->currentNode);
        return 1;
}

DEFINE_EXECUTION_BEHAVIOUR(COPY) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                          // src
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));        // size
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef) + sizeof(stdRef));        // dst

        nthp::script::stdVarWidth* r_src = nullptr;
        {                                       // Fucky shit you have to do when evaluating 2 ptrrefs.
                EVAL_PTRREF(refCache[0]);
                r_src = target_dsc;
        }
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        memcpy(target_dsc, r_src, nthp::fixedToInt(refCache[1].value) * sizeof(nthp::script::stdVarWidth));

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(NEXT) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data); // target
        const uint8_t offset = *(uint8_t*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));
        
        EVAL_PTRREF(refCache[0]);

       
        *target_dsc = (*target_dsc + offset);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(PREV) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);
        const uint8_t offset = *(uint8_t*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));
        
        EVAL_PTRREF(refCache[0]);

        *target_dsc = (*target_dsc - offset);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(INDEX) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                          // ptr
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));        // addr
        const uint8_t offsetSize = *(uint8_t*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef) + sizeof(stdRef));

        EVAL_PTRREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        *target_dsc = nthp::script::constructPtrDescriptor(nthp::script::parsePtrDescriptor((*target_dsc)).block, (nthp::fixedToInt(refCache[1].value) * offsetSize));
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(LAST) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target
        uint8_t offsetSize = *(uint8_t*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));

        EVAL_PTRREF(refCache[0]);
        auto ptr = nthp::script::parsePtrDescriptor(*target_dsc);
        
        {       // This is a safety check for the pointer after eval; the regular checks are run on the stored pointer
                // to ensure validity. Weird fucky hack to make it work with as little effort as possible.

                ptrRef temp;
                temp.value = nthp::script::constructPtrDescriptor(ptr.block, ptr.address);

                EVAL_PTRREF(temp);
        }

        if(data->blockData[ptr.block].type != nthp::script::BlockMemoryEntry::bmType::TYPELESS) {
                (*target_dsc) = nthp::script::constructPtrDescriptor(nthp::script::parsePtrDescriptor((*target_dsc)).block, data->blockData[ptr.block].sizeSpecial - 1);
        }
        else {
                (*target_dsc) = nthp::script::constructPtrDescriptor(nthp::script::parsePtrDescriptor((*target_dsc)).block, (data->blockData[ptr.block].sizeSpecial - 1) * offsetSize);
        }

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(GET_BLOCKSIZE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                        // block
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        (*target_dsc) = nthp::intToFixed(data->blockData[nthp::script::parsePtrDescriptor(refCache[0].value).block].size);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SET_BLOCKLISTSIZE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data); // size

        EVAL_STDREF(refCache[0]);

        auto newSizeBytes = nthp::fixedToInt(refCache[0].value) * sizeof(nthp::script::BlockMemoryEntry) + 1;
        auto temp = (nthp::script::BlockMemoryEntry*)realloc(data->blockData, newSizeBytes);

        if(temp != NULL) { data->blockData = temp; }
        else {
                PRINT_DEBUG_ERROR("Failed to resize BLOCK LIST.\n");
                return 1;
        }

        for(size_t i = data->blockDataSize; i < nthp::fixedToInt(refCache[0].value); ++i) {
                data->blockData[i].isFree = true;
                data->blockData[i].type = nthp::script::BlockMemoryEntry::bmType::TYPELESS;
                data->blockData[i].size = 0;
                data->blockData[i].data = nullptr;
        }

        data->blockDataSize = nthp::fixedToInt(refCache[0].value);

        PRINT_DEBUG("Reserved fixed BLOCK LIST size; size=%u.\n", nthp::fixedToInt(refCache[0].value));

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ALLOC_TARGET) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                         // size
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                       // block
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));     // output

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        if(refCache[1].value <= 0 || refCache[1].value > nthp::intToFixed(data->blockDataSize)) {
                PRINT_DEBUG_ERROR("Invalid target for ALLOC_TARGET @ [%zu]; Entry [%d] not registered.\n", data->currentNode, nthp::fixedToInt(refCache[1].value));
                return 1;
        }

        const auto result = nthp::script::nthp_internal_alloc(data, target_dsc, refCache[0].value, nthp::fixedToInt(refCache[1].value), nthp::script::BlockMemoryEntry::bmType::TYPELESS);
        if(result.block == nthp::script::NULL_REF.block) { return 1; }

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(TEXTURE_ALLOC) {
	refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // size
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // target
	
	EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);    // where to write the pointer descriptor to!

        auto textureBlock = nthp::script::nthp_internal_alloc_special<nthp::texture::gTexture>(data, target_dsc, nthp::fixedToInt(refCache[0].value), nthp::script::BlockMemoryEntry::bmType::TEXTURE);
        if(textureBlock == nullptr) { return 1; }

	return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(TEXTURE_FREE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target

        EVAL_PTRREF(refCache[0]);
        const auto ptr_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);
        
        if((ptr_dsc.block) && (ptr_dsc.block < data->blockDataSize)) {
                nthp::texture::gTexture* textures = (nthp::texture::gTexture*)(data->blockData[ptr_dsc.block].data);

                for(size_t i = 0; i < data->blockData[ptr_dsc.block].sizeSpecial; ++i) { textures[i].clean(); }


                if((!data->blockData[ptr_dsc.block].isFree)) free(data->blockData[ptr_dsc.block].data);
                data->blockData[ptr_dsc.block].isFree = true;
                data->blockData[ptr_dsc.block].size = 0;

                return 0;
        }

        PRINT_DEBUG_ERROR("TEXTURE_CLEAR at [%zu] Attempted to free global list.\n", data->currentNode);
	return 1;
}


DEFINE_EXECUTION_BEHAVIOUR(TEXTURE_CLEAN) {
        refCache[0] = *(textureRef*)(data->nodeSet[data->currentNode].access.data);             // texture

        auto t = EVAL_TEXTUREREF(refCache[0]);  

        t->getTextureData().cleanSTData();
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(TEXTURE_LOAD) {
	refCache[0] = *(textureRef*)(data->nodeSet[data->currentNode].access.data);                             // texture
	refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                // file (str)

        auto t = EVAL_TEXTUREREF(refCache[0]);
        auto filename = EVAL_STRREF(refCache[1]);
	
        if(t->autoLoadTextureFile(filename, &nthp::script::activePalette, nthp::core.getRenderer())) {
                return 1;
        }

	return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SET_ACTIVE_PALETTE) {
        refCache[0] = *(strRef*)(data->nodeSet[data->currentNode].access.data);                                 // palette file

        auto filename = EVAL_STRREF(refCache[0]);

        nthp::script::activePalette.importPaletteFromFile(filename);

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(FRAME_ALLOC) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                 // size
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                // output
        
        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);
        
        auto frameBlock = nthp::script::nthp_internal_alloc_special<nthp::texture::Frame>(data, target_dsc, nthp::fixedToInt(refCache[0].value), nthp::script::BlockMemoryEntry::bmType::FRAME);
        if(frameBlock == nullptr) { return 1; }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(FRAME_FREE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target

        EVAL_PTRREF(refCache[0]);
        auto ptr = nthp::script::parsePtrDescriptor(refCache[0].value);

        if((ptr.block) && (ptr.block < data->blockDataSize)) {
                if((!data->blockData[ptr.block].isFree)) free(data->blockData[ptr.block].data);
                data->blockData[ptr.block].isFree = true;
                data->blockData[ptr.block].size = 0;

                return 0;
        }

        PRINT_DEBUG_ERROR("FRAME_CLEAR at [%zu] Attempted to free global list.\n", data->currentNode);
        return 1;
}

DEFINE_EXECUTION_BEHAVIOUR(FRAME_SET) {
        refCache[0] = *(frameRef*)(data->nodeSet[data->currentNode].access.data);                                       // frameindex
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                        // x
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 2));                  // y
        refCache[3] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 3));                  // w
        refCache[4] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 4));                  // h
        refCache[5] = *(textureRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 5));              // texture

        auto frame = EVAL_FRAMEREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);
        EVAL_STDREF(refCache[3]);
        EVAL_STDREF(refCache[4]);
        auto texture = EVAL_TEXTUREREF(refCache[5]);

        SDL_Rect rect;
        rect.x = nthp::fixedToInt(refCache[1].value);
        rect.y = nthp::fixedToInt(refCache[2].value);
        rect.w = nthp::fixedToInt(refCache[3].value);
        rect.h = nthp::fixedToInt(refCache[4].value);

        // Written on the 29th of July 2026; I completely forgot about this! How handy!
        if(!(rect.w * rect.h)) {
                rect.x = 0;
                rect.y = 0;
                rect.w = texture->getTextureData().metadata.x;
                rect.h = texture->getTextureData().metadata.y;
        }
        
        frame->src = rect;
        frame->texture = texture->getTextureData().getTexture();

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(ENT_ALLOC) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // size
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // target

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        auto entityBlock = nthp::script::nthp_internal_alloc_special<nthp::entity::gEntity>(data, target_dsc, nthp::fixedToInt(refCache[0].value), nthp::script::BlockMemoryEntry::bmType::ENTITY);
        if(entityBlock == nullptr) { return 1; }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ENT_FREE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target

        EVAL_PTRREF(refCache[0]);
        const auto ptr_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);
        
        if((ptr_dsc.block) && (ptr_dsc.block < data->blockDataSize)) {
                nthp::entity::gEntity* entities = (nthp::entity::gEntity*)(data->blockData[ptr_dsc.block].data);

                for(size_t i = 0; i < data->blockData[ptr_dsc.block].sizeSpecial; ++i) { entities[i].clean(); }

                if((!data->blockData[ptr_dsc.block].isFree)) free(data->blockData[ptr_dsc.block].data);
                data->blockData[ptr_dsc.block].isFree = true;
                data->blockData[ptr_dsc.block].size = 0;

                return 0;
        }

        PRINT_DEBUG_ERROR("ENT_CLEAR at [%zu] Attempted to free global list.\n", data->currentNode);
	return 1;
}


DEFINE_EXECUTION_BEHAVIOUR(ENT_SETCURRENTFRAME) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                 // target 
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));                // frame
        

        auto entity = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        entity->setCurrentFrame(nthp::fixedToInt(refCache[1].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ENT_SETPOS) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                         // entity
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));                        // x
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef) + sizeof(stdRef));       // y

        auto entity = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        entity->setPosition(nthp::vectFixed(refCache[1].value, refCache[2].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ENT_MOVE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                 // entity
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                   // x
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));  // y

        auto entity = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        entity->move(nthp::vectFixed(refCache[1].value, refCache[2].value));
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(ENT_SETFRAMERANGE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                         // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));                        // start
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef) + sizeof(frameRef));     // size

        auto entity = EVAL_ENTREF(refCache[0]);
        auto frameStart = EVAL_FRAMEREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        entity->importFrameData(frameStart, nthp::fixedToInt(refCache[2].value), false);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ENT_SETHITBOXSIZE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                         // entity
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));                        // w
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef) + sizeof(stdRef));       // h

        auto entity = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        entity->setHtiboxSize(nthp::vectFixed(refCache[1].value, refCache[2].value));
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(ENT_SETHITBOXOFFSET) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                         // entity
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));                           // x
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef) + sizeof(stdRef));          // y

        auto entity = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        entity->setHitboxOffset(nthp::vectFixed(refCache[1].value, refCache[2].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ENT_SETRENDERSIZE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                                       // entity
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                           // x
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));          // y

        auto entity = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        entity->setRenderSize(nthp::vectFixed(refCache[1].value, refCache[2].value));
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(ENT_CHECKCOLLISION) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                         // entA
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));                        // entB
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef) + sizeof(stdRef));     // output
        
        auto a = EVAL_ENTREF(refCache[0]);
        auto b = EVAL_ENTREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);

        *target_dsc = nthp::intToFixed(nthp::entity::checkRectCollision(a->getHitbox(), b->getHitbox()));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ENT_SETANGLE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);                         // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(entRef));        // angle

        auto e = EVAL_ENTREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        e->setRenderAngle(refCache[1].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_ALLOC) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // size
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // target

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        auto check = nthp::script::nthp_internal_alloc_special<nthp::entity::staticSetpiece>(data, target_dsc, nthp::fixedToInt(refCache[0].value), nthp::script::BlockMemoryEntry::bmType::SETPIECE);
        if(check == nullptr) {
                PRINT_DEBUG_ERROR("Failed to allocate staticSetpiece block.\n");
                return 1;
        }

        return 0;
}

// Setpieces have no dynamic internal data, just fixed values and pointers to external data.
DEFINE_EXECUTION_BEHAVIOUR(SP_FREE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target

        EVAL_PTRREF(refCache[0]);

        const auto ptr_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);
        if((ptr_dsc.block) && (ptr_dsc.block < data->blockDataSize)) {
                if(!(data->blockData[ptr_dsc.block].isFree)) free(data->blockData[ptr_dsc.block].data);
                data->blockData[ptr_dsc.block].isFree = true;
                data->blockData[ptr_dsc.block].data = nullptr;
                data->blockData[ptr_dsc.block].size = 0;
                data->blockData[ptr_dsc.block].type = nthp::script::BlockMemoryEntry::bmType::TYPELESS;
        }
        else {
                PRINT_DEBUG_ERROR("Unable to free staticSetpiece block [b%ua%u]; invalid target.\n", ptr_dsc.block, ptr_dsc.address);
                return 1;
        }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_SETRENDERSIZE) {
        refCache[0] = *(setpieceRef*)(data->nodeSet[data->currentNode].access.data);                             // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef));                      // w
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef) + sizeof(stdRef));     // h

        auto target_ptr = EVAL_SETPIECEREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        target_ptr->renderSize = nthp::vectFixed(refCache[1].value, refCache[2].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_SETFRAMERANGE) {
        refCache[0] = *(setpieceRef*)(data->nodeSet[data->currentNode].access.data);                                            // target
        refCache[1] = *(frameRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef));                         // frameSet
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef) + sizeof(frameRef));        // size

        auto target_ptr = EVAL_SETPIECEREF(refCache[0]);
        auto frame_ptr = EVAL_FRAMEREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        target_ptr->frames = frame_ptr;
        target_ptr->frameSize = nthp::fixedToInt(refCache[2].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_SETCURRENTFRAME) {
        refCache[0] = *(setpieceRef*)(data->nodeSet[data->currentNode].access.data);                            // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef));           // frame

        auto target_ptr = EVAL_SETPIECEREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        target_ptr->currentFrame = nthp::fixedToInt(refCache[1].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_SETPOS) {
        refCache[0] = *(setpieceRef*)(data->nodeSet[data->currentNode].access.data);                                    // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef));                      // x
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef) + sizeof(stdRef));     // y

        auto target_ptr = EVAL_SETPIECEREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);

        target_ptr->position = nthp::worldPosition(refCache[1].value, refCache[2].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_SETANGLE) {
        refCache[0] = *(setpieceRef*)(data->nodeSet[data->currentNode].access.data);                     // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(setpieceRef));          // angle

        auto t = EVAL_SETPIECEREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        t->angle = nthp::fixedToDouble(refCache[1].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_COMPILE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);               // target
        
        EVAL_PTRREF(refCache[0]);
        auto block_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);
        auto blockData = (nthp::entity::staticSetpiece*)(data->blockData[block_dsc.block].data);

        for(size_t i = 0; i < data->blockData[block_dsc.block].sizeSpecial; ++i) {
                nthp::entity::compileSetpiece(blockData + i, &nthp::core.p_coreDisplay);
        }
        
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SP_ABS_COMPILE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);               // target
        
        EVAL_PTRREF(refCache[0]);
        auto block_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);
        auto blockData = (nthp::entity::staticSetpiece*)(data->blockData[block_dsc.block].data);

        for(size_t i = 0; i < data->blockData[block_dsc.block].sizeSpecial; ++i) {
                nthp::entity::compileSetpiece_abs(blockData + i, &nthp::core.p_coreDisplay);
        }
        
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_INIT) {
        if(nthp::core.getInitSuccess())
                nthp::core.cleanup();

        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                 // px
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 1));          // py
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 2));          // tx
        refCache[3] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 3));          // ty
        refCache[4] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 4));          // cx
        refCache[5] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 5));          // cy
        refCache[6] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 6));          // fs
        refCache[7] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 7));          // sr
        refCache[8] = *(strRef*)(data->nodeSet[data->currentNode].access.data + (sizeof(stdRef) * 8));          // title

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);
        EVAL_STDREF(refCache[3]);
        EVAL_STDREF(refCache[4]);
        EVAL_STDREF(refCache[5]);
        EVAL_STDREF(refCache[6]);
        EVAL_STDREF(refCache[7]);
        auto titleString = EVAL_STRREF(refCache[8]);


        nthp::core.init(nthp::RenderRuleSet(nthp::fixedToInt(refCache[0].value), nthp::fixedToInt(refCache[1].value), refCache[2].value, refCache[3].value, nthp::vectFixed(refCache[4].value, refCache[5].value)), titleString, nthp::fixedToInt(refCache[6].value) & 1, nthp::fixedToInt(refCache[7].value) & 1);


        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(CORE_QRENDER) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);         // target

        auto entity = EVAL_ENTREF(refCache[0]);

        if(nthp::core.render(entity->getUpdateRenderPacket(&nthp::core.p_coreDisplay)) < 0) {
                PRINT_DEBUG_ERROR("%s; invalid render call.\n", SDL_GetError());
        }
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(CORE_ABS_QRENDER) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);       // target

        auto entity = EVAL_ENTREF(refCache[0]);

        if(nthp::core.render(entity->abs_getRenderPacket(&nthp::core.p_coreDisplay)) < 0) {
                PRINT_DEBUG_ERROR("%s; invalid render call.\n", SDL_GetError());
        }
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_SP_QRENDER) {
        refCache[0] = *(setpieceRef*)(data->nodeSet[data->currentNode].access.data);

        auto target = EVAL_SETPIECEREF(refCache[0]);

        nthp::core.render(target->compiledPacket);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_SP_QRENDER_BLOCK) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);         // target

        EVAL_PTRREF(refCache[0]);
        auto block_dsc = nthp::script::parsePtrDescriptor(refCache[0].value);

        const auto block = (nthp::entity::staticSetpiece*)data->blockData[block_dsc.block].data;
        const auto length = data->blockData[block_dsc.block].sizeSpecial;

        for(size_t i = 0; i < length; ++i) {
                nthp::core.render(block[i].compiledPacket); 
        }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_CLEAR) {
        nthp::core.clear();
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_DISPLAY) {
        nthp::core.display();
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_SETMAXFPS) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);         // fps

        EVAL_STDREF(refCache[0]);

        nthp::setMaxFPS(nthp::fixedToInt(refCache[0].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_SETWINDOWRES) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                    // x
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));   // y

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        nthp::core.setWindowRenderSize(nthp::fixedToInt(refCache[0].value), nthp::fixedToInt(refCache[1].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_SETCAMERARES) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        nthp::core.setVirtualRenderScale(refCache[0].value, refCache[1].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_SETCAMERAPOSITION) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        nthp::core.p_coreDisplay.cameraWorldPosition.x = refCache[0].value;
        nthp::core.p_coreDisplay.cameraWorldPosition.y = refCache[1].value;
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_MOVECAMERA) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        nthp::core.p_coreDisplay.cameraWorldPosition += nthp::worldPosition(refCache[0].value, refCache[1].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_STOP) {
        nthp::core.stop();
        data->isSuspended = true;
        PRINT_DEBUG("Core SHUTDOWN call with CORE_STOP...\n");

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_GETMOUSEPOSITION) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                           // x
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));          // y
        nthp::script::stdVarWidth* xOutput;
        {
                EVAL_PTRREF(refCache[0]);
                xOutput = target_dsc;
        }

        nthp::script::stdVarWidth* yOutput;
        {
                EVAL_PTRREF(refCache[1]);
                yOutput = target_dsc;
        }
        *xOutput = nthp::mousePosition.x;
        *yOutput = nthp::mousePosition.y;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(CORE_ABS_GETMOUSEPOSITION) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));

        nthp::script::stdVarWidth* xOutput;
        {
                EVAL_PTRREF(refCache[0]);
                xOutput = target_dsc;
        }

        nthp::script::stdVarWidth* yOutput;
        {
                EVAL_PTRREF(refCache[1]);
                yOutput = target_dsc;
        }
        int x,y;
        SDL_GetMouseState(&x, &y);
        
        *xOutput = nthp::intToFixed(x);
        *yOutput = nthp::intToFixed(y);

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(ACTION_DEFINE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        data->actionList = new nthp::script::Script::Action[nthp::fixedToInt(refCache[0].value)];
        data->actionListSize = nthp::fixedToInt(refCache[0].value);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ACTION_BIND) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // target
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // var
        int32_t key = *(int32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(ptrRef));

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        data->actionList[nthp::fixedToInt(refCache[0].value)].varLocation = target_dsc;  
        data->actionList[nthp::fixedToInt(refCache[0].value)].boundKey = key;

#ifdef PM
        GENERIC_PRINT("bound ACTION [%d] key index [%d] to ptrRef [b%ua%u]\n", nthp::fixedToInt(refCache[0].value), key, nthp::script::parsePtrDescriptor(refCache[1].value).block, nthp::script::parsePtrDescriptor(refCache[1].value).address);
#endif
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ACTION_CLEAR) {
        if(data->actionListSize > 0) delete[] data->actionList;

        data->actionListSize = 0;
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(POLL_ENT_POSITION) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);
        auto entity = EVAL_ENTREF(refCache[0]);

        const auto pos = entity->getPosition();

        data->blockData[0].data[nthp::script::predefined_globals::RPOLL1_GLOBAL_INDEX] = pos.x;
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL2_GLOBAL_INDEX] = pos.y;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(POLL_ENT_CURRENTFRAME) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);
        auto entity = EVAL_ENTREF(refCache[0]);

        const auto cf = entity->getCurrentFrameIndex();
        
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL1_GLOBAL_INDEX] = nthp::intToFixed(cf);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(POLL_ENT_HITBOX) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);
        auto entity = EVAL_ENTREF(refCache[0]);

        const auto box = entity->getHitbox();

        data->blockData[0].data[nthp::script::predefined_globals::RPOLL1_GLOBAL_INDEX] = box.x;
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL2_GLOBAL_INDEX] = box.y;
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL3_GLOBAL_INDEX] = box.w;
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL4_GLOBAL_INDEX] = box.h;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(POLL_ENT_RENDERSIZE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);
        auto entity = EVAL_ENTREF(refCache[0]);
        

        const auto rs = entity->getRenderSize();

        data->blockData[0].data[nthp::script::predefined_globals::RPOLL1_GLOBAL_INDEX] = rs.x;
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL2_GLOBAL_INDEX] = rs.y;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(POLL_ENT_ANGLE) {
        refCache[0] = *(entRef*)(data->nodeSet[data->currentNode].access.data);
        auto entity = EVAL_ENTREF(refCache[0]);

        const auto a = entity->getRenderAngle();
        data->blockData[0].data[nthp::script::predefined_globals::RPOLL1_GLOBAL_INDEX] = a;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DRAW_SETCOLOR) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        data->penColor = (decltype(data->penColor))nthp::fixedToInt(refCache[0].value);


        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DRAW_LINE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                                                         // x1
        refCache[1]= *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));                                         // y1
        refCache[2] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));                       // x2
        refCache[3] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef));      // y2

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_STDREF(refCache[2]);
        EVAL_STDREF(refCache[3]);

        const nthp::vectGeneric pointA = nthp::generatePixelPosition(nthp::worldPosition(refCache[0].value, refCache[1].value), &nthp::core.p_coreDisplay) + nthp::generatePixelPosition(nthp::core.p_coreDisplay.cameraWorldPosition, &nthp::core.p_coreDisplay);
        const nthp::vectGeneric pointB = nthp::generatePixelPosition(nthp::worldPosition(refCache[2].value, refCache[3].value), &nthp::core.p_coreDisplay) + nthp::generatePixelPosition(nthp::core.p_coreDisplay.cameraWorldPosition, &nthp::core.p_coreDisplay);

        SDL_SetRenderDrawColor(nthp::core.getRenderer(), nthp::script::activePalette.colorSet[data->penColor].R,nthp::script::activePalette.colorSet[data->penColor].G, nthp::script::activePalette.colorSet[data->penColor].B, 255);
        
        SDL_RenderDrawLine(nthp::core.getRenderer(), pointA.x, pointA.y, pointB.x, pointB.y);

        SDL_SetRenderDrawColor(nthp::core.getRenderer(), DEFAULT_RENDER_COLOR);

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(AUDIOCHANNEL_DEFINE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        // Set the new channel count, then poll for the allocated channel count, in case
        // something goes wrong.
        Mix_AllocateChannels(nthp::fixedToInt(refCache[0].value));
        nthp::core.audioSystem.channelCount = Mix_AllocateChannels(-1);

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(SOUND_DEFINE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        nthp::core.audioSystem.soundEffects = new (std::nothrow) nthp::audio::SoundChannel[nthp::fixedToInt(refCache[0].value)];
        if(nthp::core.audioSystem.soundEffects == nullptr) {
                PRINT_DEBUG_ERROR("SOUND_DEFINE call @ [%zu] failed to allocate sound data.\n", data->currentNode);
                nthp::core.audioSystem.soundSize = 0;
        }
        else {
                nthp::core.audioSystem.soundSize = nthp::fixedToInt(refCache[0].value);
        }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SOUND_CLEAR) {
        if(nthp::core.audioSystem.soundSize > 0)
                delete[] nthp::core.audioSystem.soundEffects;

        nthp::core.audioSystem.soundSize = 0;


        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_DEFINE) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        nthp::core.audioSystem.music = new (std::nothrow) nthp::audio::MusicChannel[nthp::fixedToInt(refCache[0].value)];
        if(nthp::core.audioSystem.music == nullptr) {
                PRINT_DEBUG_ERROR("MUSIC_DEFINE call failed to allocate sound data. [%d] is not valid.\n", nthp::fixedToInt(refCache[0].value));
                nthp::core.audioSystem.musicSize = 0;
        }
        else
                nthp::core.audioSystem.musicSize = nthp::fixedToInt(refCache[0].value);


        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_CLEAR) {
        Mix_HaltMusic();

        if(nthp::core.audioSystem.musicSize > 0)
                delete[] nthp::core.audioSystem.music;

        nthp::core.audioSystem.musicSize = 0;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_LOAD) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                          // object
        refCache[1] = *(strRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));            // filename

        EVAL_STDREF(refCache[0]);
        auto fileString = EVAL_STRREF(refCache[1]);

        int ret = nthp::core.audioSystem.music[nthp::fixedToInt(refCache[0].value)].load(fileString);
        if(ret) { PRINT_DEBUG_ERROR("Failed to load track [%s] into MUSIC_ID %d.\n", fileString, nthp::fixedToInt(refCache[0].value)); }
        else PRINT_DEBUG("Loaded music track [%s] into MUSIC_ID %d.\n", fileString, nthp::fixedToInt(refCache[0].value));
        
        return ret;
}

DEFINE_EXECUTION_BEHAVIOUR(SOUND_LOAD) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);
        refCache[1] = *(strRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));

        EVAL_STDREF(refCache[0]);
        auto fileString = EVAL_STRREF(refCache[1]);

        int ret = nthp::core.audioSystem.soundEffects[nthp::fixedToInt(refCache[0].value)].load(fileString);
        return ret;
}

DEFINE_EXECUTION_BEHAVIOUR(SOUND_PLAY) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        nthp::core.audioSystem.soundEffects[nthp::fixedToInt(refCache[0].value)].playSound();
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SOUND_SETCHANNEL) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                      // soundID
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));     // channel

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        
        nthp::core.audioSystem.soundEffects[nthp::fixedToInt(refCache[0].value)].channel = nthp::fixedToInt(refCache[1].value);
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(SOUND_STOP) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        nthp::core.audioSystem.soundEffects[nthp::fixedToInt(refCache[0].value)].stopSound();
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_START) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        data->currentMusicTrack = nthp::fixedToInt(refCache[0].value);
        nthp::core.audioSystem.music[nthp::fixedToInt(refCache[0].value)].start();

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_STOP) {
        Mix_HaltMusic();
        data->currentMusicTrack = -1;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_PAUSE) {
        if(data->currentMusicTrack > -1)
                nthp::core.audioSystem.music[data->currentMusicTrack].pauseMusic();

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_RESUME) {
        if(data->currentMusicTrack > -1)
                nthp::core.audioSystem.music[data->currentMusicTrack].resumeMusic();

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(MUSIC_SETVOLUME) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        nthp::core.setMusicVolume(nthp::fixedToInt(refCache[0].value));
        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(SOUND_SETVOLUME) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                       // target
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));      // volume

        EVAL_STDREF(refCache[0]);
        EVAL_STDREF(refCache[1]);

        nthp::core.setSoundVolume(nthp::fixedToInt(refCache[0].value), nthp::fixedToInt(refCache[1].value));
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DFILE_READ) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                       // target
        refCache[1] = *(strRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));    // filename

        EVAL_PTRREF(refCache[0]);
        auto fileString = EVAL_STRREF(refCache[1]);

        std::fstream file;
        file.open(fileString, std::ios::in | std::ios::binary);
        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to open file [%s]; File inaccessible.\n", fileString);
                data->blockData[0].data[nthp::script::predefined_globals::NTHP_NULL] = 1;
                return 0;
        }
        nthp::script::stdVarWidth fileSize = 0;
        file.read((char*)&fileSize, sizeof(nthp::script::stdVarWidth));

        size_t byteSize = nthp::fixedToInt(fileSize) * sizeof(nthp::script::stdVarWidth);

        nthp::script::nthp_internal_alloc(data, target_dsc, fileSize, 0, nthp::script::BlockMemoryEntry::bmType::TYPELESS);
        auto ptr = nthp::script::parsePtrDescriptor(*target_dsc);
        if(ptr.block) {
                file.read((char*)data->blockData[ptr.block].data, fileSize);
                file.close();

                return 0;
        }
        else {
                PRINT_DEBUG_ERROR("Cannot use DFILE_READ to input external data into global list.\n");
                data->blockData[0].data[nthp::script::predefined_globals::NTHP_NULL] = 1;
                return 0;
        }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DFILE_WRITE) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                       // target
        refCache[1] = *(strRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));    // filename


        EVAL_PTRREF(refCache[0]); // EVAL_PTRREF does NOT change the evaluated ptr_descriptor in 'target.value'; it just creates target_dsc after evaluating it.
        const auto ptr = nthp::script::parsePtrDescriptor(refCache[0].value);


        auto fileString = EVAL_STRREF(refCache[1]);


        std::fstream file;
        file.open(fileString, std::ios::out | std::ios::binary);
        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to open file [%s] for writing; File not accessible. \n", fileString);
                data->blockData[0].data[nthp::script::predefined_globals::NTHP_NULL] = 1;
                return 0;
        }


        nthp::script::stdVarWidth size = nthp::intToFixed(data->blockData[ptr.block].size);
        file.write((char*)&size, sizeof(nthp::script::stdVarWidth));

        size_t byteSize = data->blockData[ptr.block].size * sizeof(nthp::script::stdVarWidth);
        file.write((char*)target_dsc, byteSize);

        file.close();

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(PRINT_REF) {
#ifdef DEBUG
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_STDREF(refCache[0]);

        GENERIC_PRINT("[t %u] %lf\n", SDL_GetTicks(), nthp::fixedToDouble(refCache[0].value));
#endif

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(PRINT_STRING) {
#ifdef DEBUG
        refCache[0] = *(strRef*)(data->nodeSet[data->currentNode].access.data);

        auto message = EVAL_STRREF(refCache[0]);

        GENERIC_PRINT("[t %u] %s\n", SDL_GetTicks(), message);
#endif
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(STRING) {

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(STRING_COPY) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);                       // target
        refCache[1] = *(strRef*)(data->nodeSet[data->currentNode].access.data + sizeof(ptrRef));    // c_string

        EVAL_PTRREF(refCache[0]);
        auto str = EVAL_STRREF(refCache[1]);

        // Because the string length is stored in the offset of the reference.
        if(PR_METADATA_GET(refCache[1], nthp::script::flagBits::IS_NODE_STRING_PTR)) {
                memcpy(target_dsc, str, refCache[1].offset);
        }
        else {
                int i = 0; for(; str[i] != '\0'; ++i);
                memcpy(target_dsc, str, i);
        }
        
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(STRING_GETCHAR) {
        refCache[0] = *(strRef*)(data->nodeSet[data->currentNode].access.data);                                       // string
        refCache[1] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(strRef));                       // index
        refCache[2] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(strRef) + sizeof(stdRef));     // output

        auto str = EVAL_STRREF(refCache[0]);
        EVAL_STDREF(refCache[1]);
        EVAL_PTRREF(refCache[2]);


        (*target_dsc) = nthp::intToFixed(str[nthp::fixedToInt(refCache[1].value)]);

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(STRING_TO_NUM) {
        refCache[0] = *(strRef*)(data->nodeSet[data->currentNode].access.data);                       // string
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(strRef));      // output

        auto str = EVAL_STRREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        try {
                (*target_dsc) = nthp::doubleToFixed(std::stod(str));
        }
        catch(std::invalid_argument) {
                PRINT_DEBUG_ERROR("Invalid string conversion to fixed point @ [%zu].\n", data->currentNode);
                (*target_dsc) = 0;
                data->blockData[0].data[nthp::script::predefined_globals::NTHP_NULL] = 1;
        }

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(NUM_TO_STRING) {
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data);                         // value
        refCache[1] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data + sizeof(stdRef));        // output

        EVAL_STDREF(refCache[0]);
        EVAL_PTRREF(refCache[1]);

        // 7/29/2026; slightly less shit.
        std::string temp = std::to_string(nthp::fixedToDouble(refCache[1].value));

        const auto ptr = nthp::script::nthp_internal_alloc(data, target_dsc, (nthp::intToFixed(temp.size() / sizeof(nthp::script::stdVarWidth) + 1)), 0, nthp::script::BlockMemoryEntry::bmType::TYPELESS);
        if(ptr.block == 0) { return 1; }
        
        memcpy(data->blockData[ptr.block].data, temp.c_str(), temp.size());

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(IB_SET_TARGET) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_PTRREF(refCache[0]);

        data->ibTargetOrigin = (char*)target_dsc;
        data->ibTargetSet = true;
        data->ibTargetPosition = 0;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(IB_WRITE_STRING) {
        if(!data->ibTargetSet) {
                PRINT_DEBUG_ERROR("IB_WRITE_STRING failed; target not set.\n");
                return 1;
        }

        for(size_t i = 0; (data->inputBuffer[i]); ++i) {
                data->ibTargetOrigin[data->ibTargetPosition] = data->inputBuffer[i] & 255;
                ++(data->ibTargetPosition);
        }

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(IB_STOP) {
        data->ibTargetOrigin[data->ibTargetPosition] = '\0';

        data->ibTargetOrigin = NULL;
        data->ibTargetSet = false;
        data->ibTargetPosition = 0;

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(TEXTINPUT_START) {
        refCache[0] = *(ptrRef*)(data->nodeSet[data->currentNode].access.data);

        EVAL_PTRREF(refCache[0]);

        data->textInputActive = true;
        data->textInputTarget = (char*)target_dsc;
        data->textInputLocation = nthp::script::parsePtrDescriptor(refCache[0].value);

        nthp::core.startTextInput();

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(TEXTINPUT_STOP) {
        if(!data->textInputActive) { return 0; }

        data->textInputTarget[data->textInputBufferPosition] = '\0';

        data->textInputActive = false;
        data->textInputBufferPosition = 0;
        data->textInputTarget = nullptr;
        data->textInputLocation = nthp::script::NULL_REF;

        nthp::core.stopTextInput();

        return 0;
}


DEFINE_EXECUTION_BEHAVIOUR(FUNC_START) {
        const uint32_t headerLocation = *(uint32_t*)(data->nodeSet[data->currentNode].access.data + sizeof(uint32_t));
        data->currentScriptHeaderLocation = headerLocation;


        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(FUNC_LIST) {
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(FUNC_CALL) {
        const uint32_t location = *(uint32_t*)(data->nodeSet[data->currentNode].access.data);
        const nthp::script::Script::ReturnStackEntry newEntry = { data->currentScriptHeaderLocation, (uint32_t)(data->currentNode + 1) };

        data->returnStack[data->stackPointer] = newEntry;
        ++(data->stackPointer);

        data->currentNode = location; // No -1 here; that is evaluated in the linker!

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(FUNC_LIST_CALL) {
        uint32_t listLocation = *(uint32_t*)(data->nodeSet[data->currentNode].access.data);
        refCache[0] = *(stdRef*)(data->nodeSet[data->currentNode].access.data + sizeof(uint32_t));

        EVAL_STDREF(refCache[0]);

        const nthp::script::Script::ReturnStackEntry newEntry = { data->currentScriptHeaderLocation, (uint32_t)(data->currentNode + 1) };
        data->returnStack[data->stackPointer] = newEntry;
        ++(data->stackPointer);

        data->currentNode = ((uint32_t*)(data->nodeSet[listLocation].access.data))[nthp::fixedToInt(refCache[0].value)];        
        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(DEBUG_BREAK) {
#ifdef DEBUG
std::mutex access;

        access.lock();

        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::DEBUG_CALLS::BREAK;
        printf("Breakpoint read at instruction [%zu]; HEAD at [%zu], waiting for continue.\n", data->currentNode, data->currentNode);

        access.unlock();

#endif

        return 0;
}

DEFINE_EXECUTION_BEHAVIOUR(ERROR_CLEAR) {
        data->blockData[0].data[nthp::script::predefined_globals::NTHP_NULL] = 0;

        return 0;
}

// Instruction execution behaviour functions. Use DEFINE_EXECUTION_BEHAVIOUR followed by a matching token from INSTRUCTION_TOKENS (s_instructions.hpp)
// to define what happens for every instruction. Due to using INSTRUCTION_TOKENS as the initializer (see below), the ID of an intruction will match
// that instruction's execution behaviour function in this array.
// Automatically updates ID indecies and places functions accordingly. Just add/change stuff in 's_instructions.hpp'.
// the 'nthp::script::instructions::ID' will correspond with the index of the desired instruction in this array.
static const int (*exec_func[nthp::script::instructions::ID::numberOfInstructions])(nthp::script::Script::ScriptDataSet* data) { INSTRUCTION_TOKENS() };






/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


nthp::script::Script::Script() {
        data = NULL;
        inStageContext = false;
        localCurrentNode = 0;
        localLabelBlock = nullptr;
        localLabelBlockSize = 0;
}

nthp::script::Script::Script(ScriptDataSet* dataSet, uint32_t headerLocation) {
        localCurrentNode = 0;
        localLabelBlock = nullptr;
        localLabelBlockSize = 0;


        import(dataSet, headerLocation);
}


int nthp::script::Script::import(ScriptDataSet* const dataSet, const uint32_t headerLocation) {
        if(dataSet == NULL) {
                data = new nthp::script::Script::ScriptDataSet;
                memset(data, 0, sizeof(nthp::script::Script::ScriptDataSet));
                inStageContext = false;
        }
        else {
                inStageContext = true;
        }


        data = dataSet;
        script_begin = headerLocation;
        if(!(data->nodeSet[headerLocation].access.ID == GET_INSTRUCTION_ID(HEADER))) {
                PRINT_DEBUG_ERROR("Provided script origin does not correspond to valid header node.\n");
                return 1;
        }
        
        // Note this is a pointer to the region that stores labels.
        localLabelBlock = (uint32_t*)(data->nodeSet[headerLocation].access.data + (sizeof(uint32_t) + sizeof(uint32_t)));
        localLabelBlockSize = *(uint32_t*)(data->nodeSet[headerLocation].access.data + (sizeof(uint32_t)));
        localCurrentNode = headerLocation;


        return 0;
}




int nthp::script::Script::execute() {

        #ifdef DEBUG
                std::mutex debug_access;
                debug_access.lock();

                if(nthp::script::debug::suspendExecution) { 
                        data->isSuspended = true;
                }
                else { 
                        data->isSuspended = false;
                        data->currentNode = localCurrentNode;
                        

                        if(data->stackPointer == 0) {
                                data->currentScriptHeaderLocation = script_begin;
                                data->currentLabelBlock = localLabelBlock;
                                data->currentLabelBlockSize = localLabelBlockSize;
                        }
                }
                
        #else
                data->isSuspended = false;

                data->currentLabelBlock = localLabelBlock;
                data->currentLabelBlockSize = localLabelBlockSize;
                data->currentNode = localCurrentNode;
                data->currentScriptHeaderLocation = script_begin;
        #endif

        

        #ifdef DEBUG
                switch(nthp::script::debug::debugInstructionCall.x) {
                        case nthp::script::debug::CONTINUE: {
                                nthp::script::debug::suspendExecution = false;
                                nthp::script::debug::debugInstructionCall.x = -1;
                                goto SKIP_EXECUTION;
                        }
                                break;
                        // Executes next instruction, then breaks.
                        case nthp::script::debug::STEP: {
                                nthp::script::debug::suspendExecution = false;
                                data->isSuspended = false;
                                
                                break;
                        }
                        case(nthp::script::debug::JUMP_TO): {
                                data->currentNode = nthp::script::debug::debugInstructionCall.y;
                                localCurrentNode = nthp::script::debug::debugInstructionCall.y;
                                data->currentScriptHeaderLocation = nthp::script::Script::findInstructionHeader(data->nodeSet, nthp::script::debug::debugInstructionCall.y);
                        }
                                break;

                        default:
                                break;
                }
        debug_access.unlock();

        #endif

        while((data->currentNode < data->nodeSetSize) && 
                (data->isSuspended == false)) {
#ifdef DEBUG
                        debug_access.lock();
                        switch(nthp::script::debug::debugInstructionCall.x) {

                                case(nthp::script::debug::BREAK):
                                        nthp::script::debug::suspendExecution = true;
                                        nthp::script::debug::debugInstructionCall.x = -1;
                                        goto SKIP_EXECUTION;
                                        break;

                                case(nthp::script::debug::CONTINUE):
                                        nthp::script::debug::suspendExecution = false;
                                        nthp::script::debug::debugInstructionCall.x = -1;
                                        break;
                                
                                case(nthp::script::debug::STEP):
                                        // Executes the next instruction, and breaks the next tick.
                                        nthp::script::debug::suspendExecution = true;
                                        nthp::script::debug::debugInstructionCall.x = -1;
                                        data->isSuspended = true;
                                        break;
                                default:
                                        break;

                        }
                        debug_access.unlock();
#endif
                if(exec_func[data->nodeSet[data->currentNode].access.ID](data)) { 
                        PRINT_DEBUG_ERROR("Failure in script object [%p] @ inst=%zu.\n", this, data->currentNode);
                        return 1; 
                }
                ++data->currentNode;
        }

        if(data->nodeSet[data->currentNode - 1].access.ID == GET_INSTRUCTION_ID(EXIT)) { localCurrentNode = script_begin; }
        else localCurrentNode = data->currentNode;

#ifdef DEBUG
        SKIP_EXECUTION: // Using goto. Hail to the king, baby.
        debug_access.unlock();
#endif

        

        return 0;
}


int nthp::script::Script::execute(uint32_t entryPoint) {
        localCurrentNode = entryPoint;

        return execute();
}




nthp::script::Script::~Script() {

        // Make sure NOT to delete the localLabelBlock, as tempting as it seems.
        // it points to the HEADER node data, so it's freed with the rest of the
        // nodes. I write this because I made that mistake on Oct. 15, 2024.

        if(!inStageContext) {
                nthp::script::Script::cleanDataSet(data);
        }

}
