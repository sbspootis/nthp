#pragma once
#include "s_node.hpp"



namespace nthp { namespace script { 
        
        typedef enum {
                NTHP_NULL,
                DELTATIME_GLOBAL_INDEX,
                MOUSE1_GLOBAL_INDEX,
                MOUSE2_GLOBAL_INDEX,
                MOUSE3_GLOBAL_INDEX,
                RPOLL1_GLOBAL_INDEX,
                RPOLL2_GLOBAL_INDEX,
                RPOLL3_GLOBAL_INDEX,
                RPOLL4_GLOBAL_INDEX,
        
                COUNT_PREDEFINED_GLOBALS
        } predefined_globals;
        
        namespace instructions {


#define INSTRUCTION_LIST(...) typedef enum __inst {\
        __VA_ARGS__\
        } Instruction

#define INSTRUCTION_SIZE_LIST(...) constexpr decltype(nthp::script::Node::n_binary_t::size) __VA_ARGS__

#define ____INSTRUCTION_TOKENS(...) __VA_ARGS__

#define         DYNAMIC_SIZE    0
#define         NTHP_CORE_INIT_SOFTWARE_RENDERING    1
#define         NTHP_CORE_INIT_FULLSCREEN            0

namespace ID {
        #define INSTRUCTION_TOKENS() ____INSTRUCTION_TOKENS(\
                HEADER,\
                EXIT,\
                LABEL,\
                GOTO,\
                JUMP,\
                SUSPEND,\
                RETURN,\
                GETINDEX,\
                DATA,\
                INC,\
                DEC,\
                LSHIFT,\
                RSHIFT,\
                ADD,\
                SUB,\
                MUL,\
                DIV,\
                SQRT,\
                POW,\
                ABS,\
                MOD,\
                RAND,\
                SIN,\
                COS,\
                TAN,\
                ASIN,\
                ACOS,\
                ATAN,\
                LOGIC_IF_TRUE,\
                LOGIC_EQU,\
                LOGIC_NOT,\
                LOGIC_GRT,\
                LOGIC_LST,\
                LOGIC_GRTE,\
                LOGIC_LSTE,\
                END,\
                ELSE,\
                SKIP,\
                SKIP_END,\
                SET,\
                ALLOC,\
                NEW,\
                FREE,\
                COPY,\
                NEXT,\
                PREV,\
                INDEX,\
                LAST,\
                GET_BLOCKSIZE,\
                SET_BLOCKLISTSIZE,\
                ALLOC_TARGET,\
		TEXTURE_ALLOC,\
		TEXTURE_FREE,\
                TEXTURE_CLEAN,\
		TEXTURE_LOAD,\
		SET_ACTIVE_PALETTE,\
                FRAME_ALLOC,\
                FRAME_FREE,\
                FRAME_SET,\
                ENT_ALLOC,\
                ENT_FREE,\
                ENT_SETCURRENTFRAME,\
                ENT_SETPOS,\
                ENT_MOVE,\
                ENT_SETFRAMERANGE,\
                ENT_SETHITBOXSIZE,\
                ENT_SETHITBOXOFFSET,\
                ENT_SETRENDERSIZE,\
                ENT_CHECKCOLLISION,\
                ENT_SETANGLE,\
                SP_ALLOC,\
                SP_FREE,\
                SP_SETRENDERSIZE,\
                SP_SETFRAMERANGE,\
                SP_SETCURRENTFRAME,\
                SP_SETANGLE,\
                SP_SETPOS,\
                SP_COMPILE,\
                SP_ABS_COMPILE,\
                CORE_INIT,\
                CORE_QRENDER,\
                CORE_ABS_QRENDER,\
                CORE_SP_QRENDER,\
                CORE_SP_QRENDER_BLOCK,\
                CORE_CLEAR,\
                CORE_DISPLAY,\
                CORE_SETMAXFPS,\
                CORE_SETWINDOWRES,\
                CORE_SETCAMERARES,\
                CORE_SETCAMERAPOSITION,\
                CORE_MOVECAMERA,\
                CORE_STOP,\
                CORE_GETMOUSEPOSITION,\
                CORE_ABS_GETMOUSEPOSITION,\
                ACTION_DEFINE,\
                ACTION_BIND,\
                ACTION_CLEAR,\
                POLL_ENT_POSITION,\
                POLL_ENT_HITBOX,\
                POLL_ENT_RENDERSIZE,\
                POLL_ENT_CURRENTFRAME,\
                POLL_ENT_ANGLE,\
                DRAW_SETCOLOR,\
                DRAW_LINE,\
                AUDIOCHANNEL_DEFINE,\
                SOUND_DEFINE,\
                SOUND_CLEAR,\
                MUSIC_DEFINE,\
                MUSIC_CLEAR,\
                MUSIC_LOAD,\
                SOUND_LOAD,\
                SOUND_PLAY,\
                SOUND_SETCHANNEL,\
                SOUND_STOP,\
                MUSIC_START,\
                MUSIC_STOP,\
                MUSIC_PAUSE,\
                MUSIC_RESUME,\
                MUSIC_SETVOLUME,\
                SOUND_SETVOLUME,\
                DFILE_READ,\
                DFILE_WRITE,\
                PRINT_REF,\
                PRINT_STRING,\
                STRING,\
                STRING_COPY,\
                STRING_GETCHAR,\
                STRING_TO_NUM,\
                NUM_TO_STRING,\
                IB_SET_TARGET,\
                IB_WRITE_STRING,\
                IB_STOP,\
                TEXTINPUT_START,\
                TEXTINPUT_STOP,\
                FUNC_START,\
                FUNC_CALL,\
                FUNC_LIST,\
                FUNC_LIST_CALL,\
                DEBUG_BREAK,\
                ERROR_CLEAR\
        )

        INSTRUCTION_LIST( INSTRUCTION_TOKENS(), numberOfInstructions);
}

#define GET_INSTRUCTION_ID(instruction) nthp::script::instructions::ID::Instruction::instruction


typedef P_Reference<nthp::script::stdVarWidth> stdRef;  // The standard value type; Can be a reference to memory or a constant, 'metadata' bits can be set for type description. The endpoint should be a workable value.
typedef stdRef ptrRef;                                  // ptrRef endpoints are evaluated as ptr_descriptors; otherwise identical to stdRefs
typedef stdRef strRef;                                  // strRef endpoints are the same as stdRef, but instead point to a STRING node or assumed block data (ptr_descriptor) string.

// Special types with custom runtime evaluation go here VV 

typedef stdRef entRef;                                  // Uses eval_special on runtime to parse a gEntity object in a given block.
typedef stdRef textureRef;                              // Uses eval_special on runtime to parse a gTexture object in a given block.
typedef stdRef frameRef;                                // Uses eval_special on runtime to parse a Frame object in a given block.
typedef stdRef setpieceRef;                             // Uses eval_special on runtime to parse staticSetpiece object in a given block.


// Sizes must have the same name as the ENUM entry in 'ID'.
namespace Size {

        INSTRUCTION_SIZE_LIST(
                HEADER = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t),
                EXIT = 0,

                LABEL = sizeof(uint32_t),
                GOTO = sizeof(uint32_t),
                JUMP = sizeof(stdRef),
                SUSPEND = 0,
                RETURN = 0,
                GETINDEX = sizeof(ptrRef),

                DATA = DYNAMIC_SIZE,

                INC = sizeof(ptrRef),
                DEC = sizeof(ptrRef),
                RSHIFT = sizeof(ptrRef) + sizeof(stdRef),
                LSHIFT = sizeof(ptrRef) + sizeof(stdRef),

                ADD = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                SUB = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                MUL = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                DIV = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                SQRT = sizeof(stdRef) + sizeof(ptrRef),
                POW = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                RAND = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                ABS = sizeof(stdRef) + sizeof(ptrRef),
                MOD = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                SIN = sizeof(stdRef) + sizeof(ptrRef),
                COS = sizeof(stdRef) + sizeof(ptrRef),
                TAN = sizeof(stdRef) + sizeof(ptrRef),
                ASIN = sizeof(stdRef) + sizeof(ptrRef),
                ACOS = sizeof(stdRef) + sizeof(ptrRef),
                ATAN = sizeof(stdRef) + sizeof(ptrRef),

                LOGIC_IF_TRUE = sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t),
                LOGIC_EQU = sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t), // OpA OpB EndLocation ElseLocation
                LOGIC_NOT = sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t),
                LOGIC_GRT = sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t),
                LOGIC_LST = sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t),
                LOGIC_GRTE = sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t),
                LOGIC_LSTE = sizeof(stdRef) + sizeof(stdRef) + sizeof(uint32_t) + sizeof(uint32_t),

                END = 0,
                ELSE = sizeof(uint32_t), // endLocation
                SKIP = sizeof(uint32_t),
                SKIP_END = 0,

                SET = sizeof(ptrRef) + sizeof(stdRef),
                ALLOC = sizeof(stdRef) + sizeof(ptrRef),
                NEW = sizeof(stdRef) + sizeof(ptrRef) + sizeof(uint32_t),
                COPY = sizeof(ptrRef) + sizeof(stdRef) + sizeof(ptrRef),
                FREE = sizeof(ptrRef),
                NEXT = sizeof(ptrRef) + sizeof(uint8_t),
                PREV = sizeof(ptrRef) + sizeof(uint8_t),
                INDEX = sizeof(ptrRef) + sizeof(stdRef) + sizeof(uint8_t),
                LAST = sizeof(ptrRef) + sizeof(uint8_t),
                GET_BLOCKSIZE = sizeof(stdRef) + sizeof(ptrRef),
                SET_BLOCKLISTSIZE = sizeof(stdRef),
                ALLOC_TARGET = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),

		TEXTURE_ALLOC = sizeof(stdRef) + sizeof(ptrRef),
		TEXTURE_FREE = sizeof(ptrRef),
                TEXTURE_CLEAN = sizeof(textureRef),
		TEXTURE_LOAD = sizeof(textureRef) + sizeof(strRef), 
		SET_ACTIVE_PALETTE = sizeof(strRef), 


                FRAME_ALLOC = sizeof(stdRef) + sizeof(ptrRef),
                FRAME_FREE = sizeof(ptrRef),
                FRAME_SET = sizeof(frameRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(textureRef),

                ENT_ALLOC = sizeof(stdRef) + sizeof(ptrRef),
                ENT_FREE = sizeof(ptrRef),

                ENT_SETCURRENTFRAME = sizeof(entRef) + sizeof(stdRef),
                ENT_SETPOS = sizeof(entRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_MOVE = sizeof(entRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETFRAMERANGE = sizeof(entRef) + sizeof(frameRef) + sizeof(stdRef),
                ENT_SETHITBOXSIZE = sizeof(entRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETHITBOXOFFSET = sizeof(entRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETRENDERSIZE = sizeof(entRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_CHECKCOLLISION = sizeof(entRef) + sizeof(stdRef) + sizeof(entRef),
                ENT_SETANGLE = sizeof(entRef) + sizeof(stdRef),

                SP_ALLOC = sizeof(stdRef) + sizeof(ptrRef),
                SP_FREE = sizeof(ptrRef),
                SP_SETRENDERSIZE = sizeof(setpieceRef) + sizeof(stdRef) + sizeof(stdRef),
                SP_SETFRAMERANGE = sizeof(setpieceRef) + sizeof(frameRef) + sizeof(stdRef),
                SP_SETCURRENTFRAME = sizeof(setpieceRef) + sizeof(stdRef),
                SP_SETANGLE = sizeof(setpieceRef) + sizeof(stdRef),
                SP_SETPOS = sizeof(setpieceRef) + sizeof(stdRef) + sizeof(stdRef),
                SP_COMPILE = sizeof(ptrRef),
                SP_ABS_COMPILE = sizeof(ptrRef),

                CORE_INIT = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(strRef),
                CORE_QRENDER = sizeof(entRef),
                CORE_ABS_QRENDER = sizeof(entRef),
                CORE_SP_QRENDER = sizeof(setpieceRef),
                CORE_SP_QRENDER_BLOCK = sizeof(ptrRef),
                CORE_CLEAR = 0,
                CORE_DISPLAY = 0,
                CORE_SETMAXFPS = sizeof(stdRef),
                CORE_SETWINDOWRES = sizeof(stdRef) + sizeof(stdRef),
                CORE_SETCAMERARES = sizeof(stdRef) + sizeof(stdRef),
                CORE_SETCAMERAPOSITION = sizeof(stdRef) + sizeof(stdRef),
                CORE_MOVECAMERA = sizeof(stdRef) + sizeof(stdRef),
                CORE_GETMOUSEPOSITION = sizeof(ptrRef) + sizeof(ptrRef),
                CORE_ABS_GETMOUSEPOSITION = sizeof(ptrRef) + sizeof(ptrRef),
                CORE_STOP = 0,

                ACTION_DEFINE = sizeof(stdRef),
                ACTION_CLEAR = 0, //
                ACTION_BIND =  sizeof(stdRef) + sizeof(ptrRef) + sizeof(int32_t), // actionIndex, varIndex, key

                POLL_ENT_POSITION = sizeof(entRef),
                POLL_ENT_HITBOX = sizeof(entRef),
                POLL_ENT_RENDERSIZE = sizeof(entRef),
                POLL_ENT_CURRENTFRAME = sizeof(entRef),
                POLL_ENT_ANGLE = sizeof(entRef),

                DRAW_SETCOLOR = sizeof(stdRef),
                DRAW_LINE = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                
                AUDIOCHANNEL_DEFINE = sizeof(stdRef),
                SOUND_DEFINE = sizeof(stdRef),
                SOUND_CLEAR = 0,
                MUSIC_DEFINE = sizeof(stdRef),
                MUSIC_CLEAR = 0,
                MUSIC_LOAD = sizeof(stdRef) + sizeof(strRef),
                SOUND_LOAD = sizeof(stdRef) + sizeof(strRef),
                SOUND_PLAY = sizeof(stdRef),
                SOUND_SETCHANNEL = sizeof(stdRef) + sizeof(stdRef),
                SOUND_STOP = sizeof(stdRef),
                MUSIC_START = sizeof(stdRef),
                MUSIC_STOP = 0,
                MUSIC_PAUSE = 0,
                MUSIC_RESUME = 0,
                MUSIC_SETVOLUME = sizeof(stdRef),
                SOUND_SETVOLUME = sizeof(stdRef) + sizeof(stdRef),

                DFILE_READ = sizeof(ptrRef) + sizeof(strRef),
                DFILE_WRITE = sizeof(ptrRef) + sizeof(strRef),

                PRINT_REF = sizeof(stdRef),
                PRINT_STRING = sizeof(strRef),
                STRING = DYNAMIC_SIZE,
                STRING_COPY = sizeof(ptrRef) + sizeof(strRef),
                STRING_GETCHAR = sizeof(strRef) + sizeof(stdRef) + sizeof(ptrRef),
                STRING_TO_NUM = sizeof(strRef) + sizeof(ptrRef),
                NUM_TO_STRING = sizeof(stdRef) + sizeof(ptrRef),
                IB_SET_TARGET = sizeof(ptrRef),
                IB_WRITE_STRING = 0,
                IB_STOP = 0,
                TEXTINPUT_START = sizeof(ptrRef),
                TEXTINPUT_STOP = 0,

                FUNC_START = sizeof(uint32_t) + sizeof(uint32_t), // Func ID, to be identified by the linker, followed by local header location.
                FUNC_CALL = sizeof(uint32_t), // Func ID, to be matched to a FUNC_START by the linker.
                FUNC_LIST = DYNAMIC_SIZE, // FUNC_LIST acts as an array of function pointers.
                FUNC_LIST_CALL = sizeof(uint32_t) + sizeof(stdRef),        // Calls an element from a FUNC_LIST. The ID is of the list, not the access. If access is constant, compiler will substitute for a FUNC_CALL.

                DEBUG_BREAK = 0,
                ERROR_CLEAR = 0;
        );
}


#define GET_INSTRUCTION_SIZE(instruction) nthp::script::instructions::Size::instruction

}}}
