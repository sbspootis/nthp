#pragma once
#include "s_node.hpp"



namespace nthp { namespace script { 
        
        typedef enum {
                MOUSEPOS_X_GLOBAL_INDEX,
                MOUSEPOS_Y_GLOBAL_INDEX,
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
                INC,\
                DEC,\
                LSHIFT,\
                RSHIFT,\
                ADD,\
                SUB,\
                MUL,\
                DIV,\
                SQRT,\
                ABS,\
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
		TEXTURE_ALLOC,\
		TEXTURE_CLEAR,\
                TEXTURE_TARGET,\
		TEXTURE_LOAD,\
		SET_ACTIVE_PALETTE,\
                FRAME_DEFINE,\
                FRAME_CLEAR,\
                FRAME_SET,\
                SM_READ,\
                SM_WRITE,\
                ENT_DEFINE,\
                ENT_CLEAR,\
                ENT_SETCURRENTFRAME,\
                ENT_SETPOS,\
                ENT_MOVE,\
                ENT_SETFRAMERANGE,\
                ENT_SETHITBOXSIZE,\
                ENT_SETHITBOXOFFSET,\
                ENT_SETRENDERSIZE,\
                ENT_CHECKCOLLISION,\
                CORE_INIT,\
                CORE_QRENDER,\
                CORE_ABS_QRENDER,\
                CORE_CLEAR,\
                CORE_DISPLAY,\
                CORE_SETMAXFPS,\
                CORE_SETWINDOWRES,\
                CORE_SETCAMERARES,\
                CORE_SETCAMERAPOSITION,\
                CORE_MOVECAMERA,\
                CORE_STOP,\
                ACTION_DEFINE,\
                ACTION_BIND,\
                ACTION_CLEAR,\
                STAGE_LOAD,\
                POLL_ENT_POSITION,\
                POLL_ENT_HITBOX,\
                POLL_ENT_RENDERSIZE,\
                POLL_ENT_CURRENTFRAME,\
                DRAW_SETCOLOR,\
                DRAW_LINE,\
                SOUND_DEFINE,\
                SOUND_CLEAR,\
                MUSIC_DEFINE,\
                MUSIC_CLEAR,\
                MUSIC_LOAD,\
                SOUND_LOAD,\
                SOUND_PLAY,\
                MUSIC_START,\
                MUSIC_STOP,\
                MUSIC_PAUSE,\
                MUSIC_RESUME,\
                DFILE_READ,\
                DFILE_WRITE,\
                PRINT_REF,\
                PRINT_STRING,\
                STRING,\
                STRING_COPY,\
                IB_SET_TARGET,\
                IB_WRITE_STRING,\
                IB_STOP,\
                FUNC_START,\
                FUNC_CALL\
        )

        INSTRUCTION_LIST( INSTRUCTION_TOKENS(), numberOfInstructions);
}

#define GET_INSTRUCTION_ID(instruction) nthp::script::instructions::ID::instruction
typedef P_Reference<nthp::script::stdVarWidth> stdRef;  // The standard value type; Can be a reference to memory or a constant, 'metadata' bits can be set for type description. The endpoint should be a workable value.
typedef stdRef ptrRef;                                  // ptrRef endpoints are evaluated as ptr_descriptors; otherwise identical to stdRefs
typedef stdRef strRef;                                  // strRef endpoints are the same as stdRef, but instead point to a STRING node or assumed block data (ptr_descriptor) string.


// Sizes must have the same name as the ENUM entry in 'ID'.
namespace Size {

        INSTRUCTION_SIZE_LIST(
                HEADER = DYNAMIC_SIZE,
                EXIT = 0,

                LABEL = sizeof(uint32_t),
                GOTO = sizeof(uint32_t),
                JUMP = sizeof(stdRef),
                SUSPEND = 0,
                RETURN = 0,
                GETINDEX = sizeof(ptrRef),

                INC = sizeof(ptrRef),
                DEC = sizeof(ptrRef),
                RSHIFT = sizeof(ptrRef) + sizeof(stdRef),
                LSHIFT = sizeof(ptrRef) + sizeof(stdRef),

                ADD = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                SUB = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                MUL = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                DIV = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),
                SQRT = sizeof(stdRef) + sizeof(ptrRef),
                ABS = sizeof(stdRef) + sizeof(ptrRef),


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
                INDEX = sizeof(ptrRef) + sizeof(stdRef),

		TEXTURE_ALLOC = sizeof(stdRef) + sizeof(ptrRef),
		TEXTURE_CLEAR = sizeof(ptrRef),
                TEXTURE_TARGET = sizeof(ptrRef),
		TEXTURE_LOAD = sizeof(stdRef) + sizeof(strRef), 
		SET_ACTIVE_PALETTE = sizeof(strRef), 


                FRAME_DEFINE = sizeof(stdRef),
                FRAME_CLEAR = 0,
                FRAME_SET = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                SM_WRITE = sizeof(stdRef) + sizeof(stdRef),
                SM_READ = sizeof(stdRef) + sizeof(ptrRef),

                ENT_DEFINE = sizeof(stdRef),
                ENT_CLEAR = 0,
                ENT_SETCURRENTFRAME = sizeof(stdRef) + sizeof(stdRef),
                ENT_SETPOS = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_MOVE = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETFRAMERANGE = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETHITBOXSIZE = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETHITBOXOFFSET = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_SETRENDERSIZE = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                ENT_CHECKCOLLISION = sizeof(stdRef) + sizeof(stdRef) + sizeof(ptrRef),


                CORE_INIT = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(uint8_t) + sizeof(strRef),
                CORE_QRENDER = sizeof(stdRef),
                CORE_ABS_QRENDER = sizeof(stdRef),
                CORE_CLEAR = 0,
                CORE_DISPLAY = 0,
                CORE_SETMAXFPS = sizeof(stdRef),
                CORE_SETWINDOWRES = sizeof(stdRef) + sizeof(stdRef),
                CORE_SETCAMERARES = sizeof(stdRef) + sizeof(stdRef),
                CORE_SETCAMERAPOSITION = sizeof(stdRef) + sizeof(stdRef),
                CORE_MOVECAMERA = sizeof(stdRef) + sizeof(stdRef),
                CORE_STOP = 0,

                ACTION_DEFINE = sizeof(stdRef),
                ACTION_CLEAR = 0, //
                ACTION_BIND =  sizeof(stdRef) + sizeof(ptrRef) + sizeof(int32_t), // actionIndex, varIndex, key
                STAGE_LOAD = sizeof(strRef),

                POLL_ENT_POSITION = sizeof(stdRef),
                POLL_ENT_HITBOX = sizeof(stdRef),
                POLL_ENT_RENDERSIZE = sizeof(stdRef),
                POLL_ENT_CURRENTFRAME = sizeof(stdRef),

                DRAW_SETCOLOR = sizeof(stdRef),
                DRAW_LINE = sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef),
                
                SOUND_DEFINE = sizeof(stdRef),
                SOUND_CLEAR = 0,
                MUSIC_DEFINE = sizeof(stdRef),
                MUSIC_CLEAR = 0,
                MUSIC_LOAD = sizeof(stdRef) + sizeof(strRef),
                SOUND_LOAD = sizeof(stdRef) + sizeof(strRef),
                SOUND_PLAY = sizeof(stdRef),
                MUSIC_START = sizeof(stdRef),
                MUSIC_STOP = 0,
                MUSIC_PAUSE = 0,
                MUSIC_RESUME = 0,

                DFILE_READ = sizeof(ptrRef) + sizeof(strRef),
                DFILE_WRITE = sizeof(ptrRef) + sizeof(strRef),

                PRINT_REF = sizeof(stdRef),
                PRINT_STRING = sizeof(strRef),
                STRING = DYNAMIC_SIZE,
                STRING_COPY = sizeof(ptrRef) + sizeof(strRef),
                IB_SET_TARGET = sizeof(ptrRef),
                IB_WRITE_STRING = 0,
                IB_STOP = 0,
                FUNC_START = sizeof(uint32_t) + sizeof(uint32_t), // Func ID, to be identified by the linker, followed by local header location.
                FUNC_CALL = sizeof(uint32_t) // Func ID, to be matched to a FUNC_START by the linker.
        );
}


#define GET_INSTRUCTION_SIZE(instruction) nthp::script::instructions::Size::instruction

}}}
