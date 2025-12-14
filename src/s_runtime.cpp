#include "s_runtime.hpp"

nthp::script::Runtime::Runtime() { 
        memset(&data, 0, sizeof(data));
}


nthp::script::Runtime::Runtime(const char* filename) {
        memset(&data, 0, sizeof(data));

        if(importExecutable(filename)) {

        }
}


int nthp::script::Runtime::importExecutable(const char* filename) {
        initList.clear();
        tickList.clear();
        exitList.clear();
        allHeaderLocations.clear();
        scriptData.clear();


        std::fstream file;
        file.open(filename, std::ios::in | std::ios::binary);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Failed to import executable.\n");
                return 1;
        }

        nthp::script::LinkerInstance::link_header header;
        file.read((char*)&header, sizeof(header));
        if(header.HeaderID != nthp::script::LinkerInstance::ID) {
                PRINT_DEBUG_ERROR("Invalid executable file.\n");
                file.close();

                return 1;
        }

        data.nodeSetSize = header.totalNodeCount;
        data.nodeSet = new (std::nothrow) nthp::script::Node[data.nodeSetSize];
        PRINT_DEBUG("Allocated [%zu] nodes.\n", data.nodeSetSize);
        if(data.nodeSet == NULL) { 
                PRINT_DEBUG_ERROR("Unable to allocate executable memory for [%s].\n", filename);
                file.close();

                return 1;
        }

        nthp::script::Node fileRead;
        for(size_t i = 0; i < data.nodeSetSize; ++i) {
                file.read((char*)&fileRead, sizeof(nthp::script::Node::n_file_t));
                if(fileRead.access.size) { 
                        fileRead.access.data = (char*)malloc(fileRead.access.size);
                        file.read(fileRead.access.data, fileRead.access.size);

                }
                else fileRead.access.data = NULL;

                // Might seem a little bad, but it's ok!
                data.nodeSet[i].access = fileRead.access;
                NOVERB_PRINT_DEBUG("\t[%zu] ID:%u Size:%u\n", i, data.nodeSet[i].access.ID, data.nodeSet[i].access.size);
        }


        {
                volatile uint8_t flags = 0;
                volatile uint32_t globalMemoryNeeded = 0;
                data.globalMemBudget = nthp::script::predefined_globals::COUNT_PREDEFINED_GLOBALS;

                // Once data is copied, scan through for header locations; add location to respective
                // trigger list and headerLocations.
                for(uint32_t i = 0; i < data.nodeSetSize; ++i) {
                        if(data.nodeSet[i].access.ID == GET_INSTRUCTION_ID(HEADER)) {
                                allHeaderLocations.push_back(i);

                                globalMemoryNeeded = nthp::script::CompilerInstance::getScriptGlobalRequest(data.nodeSet[i]);
                                data.globalMemBudget += globalMemoryNeeded;

                                flags = nthp::script::CompilerInstance::getScriptTriggerFlags(data.nodeSet[i]);
                                if(flags & (1 << nthp::script::CompilerInstance::TriggerBits::T_INIT)) { initList.push_back(allHeaderLocations.size() - 1); PRINT_DEBUG("Pushed INITLIST...\n"); }
                                if(flags & (1 << nthp::script::CompilerInstance::TriggerBits::T_TICK)) { tickList.push_back(allHeaderLocations.size() - 1); PRINT_DEBUG("Pushed TICKLIST...\n"); }
                                if(flags & (1 << nthp::script::CompilerInstance::TriggerBits::T_EXIT)) { exitList.push_back(allHeaderLocations.size() - 1); PRINT_DEBUG("Pushed EXITLIST...\n"); }

                                scriptData.push_back(nthp::script::Script(&data, i));
                        }
                }
        }


        auto globalVarSet = nthp::script::nthp_internal_alloc(&data, NULL, nthp::intToFixed(data.globalMemBudget), 0, nthp::script::BlockMemoryEntry::bmType::GLOBAL);
        if(globalVarSet.block) {
                PRINT_DEBUG_ERROR("Unable to allocate global memory.\n");
                return 1;
        }
        
        PRINT_DEBUG("Reserved [%u] entries in GLOBAL list.\n", data.globalMemBudget);
        memset(data.blockData[globalVarSet.block].data, 0, sizeof(nthp::script::stdVarWidth) * data.globalMemBudget);


        return 0;
}



int nthp::script::Runtime::execInit() {
        for(size_t i = 0; i < initList.size(); ++i) {
                if(scriptData[initList[i]].execute()) { return 1; }
        }

        return 0;
}

int nthp::script::Runtime::execTick() {
        for(size_t i = 0; i < tickList.size(); ++i) {
                if(scriptData[tickList[i]].execute()) { return 1; }
        }

        return 0;
}

int nthp::script::Runtime::execExit() {
        for(size_t i = 0; i < exitList.size(); ++i) {
                if(scriptData[exitList[i]].execute()) { return 1; }
        }

        return 0;
}



void nthp::script::Runtime::handleEvents() {
        int x,y;
        SDL_GetMouseState(&x, &y);
        nthp::mousePosition = nthp::generateWorldPosition(nthp::vectGeneric(x, y), &nthp::core.p_coreDisplay);
        nthp::mousePosition -= nthp::core.p_coreDisplay.cameraWorldPosition;

        data.blockData[0].data[MOUSEPOS_X_GLOBAL_INDEX] = nthp::mousePosition.x;
        data.blockData[0].data[MOUSEPOS_Y_GLOBAL_INDEX] = nthp::mousePosition.y;

        data.inputBufferPtr = 0;

        while(SDL_PollEvent(&nthp::core.eventList)) {
                switch(nthp::core.eventList.type) {
                case SDL_QUIT:
                        nthp::core.stop();
                break;

                case SDL_KEYDOWN:
                        if(nthp::core.eventList.key.keysym.sym < 256) {
                                data.inputBuffer[data.inputBufferPtr] = nthp::core.eventList.key.keysym.sym; // Add keypress to the input buffer.
                                ++data.inputBufferPtr;
                        }
                        

                        for(size_t i = 0; i < data.actionListSize; ++i) {
                                if(nthp::core.eventList.key.keysym.sym == data.actionList[i].boundKey) {
                                        *(data.actionList[i].varLocation) = nthp::intToFixed(1);
                                }
                        }

                break;
                
                case SDL_KEYUP:
                        for(size_t i = 0; i < data.actionListSize; ++i) {
                                if(nthp::core.eventList.key.keysym.sym == data.actionList[i].boundKey) {
                                        *(data.actionList[i].varLocation) = 0;
                                }
                        }
                break;

                case SDL_MOUSEBUTTONDOWN:
                        if(nthp::core.eventList.button.button == SDL_BUTTON_LEFT) {
                                data.blockData[0].data[MOUSE1_GLOBAL_INDEX] = nthp::intToFixed(1);
                                break;
                        }
                        if(nthp::core.eventList.button.button == SDL_BUTTON_RIGHT) {
                                data.blockData[0].data[MOUSE2_GLOBAL_INDEX] = nthp::intToFixed(1);
                                break;
                        }
                        if(nthp::core.eventList.button.button == SDL_BUTTON_MIDDLE) {
                                data.blockData[0].data[MOUSE3_GLOBAL_INDEX] = nthp::intToFixed(1);
                                break;
                        }

                        
                        
                break;

                case SDL_MOUSEBUTTONUP:
                        if(nthp::core.eventList.button.button == SDL_BUTTON_LEFT) {
                                data.blockData[0].data[MOUSE1_GLOBAL_INDEX] = 0;
                                break;
                        }
                        if(nthp::core.eventList.button.button == SDL_BUTTON_RIGHT) {
                                data.blockData[0].data[MOUSE2_GLOBAL_INDEX] = 0;
                                break;
                        }
                        if(nthp::core.eventList.button.button == SDL_BUTTON_MIDDLE) {
                                data.blockData[0].data[MOUSE3_GLOBAL_INDEX] = 0;
                                break;
                        }
                break;

                case SDL_TEXTINPUT:
                {
                        if(!data.textInputActive) { break; }

                        uint8_t i = 0;
                        for(;nthp::core.eventList.text.text[i] != '\0'; ++i) {
                                if((data.textInputBufferPosition + i) >= (data.blockData[data.textInputLocation.block].size * sizeof(nthp::script::stdVarWidth))) break;
                                data.textInputTarget[data.textInputBufferPosition + i] = nthp::core.eventList.text.text[i];
                        }
                        data.textInputBufferPosition += i;

                        break;
                }

                
                default:
                        break;
                }

                
        }
        
        data.inputBuffer[data.inputBufferPtr] = 0; // Mark end of buffer with 0
}





















// Cleanly erases the whole stage; scripts, triggers, and any shared script data.
// After calling, stage object is safe to use again (i.e. load another stage file)
void nthp::script::Runtime::clean() {
        nthp::script::Script::cleanDataSet(&data);


}


nthp::script::Runtime::~Runtime() {
        clean();
}
