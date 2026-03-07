// NOTE TO FUTURE SELF; DO NOT REMOVE THIS SEEMINGLY POINTLESS/REDUNDANT CHECK;
// ALL THE FUNCTIONS IN THIS FILE MUST EXIST, REGARDLESS OF DEBUG FLAG. THE HEADER ACTS
// AS A SWITCH TO DEFINE EACH DEBUG OUTPUT FUNCTION AS A MACRO OR EXTERN; FOR FILES WHERE IT'S
// DEFINED AS AN EXTERN, THESE UNITS NEED TO EXIST FOR THE LINKER TO PASS.
//
// THIS MISTAKE WAS CORRECTED DEC. 28, 2025. HAPPY NEW YEAR!
#ifndef DEBUG
	#define DEBUG
#endif


#include "global.hpp"

nthp::fixed_t nthp::deltaTime = 0;
nthp::fixed_t nthp::frameDelay = 0;

nthp::vectFixed nthp::mousePosition;



void nthp::THROW_FATAL(char errorcode, const char* fatal_message) {
        printf("[t %u] FATAL: %s\n", SDL_GetTicks(), fatal_message);

        throw FATAL_ERROR(errorcode);
}

#ifdef DEBUG
FILE* NTHP_debug_output = stdout;

void PRINT_DEBUG(const char* format, ...) {
	va_list ap;
	
	va_start(ap, format);


	fprintf(NTHP_debug_output, "[t %u] DEBUG: ", SDL_GetTicks());	
	vfprintf(NTHP_debug_output, format, ap);


	va_end(ap);

        fflush(NTHP_debug_output);
}

void PRINT_DEBUG_ERROR(const char* format, ...) {

	va_list ap;
	
	va_start(ap, format);
#ifdef PM
        va_list pmOutputList;
        va_copy(pmOutputList, ap);
#endif

	fprintf(NTHP_debug_output, "[t %u] ERROR: ", SDL_GetTicks());	
	vfprintf(NTHP_debug_output, format, ap);
#ifdef PM
        printf("[t %u] ERROR: ", SDL_GetTicks());
        vprintf(format, pmOutputList);

        va_end(pmOutputList);
#endif

	va_end(ap);

        fflush(NTHP_debug_output);
}

void PRINT_DEBUG_WARNING(const char* format, ...) {

	va_list ap;
	
	va_start(ap, format);
#ifdef PM
        va_list pmOutputList;
        va_copy(pmOutputList, ap);
#endif

	fprintf(NTHP_debug_output, "[t %u] WARNING: ", SDL_GetTicks());	
	vfprintf(NTHP_debug_output, format, ap);

#ifdef PM
        printf("[t %u] WARNING: ", SDL_GetTicks());
        vprintf(format, pmOutputList);

        va_end(pmOutputList);
#endif

	va_end(ap);
        fflush(NTHP_debug_output);
}


int NTHP_GEN_DEBUG_INIT(FILE* fdescriptor) {
        if(fdescriptor == NULL) {
                NTHP_debug_output = stdout;
                return -1;
        }

        NTHP_debug_output = fdescriptor;


        time_t ti = time(NULL);

        // I know how it looks, but 'localtime' is statically allocated,
        // so this is not a memory leak.
        struct tm *info = localtime(&ti);


        fprintf(NTHP_debug_output, "\tNTHP Debug Session Start : %s\n", asctime(info));
        return 0;
}

void NTHP_GEN_DEBUG_CLOSE(void) {
        if(NTHP_debug_output != NULL) {        
                time_t ti = time(NULL);

                // I know how it looks, but 'localtime' is statically allocated,
                // so this is not a memory leak.
                struct tm *info = localtime(&ti);
                fprintf(NTHP_debug_output, "\n\tNTHP Debug Session End : %s", asctime(info));


                if(NTHP_debug_output != stdout) {
                        fclose(NTHP_debug_output);
                }

                NTHP_debug_output = stdout;
        }
}
#endif


