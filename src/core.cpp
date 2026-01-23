#include "core.hpp"

std::vector<std::string> nthp::audio::audioDeviceNames;


nthp::EngineCore::EngineCore(nthp::RenderRuleSet settings, const char* title, bool fullscreen, bool softwareRendering) {
        window = nullptr;
        renderer = nullptr;
        running = false;
        initSuccess = false;

        if(this->init(settings, title, fullscreen, softwareRendering)) { }
}

int nthp::EngineCore::init(nthp::RenderRuleSet settings, const char* title, bool fullscreen, bool softwareRendering) {
        p_coreDisplay = settings;
        SDL_StopTextInput();

        // If already initialized, skip the init.
        if(!initSuccess) {
                PRINT_DEBUG("Initializing SDL binaries...\t");

                auto flags = SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER;
                
                if(SDL_Init(flags) != 0) {
                        FATAL_PRINT(nthp::FATAL_ERROR::SDL_Failure, SDL_GetError());
                }

        #if USE_SDLIMG == 1
                auto imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;

                if(IMG_Init(imgFlags) != imgFlags) {
                        FATAL_PRINT(nthp::FATAL_ERROR::SDL_Failure, SDL_GetError());
                }
        #endif
                NOVERB_PRINT_DEBUG("done.\n");
        }
        PRINT_DEBUG("Setting up window and renderer...\t");

        int fullscreenFlag = 0;
        if(fullscreen) {
                fullscreenFlag = SDL_WINDOW_FULLSCREEN;
        }

        window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, p_coreDisplay.pxlResolution_x, p_coreDisplay.pxlResolution_y, fullscreenFlag);
        if(window == NULL) {
                FATAL_PRINT(nthp::FATAL_ERROR::SDL_Failure, SDL_GetError());
        }


        if(softwareRendering)
                renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        else
                renderer = SDL_CreateRenderer(window, -1, 0);


        if(renderer == NULL) {
                FATAL_PRINT(nthp::FATAL_ERROR::SDL_Failure, SDL_GetError());
        }
        NOVERB_PRINT_DEBUG("done.\n");
        PRINT_DEBUG("Configuring defaults...\t");

        SDL_SetRenderDrawColor(renderer, DEFAULT_RENDER_COLOR);
        running = true;
	
	// This ensures the correct render resolution context when calculating scale factors.
	// If the requested resolution is too small (or too large), SDL will correct the resolution to
	// match the aspect ratio of the display and capabilities of the graphics card. Querying for them
        // might seem redundant, but it isn't.
	{
		int w, h;
		SDL_GetRendererOutputSize(renderer, &w, &h);
		p_coreDisplay = nthp::RenderRuleSet(w, h, settings.tunitResolution_x, settings.tunitResolution_y, settings.cameraWorldPosition);
	}


        initSuccess = true;
        NOVERB_PRINT_DEBUG("done.\n\n");


        if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
                PRINT_DEBUG_ERROR("Unable to initialize audio systems. Disabling audio system until restart.\n");

                // TODO

                return 0;
        }

        const size_t deviceCount = SDL_GetNumAudioDevices(0);
        PRINT_DEBUG("Searching for audio devices...");
        for(size_t i = 0; i < deviceCount; ++i) {
                nthp::audio::audioDeviceNames.push_back(std::string(SDL_GetAudioDeviceName(i, 0)));
                NOVERB_PRINT_DEBUG("\n\tDetected audio device: %s...", nthp::audio::audioDeviceNames[i].c_str());
        }
        NOVERB_PRINT_DEBUG("Done.\n");



        return 0;
}


void nthp::EngineCore::handleEvents() {
        int x,y;
        SDL_GetMouseState(&x, &y);
        nthp::mousePosition = nthp::generateWorldPosition(nthp::vectGeneric(x, y), &p_coreDisplay);
        nthp::mousePosition -= p_coreDisplay.cameraWorldPosition;

        while(SDL_PollEvent(&eventList)) {
                switch(eventList.type) {
                case SDL_QUIT:
                        running = false;
                        break;
                }
        }
}

void nthp::EngineCore::handleEvents(void (*handler)(SDL_Event*)) {
        int x,y;
        SDL_GetMouseState(&x, &y);
        nthp::mousePosition = nthp::generateWorldPosition(nthp::vectGeneric(x, y), &p_coreDisplay);
        nthp::mousePosition -= p_coreDisplay.cameraWorldPosition;

        while(SDL_PollEvent(&eventList)) {
                switch(eventList.type) {
                case SDL_QUIT:
                        running = false;
                        break;
                default:
                        handler(&eventList);
                        break;
                }
        }
}




void nthp::EngineCore::display() {
        SDL_RenderPresent(renderer);
}

void nthp::EngineCore::clear() {
        SDL_RenderClear(renderer);
}

void nthp::EngineCore::stop() {
        running = false;
}


int nthp::EngineCore::render(nthp::RenderPacket packet) {
        switch(packet.state) {
                case nthp::RenderPacket::C_OPERATE::VALID:
                        {
                                const vectGeneric offset = nthp::generatePixelPosition(nthp::worldPosition(p_coreDisplay.cameraWorldPosition.x, p_coreDisplay.cameraWorldPosition.y), &p_coreDisplay);
                                packet.dstRect.x += offset.x;
                                packet.dstRect.y += offset.y;
                                return SDL_RenderCopyEx(renderer, packet.texture, packet.srcRect, &packet.dstRect, packet.angle, NULL, SDL_RendererFlip::SDL_FLIP_NONE);
                        }
                        break;
                case nthp::RenderPacket::C_OPERATE::ABSOLUTE:
                        return SDL_RenderCopyEx(renderer, packet.texture, packet.srcRect, &packet.dstRect, packet.angle, NULL, SDL_RendererFlip::SDL_FLIP_NONE);
                        break;


                case nthp::RenderPacket::C_OPERATE::INVALID:
                        return 1;
                        break;
                default:
                        break;
        }

        return 0;
}

void nthp::EngineCore::setWindowRenderSize(int x, int y) {
        if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
                SDL_DisplayMode mode;
                SDL_GetWindowDisplayMode(window, &mode);
                
                // For some fucking reason, SDL Can't resize a window while in fullscreen,
                // Even when tweaking the DisplayMode. :/
                SDL_SetWindowFullscreen(window, 0);
                mode.w = x;
                mode.h = y;

                if(SDL_SetWindowDisplayMode(window, &mode) < 0) {
                        PRINT_DEBUG_ERROR("%s\n", SDL_GetError());
                }
                SDL_GetWindowDisplayMode(window, &mode);

                p_coreDisplay.pxlResolution_x = mode.w;
                p_coreDisplay.pxlResolution_y = mode.h;

                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }
        else {
                SDL_SetWindowSize(window, x, y);
		#ifdef LINUX // GETWINDOWSIZEINPIXELS is not a valid SDLcall on linux. Although not exactly the same, works in most cases.
	                SDL_GetWindowSize(window, &x, &y);
		#endif

		#ifdef WINDOWS
			SDL_GetWindowSizeInPixels(window, &x, &y);
		#endif

		
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

                p_coreDisplay.pxlResolution_x = x;
                p_coreDisplay.pxlResolution_y = y;
        }

        p_coreDisplay.updateScaleFactor();
}

void nthp::EngineCore::setVirtualRenderScale(nthp::fixed_t x, nthp::fixed_t y) {
        p_coreDisplay.tunitResolution_x = x;
        p_coreDisplay.tunitResolution_y = y;

        p_coreDisplay.updateScaleFactor();
}



int nthp::EngineCore::cleanup() {
        PRINT_DEBUG("Destroying Core and cleaning up...  ");

        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        Mix_CloseAudio();

        NOVERB_PRINT_DEBUG("done.\n");

        return 0;
}

void nthp::EngineCore::stopTextInput() {
        SDL_StopTextInput();
}

void nthp::EngineCore::startTextInput() {
        SDL_StartTextInput();
}

nthp::EngineCore::~EngineCore() {
        cleanup();

        SDL_Quit();
#if USE_SDLIMG == 1
        IMG_Quit();
#endif
        Mix_Quit();

        initSuccess = false;

#ifdef DEBUG
        NTHP_GEN_DEBUG_CLOSE();
#endif

}

nthp::RenderRuleSet::RenderRuleSet() { 
        pxlResolution_x = 0;
        pxlResolution_y = 0;
        tunitResolution_x = 0;
        tunitResolution_y = 0;
}

nthp::RenderRuleSet::RenderRuleSet(FIXED_TYPE x, FIXED_TYPE y, nthp::fixed_t tx, nthp::fixed_t ty, vectFixed cameraPosition) {
        pxlResolution_x = x;
        pxlResolution_y = y;
        tunitResolution_x = tx;
        tunitResolution_y = ty;

        cameraWorldPosition = cameraPosition;

       updateScaleFactor();
}


// If you're running this every frame, and need the speed, use nocast_updateScaleFactor() instead.
// High accuracy, slower calculation.
void nthp::RenderRuleSet::updateScaleFactor() {
        // Yes yes I know. But the precision is too important here to pass up.
        // It gets converted afterwards back to fixed-point, so overall speed is better.
        // If you're running this every frame, and need the speed, use nocast_updateScaleFactor() instead.
	float xs, ys;
	xs = (float)pxlResolution_x / nthp::fixedToDouble(tunitResolution_x);
	ys = (float)pxlResolution_y / nthp::fixedToDouble(tunitResolution_y);


	scaleFactor.x = nthp::doubleToFixed(xs);
	scaleFactor.y = nthp::doubleToFixed(ys);
        PRINT_DEBUG("Recalculated scale factor ; ScaleX=%lf ScaleY=%lf\n", nthp::fixedToDouble(scaleFactor.x), nthp::fixedToDouble(scaleFactor.y));
}

// Low accuracy, faster calculation. Useful for continuous camera scaling.
// If using a single camera configuration for the whole program, use updateScaleFactor() for higher accuracy.
void nthp::RenderRuleSet::nocast_updateScaleFactor() {
        scaleFactor = nthp::vectFixed(nthp::f_fixedQuotient(nthp::intToFixed(pxlResolution_x), tunitResolution_x), nthp::f_fixedQuotient(nthp::intToFixed(pxlResolution_y), tunitResolution_y));
}
