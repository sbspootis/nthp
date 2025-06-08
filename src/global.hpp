#pragma once

#ifndef NTHP_VERSION
        #define NTHP_VERSION "1.0.3"
#endif

#ifdef LINUX
	#include <SDL2/SDL.h>
        #include <SDL2/SDL_mixer.h>
#endif

#ifdef WINDOWS
	#include <SDL.h>
        #include <SDL_audio.h>
        #include <SDL_mixer.h>
#endif

#if USE_SDLIMG == 1
#ifdef WINDOWS
    	#include <SDL_image.h>
#endif
#ifdef LINUX
	#include <SDL2/SDL_image.h>
#endif

#endif



#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cstdarg>
#include <ctime>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <mutex>
#include "fixed.hpp"



// Use DEBUG_PRINT as a regular printf wrapper. It gets substituted out
// if unless DEBUG is defined 
        #ifdef DEBUG

	        extern FILE* NTHP_debug_output;

                #ifndef SUPPRESS_DEBUG_OUTPUT
	                extern void PRINT_DEBUG                 (const char* format, ...);
                        #define NOVERB_PRINT_DEBUG(...)         fprintf(NTHP_debug_output, __VA_ARGS__)
                        #define GENERIC_PRINT(...)              printf(__VA_ARGS__)

                #else
                        #define PRINT_DEBUG(...)
                        #define NOVERB_PRINT_DEBUG(...)
                        #define GENERIC_PRINT(...)
                #endif

                extern void PRINT_DEBUG_ERROR           (const char* format, ...);
                extern void PRINT_DEBUG_WARNING         (const char* format, ...);


                #else
        // All arguments are substituted out when DEBUG is not defined.
        // The debug build config defines DEBUG in all translation units, but all
        // debug functions are valid in the release build, so debugging individual source
        // files is possible if DEBUG is defined before any includes.

                #define PRINT_DEBUG(...)
                #define NOVERB_PRINT_DEBUG(...)
                #define GENERIC_PRINT(...)
                #define PRINT_DEBUG_ERROR(...)
                #define PRINT_DEBUG_WARNING(...)

        #endif


        // Must be executed first to ensure a valid file descriptor.
        // Specifies a target for the debug output system. Default is 'stdout'
        extern int NTHP_GEN_DEBUG_INIT(FILE* fdescriptor);

        // Can be executed whenever; Ties up loose ends in the debugging system.
        extern void NTHP_GEN_DEBUG_CLOSE(void);

namespace nthp {
        typedef enum {
                SDL_Failure,
                Memory_Fault,
                CriticalFileFailure
        } FATAL_ERROR;

        extern void THROW_FATAL(char errorcode, const char* fatal_message);

        #define NTHP_TRUE       1
        #define NTHP_FALSE      0
        
        // Outputs a message and crashes the program. Use nthp::FATAL_ERROR values for
        // errorcode corresponding to the fatal fault.
        #define FATAL_PRINT(errorcode, message) nthp::THROW_FATAL(errorcode, message)

    
        template<class T>
        class vector {
        public:
                vector() { x = 0; y = 0; };
                vector(T x, T y) { this->x = x; this->y = y; };
                
                vector<T> operator+(vector<T> a) { return vector<T>(x + a.x, y + a.y); }
                vector<T> operator-(vector<T> a) { return vector<T>(x - a.x, y - a.y); }
                void operator+=(vector<T> a) { x += a.x; y += a.y; }
                void operator-=(vector<T> a) { x -= a.x; y -= a.y; }

                T x;
                T y;
        };
        typedef vector<int> vectGeneric;

        typedef vector<int32_t> vect32;
        typedef vector<int64_t> vect64;

        typedef vector<float> vectf32;
        typedef vector<double> vectf64;

        typedef vector<fixed_t> vectFixed;

        // The number of milliseconds the previous frame took to complete (logic and rendering)
        extern fixed_t deltaTime;

        // The maximum time a frame can take in milliseconds. Set to -1 to disable.
        extern fixed_t frameDelay;

        // The mouse's current WorldPosition (not pixel position, be careful) relative to the active window
        extern vectFixed mousePosition;

        // Sets the max framerate in FPS to nthp::frameDelay
        inline void setMaxFPS(const FIXED_TYPE fps) { nthp::frameDelay = nthp::f_fixedQuotient(nthp::intToFixed(1000), nthp::intToFixed(fps)); }



        // Light dynamic storage class. Didn't want to use std::array for reasons
        // I can't rememeber. Probably something stupid.
        template<class T>
        class sArray {
        public:
		sArray() { array = nullptr; size = 0; }
                sArray(size_t size) {
                        array = new T[size];
                        this->size = size;
                }

                T& operator[](size_t index) {
                        if(index > size) PRINT_DEBUG_ERROR("Segmentation Fault in sArray.");
                        return array[index];
                }
                inline T* getData() { return array; }
                inline size_t getSize() { return size; }

                inline T*& getsArrayDataPointer() { return array; }

		void alloc(size_t t_size) { array = new T[t_size]; this->size = t_size; }
		void dealloc() { delete[] array; size = 0; }

                inline size_t getBinarySize() { return (size * sizeof(T)); }

                ~sArray() { if(size > 0) delete[] array; }
        private:
                T* array;
                size_t size;
        };


        struct RenderRuleSet {
        public:
		RenderRuleSet();
                RenderRuleSet(FIXED_TYPE x, FIXED_TYPE y, fixed_t tx, fixed_t ty, vectFixed cameraPosition);
                void updateScaleFactor();

                // Uses the f_fixedQuotient and f_fixedProduct operations instead of converting to
                // floating point. Faster to compute, but considerably lower in accuracy
                // (depending on the configuration of the operations, see fixed.hpp).
                void nocast_updateScaleFactor();

		FIXED_TYPE pxlResolution_x;
		FIXED_TYPE pxlResolution_y;
		fixed_t tunitResolution_x;
		fixed_t tunitResolution_y;
                vectFixed scaleFactor;

		vectFixed cameraWorldPosition;
  
        };


#ifndef PM
        extern int runtimeBehaviour(int argv, char** argc);
#else
        extern int debuggerBehaviour(std::string target, FILE* debugOutputTarget);
#endif


template<class width>
width checksum(void* data, size_t blockSize) {
        width* blocks = (width*)data;
        width result = blocks[0] ^ blocks[1];

        for(size_t i = 2; i < blockSize; ++i) {
                result ^= blocks[i];
        }

        return result;
}



}


// Global header file; definitions shared across all source files.
// Try to only add what's nessesary.

