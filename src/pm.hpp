#pragma once

#include "s_linker.hpp"
#include "s_script.hpp"
#include "s_runtime.hpp"
#include "st_font.hpp"
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
