#include "global.hpp"
#include "core.hpp"
#include "ray.hpp"
#include "position.hpp"
#include "palette.hpp"
#include "softwaretexture.hpp"
#include "e_entity.hpp"
#include "st_compress.hpp"
#include "s_linker.hpp"
#include "s_runtime.hpp"
#include "st_mono.hpp"


nthp::EngineCore nthp::core;
nthp::script::Runtime mainRuntime;


int nthp::runtimeBehaviour(int argv, char** argc) {
        // The DEBUG_INIT is called at the start of main, and DEBUG_CLOSE
        // is called after the destruction of the main core.

        


#ifdef DEBUG
        NTHP_GEN_DEBUG_INIT(stdout);
       //NTHP_GEN_DEBUG_INIT(fopen("debug.log", "w+"));
#endif
        { // The entire engine debug context.
               
                auto frameStart = SDL_GetTicks();


                // Anyone would agree an infinite loop here is acceptable.
                while(true) {
                        mainRuntime.importExecutable("m_prog.p");

                        // Init phase.
                        if(mainRuntime.execInit()) return -1;


  
                        
                        while(nthp::core.isRunning() && (!mainRuntime.data.changeStage)) {
                                frameStart = SDL_GetTicks();

                                mainRuntime.handleEvents();

                                // Tick phase.
                                mainRuntime.execTick();

                                nthp::deltaTime = nthp::intToFixed(SDL_GetTicks() - frameStart);
                                if(nthp::deltaTime < nthp::frameDelay) {
                                        SDL_Delay(nthp::fixedToInt(nthp::frameDelay - nthp::deltaTime));
                                        nthp::deltaTime = nthp::getFixedInteger(nthp::frameDelay);
                                }
                                mainRuntime.data.blockData[0].data[nthp::script::predefined_globals::DELTATIME_GLOBAL_INDEX] = nthp::deltaTime;
                        }
                        
                        
                        // Exit Phase
                        if(mainRuntime.execExit()) return -1;

                        break;
                }
                
        }

        return 0;
}




int main(int argv, char** argc) {
        return nthp::runtimeBehaviour(argv, argc);
}
