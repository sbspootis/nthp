#pragma once
#ifdef WINDOWS
        #define WIN32_LEAN_AND_MEAN
        #define _WIN32_WINNT 0x0A00

        #undef PM
        #include "imgui/imgui.h"
        #include "imgui/imgui_impl_sdl2.h"
        #include "imgui/imgui_impl_sdlrenderer2.h"
        #include "imgui/imgui_stdlib.h"
        
        #define PM
#endif

#ifdef LINUX
        #undef PM
        #include "imgui/imgui.h"
        #include "imgui/imgui_impl_sdl2.h"
        #include "imgui/imgui_impl_sdlrenderer2.h"
        #include <SDL2/SDL_syswm.h>

#endif

#include "s_linker.hpp"
#include "s_script.hpp"
#include "s_runtime.hpp"
#include "st_font.hpp"
#include "st_rendertext.hpp"
#include <sstream>
#include <thread>
#include <mutex>


#define PM_PRINT(...) printf(__VA_ARGS__)
extern void	PM_PRINT_ERROR(const char* format, ...);

#ifdef PM_VERBOSE
	#define  PM_PRINT_V(...) printf(__VA_ARGS__);
#else
	#define PM_PRINT_V(...)
#endif



namespace nthp { namespace pm {
        extern nthp::script::Runtime mainRuntime;

}}