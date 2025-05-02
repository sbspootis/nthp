#ifndef PM
        #define PM
#endif

#include "pm.hpp"

// NTHP Project manager; compiles scripts, stages, textures, and palettes! Planned with built-in
// debugger, allowing to step and breakpoint scripts with a virtual machine. Headless version first, then
// GUI.

nthp::EngineCore nthp::core;
nthp::script::Runtime mainRuntime;
nthp::script::CompilerInstance symbolData;
std::string testTarget;

bool debuggingActiveProcess = false;
bool suspendExecution = false;



int headless_runtime();
int help_headless(std::vector<std::string>& args);
inline void help_output() {
        PM_PRINT("NTHP Project Manager Help --\n\t  [expression] = optional flag/argument.\n\tCommands: compile, gt, ct, debug, exit, help\ngt [-c (compress output)] palette_file input_image output_texture\nct input_texture output_compressed_texture\ncompile (src or stg) [-f (force)] input_file output_file\ndebug input_prog_directive_file [debug_log_output_file]\nexit\n");
}



int nthp::debuggerBehaviour(std::string target, FILE* debugOutputTarget) {
        // The DEBUG_INIT is called at the start of main, and DEBUG_CLOSE
        // is called after the destruction of the main core.
        std::mutex g_access;

        { // The entire engine debug context.
                auto frameStart = SDL_GetTicks();


                // Anyone would agree an infinite loop here is acceptable.
                while(true) {
                        if(mainRuntime.importExecutable(target.c_str())) return 1;
                        memset(nthp::script::stageMemory, 0, STAGEMEM_MAX);

                        // Init phase.
                        PRINT_DEBUG("Beginning INIT phase...\n");
               
                    

                        if(mainRuntime.execInit()) return 1;
                        g_access.lock();

                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::DEBUG_CALLS::BREAK;
                        nthp::script::debug::suspendExecution = true;
                        PM_PRINT("Ready. Waiting for continue (c)...\n");

                        g_access.unlock();

                        
                        while((nthp::core.isRunning()) && (!mainRuntime.data.changeStage) && debuggingActiveProcess) {
                                frameStart = SDL_GetTicks();
                                
                                mainRuntime.handleEvents();


                                
                                // Tick phase.
                                mainRuntime.execTick();

                                deltaTime = nthp::intToFixed(SDL_GetTicks() - frameStart);
                        
                                if(nthp::deltaTime < nthp::frameDelay) {
                                        SDL_Delay(nthp::fixedToInt(nthp::frameDelay - nthp::deltaTime));
                                        nthp::deltaTime = nthp::frameDelay;
                                }
                                
                                g_access.lock();
                                        mainRuntime.data.globalVarSet[nthp::script::predefined_globals::DELTATIME_GLOBAL_INDEX] = nthp::deltaTime;
                                g_access.unlock();
                        }

                        // Exit Phase
                        PRINT_DEBUG("Beginning EXIT phase...\n");

                        
                        if(mainRuntime.execExit()) return 1;

                        // If CHANGESTAGE is false after the exit phase, then the core must've been stopped,
                        // so exit the program. Otherwise, redo the init phase with the new stage. 
                        if(mainRuntime.data.changeStage && debuggingActiveProcess) {
                                mainRuntime.data.changeStage = false;
                                // StageMemory has been set to the new filename.
                                continue;
                        }
                        else {
                                debuggingActiveProcess = false;
                                suspendExecution = false;
                                nthp::script::debug::suspendExecution = false;
                                break;
                        }
                }
        }
        g_access.lock();

        mainRuntime.clean();
        nthp::core.cleanup();
        symbolData.clean();

        g_access.unlock();
        
        return 0;
}



bool Kill_main_process = false;
bool inHeadlessMode = false;

int main(int argv, char** argc) {
        nthp::setMaxFPS(15);
        std::mutex g_access;


        std::thread debuggerThread(headless_runtime);
        std::string debugOutput;
        FILE* debug_fd = stdout;



	if(argv > 1) {
                if(std::string(argc[1]) == "-h") {
                        inHeadlessMode = true;
                        if(debuggerThread.joinable()) debuggerThread.join();
                        return 0;
                }
                testTarget = argc[1];

                if(argv > 2) {
                        debugOutput = argc[2];
                }
                else {
                        debugOutput = "debug.log";
                }
        }
        else {
                testTarget = "prog";
                debugOutput = "debug.log";
        }

	if(debugOutput != "stdout") {
		debug_fd = fopen(debugOutput.c_str(), "w+");
		if(debug_fd == NULL) {
			PM_PRINT_ERROR("Unable to access debug output file descriptor. Defaulting to standard output.\n");
			debug_fd = stdout;
		}
	}
	
	NTHP_GEN_DEBUG_INIT(debug_fd);


        // Checks every frame if the debugger requests a session.
        do {
                std::this_thread::sleep_for(std::chrono::milliseconds(nthp::fixedToInt(nthp::frameDelay)));
                
                if(debuggingActiveProcess) {

                                               

                        int ret = nthp::debuggerBehaviour(testTarget, debug_fd);
                        if(ret) {
                                PM_PRINT_ERROR("\nCritical failure in debugger; return code %d\n", ret);
                        }
                        else {
                                PM_PRINT("\nCompleted debugging session without critical errors.\n");
                        }
                        
       
                        nthp::setMaxFPS(15);
                        std::cout << "> ";
                }
        }
        while(!Kill_main_process);
       	

	NTHP_GEN_DEBUG_CLOSE();


        if(debuggerThread.joinable()) debuggerThread.join();
        return 0;
}


int singleThread_debugger() {
        return 0;
}



int headless_runtime() {
        PM_PRINT("Pm.exe;\nNTHP Game Engine project manager v." NTHP_VERSION "\nType 'help' for instructions.\n\n");
	std::vector<std::string> args;
	std::mutex g_access;

	std::string input, arg, configTestingTarget = "";
	bool isRunning = true;
        

	while(isRunning) {
		args.clear();
		std::cin.clear();

                if(debuggingActiveProcess) std::cout << "debug> ";
		else std::cout << "> ";

		std::getline(std::cin, input);

		if(input != "") {
			{ // Separates all input symbols into the args vector.
				std::istringstream inputStream(input);
				while(std::getline(inputStream, arg, ' '))
					args.push_back(arg);
			}

			if(args[0] == "exit") {
                                g_access.lock();

                                Kill_main_process = true;
                                debuggingActiveProcess = false;

                                g_access.unlock();
				return 0;
			}

			if(args[0] == "compile" || args[0] == "cc") {
				nthp::script::CompilerInstance cc;
				if(args[1] == "src") {
					if(args.size() < 4) {
						PM_PRINT_ERROR("No output file specified.\n");
						continue;
					}
					
					if(!cc.compileSourceFile(args[2].c_str(), args[3].c_str(), false, (1 << nthp::script::CompilerInstance::TriggerBits::T_TICK), false)) {
					        PM_PRINT("Script, Done. %s > %s\n", args[2].c_str(), args[3].c_str());
                                        }
                                        else {
                                                PM_PRINT_ERROR("Failure in CompilerInstance [%p].\n", &cc);
                                        }
                                        continue;
				}
				if(args[1] == "stg") {
                                        bool forceBuild = false;
                                        int sizeTarget = 0;
					if(args.size() < 3) {
						PM_PRINT_ERROR("No file specified.\n");
						continue;
					}
                                        if(args[2] == "-f") { forceBuild = true; sizeTarget = 1; }
					if(!cc.compileStageConfig(args[2 + sizeTarget].c_str(), NULL, forceBuild, false)) {
					        PM_PRINT("Stage, Done. %s complete.\n", args[2 + sizeTarget].c_str());
                                        }
                                        else {
                                                PM_PRINT_ERROR("Failure in CompilerInstance [%p].\n", &cc);
                                        }
					continue;
				}
				
				PM_PRINT_ERROR("No compilation behaviour specified. Aborting.\n");
				continue;
			}
                        if(args[0] == "link" || args[0] == "lk") {
                                nthp::script::LinkerInstance linker;
                                std::vector<std::string> targetFiles;

                                PM_PRINT("Linking files...\n");
                                for(size_t i = 0; i < args.size() - 2; ++i) {
                                        PM_PRINT("%s, ", args[i + 1].c_str());
                                        targetFiles.push_back(args[i + 1]);
                                }
                                linker.linkFiles(targetFiles, args.back().c_str());

                                PM_PRINT("\ndone. output executable = [%s].\n", args.back().c_str());
                                continue;
                        }
                        if(args[0] == "build" || args[0] == "bd") {
                                if(args.size() < 3) { PM_PRINT("Build failure; syntax error. bd [buildsystem] [outputExecutable]\n"); continue; }

                                nthp::script::CompilerInstance cc;
                                nthp::script::LinkerInstance linker;
                                std::vector<std::string> outputFiles;

                                PM_PRINT("Compiling source files...\n");
                                if(!cc.compileStageConfig(args[1].c_str(), &outputFiles, false, false)) {
                                        PM_PRINT("Stage, Done. %s complete.\n", args[1].c_str());
                                }
                                else {
                                        PM_PRINT_ERROR("Failure in CompilerInstance [%p].\n", &cc);
                                        continue;
                                }

                                PM_PRINT("done. Linking output...\n");
                                linker.linkFiles(outputFiles, args[2].c_str());
                                PM_PRINT("Program successfully compiled and linked; output executable = [%s].\n", args[2].c_str());

                                continue;
                        }
                        if(args[0] == "debug") {
                                if(!inHeadlessMode) {
                                        if(debuggingActiveProcess) {
                                                PM_PRINT_ERROR("Target currently in active debugging session; unable to start.\n");
                                                continue;
                                        }

                                        if(args.size() > 1) {
                                                testTarget = args[1];
                                        }
                                        g_access.lock();

                                        debuggingActiveProcess = true;
                                        PM_PRINT("Now debugging target [%s].\n", testTarget.c_str());

                                        g_access.unlock();
                                        continue;
                                }
                                else {
                                        PM_PRINT_ERROR("Currently running in headless mode; No target.\n");
                                }
                        }
                        if(args[0] == "target") {
                                if(inHeadlessMode) {
                                        PM_PRINT_ERROR("Currently running in headless mode; No target.\n");
                                        continue;
                                }

                                if(args.size() < 2) {
                                        PM_PRINT_ERROR("No target specified.\n");
                                        continue;
                                }


                                configTestingTarget = args[1];
                                PM_PRINT("Set debug target config to [%s].\n", configTestingTarget.c_str());
                                continue;

                        }

                        if(args[0] == "test") {
                                if(debuggingActiveProcess) {
                                        PM_PRINT_ERROR("Target currently in active debugging session; unable to start.\n");
                                        continue;
                                }
                                nthp::script::CompilerInstance cc;
                                nthp::script::LinkerInstance linker;
                                std::vector<std::string> outputFiles;

                                PM_PRINT("Compiling source files...\n");
                                if(!cc.compileStageConfig(configTestingTarget.c_str(), &outputFiles, false, false)) {
                                        PM_PRINT("Stage, Done. %s complete.\n", configTestingTarget.c_str());
                                }
                                else {
                                        PM_PRINT_ERROR("Failure in CompilerInstance [%p].\n", &symbolData);
                                        continue;
                                }

                                PM_PRINT("done. Linking output...\n");
                                
                                std::string session_output = "m_" + configTestingTarget + ".p";
                                linker.linkFiles(outputFiles, session_output.c_str());
                                PM_PRINT("Program successfully compiled and linked; output executable = [%s].\n", session_output.c_str());


                                if(symbolData.compileStageConfig(configTestingTarget.c_str(), NULL, false, true)) {
                                        PRINT_COMPILER_WARNING("Failed to import symbols from config. Try again with \"import\" or make due without.\n");
                                }

                                g_access.lock();

                                testTarget = session_output;
                                debuggingActiveProcess = true;
                                PM_PRINT("Now debugging target [%s].\n", session_output.c_str());

                                g_access.unlock();

                                continue;
                        }


                        if(args[0] == "help") {
                                help_output();
                                continue;
                        }

                        if(args[0] == "stop") {
                                if(debuggingActiveProcess) {
                                        PM_PRINT("Stopping active debug session.\n");
                                        g_access.lock();

                                                nthp::core.stop();
                                                debuggingActiveProcess = false;
                                                suspendExecution = false;

                                        g_access.unlock();
                                }
                                else {
                                        PM_PRINT("Not in an active debug session.\n");
                                }
                                continue;

                        } 


                        if(args[0] == "gt") {
                                if(args.size() < 4) {
                                        PM_PRINT_ERROR("Invalid command argument. (gt [-flag] paletteFile imageFile outputFile)\n");
                                        continue;
                                }

                                int offset = 0;
                                bool compressOutput = false;
                                if(args[1] == "-c") {
                                        offset = 1;
                                        compressOutput = true;
                                }
                                nthp::texture::Palette tempPal;
                                if(tempPal.importPaletteFromFile(args[1 + offset].c_str())) {
                                        PM_PRINT_ERROR("Unable to import palette [%s].\n", args[1 + offset].c_str());
                                        continue;
                                }
                                PM_PRINT("Generating texture from file [%s] with palette [%s]...\n", args[2 + offset].c_str(), args[1 + offset].c_str());
                                PM_PRINT("Calculating color approximations (this may take a while)...\n");
                                if(nthp::texture::tools::generateSoftwareTextureFromImage(args[2 + offset].c_str(), &tempPal, args[3 + offset].c_str())) {
                                        PM_PRINT_ERROR("Failed to generate new softwareTexture.\n");
                                        continue;
                                }

                                if(compressOutput) {
                                        PM_PRINT("compressing...\n");
                                        std::string newFileName = args[3 + offset] + ".cst";
                                        nthp::texture::compression::compressSoftwareTextureFile(args[3 + offset].c_str(), newFileName.c_str());
                                }
                                PM_PRINT("done.\n");
                                continue;
                        }

                        if(args[0] == "ct") {
                                if(args.size() < 3) {
                                        PM_PRINT_ERROR("Invalid command argument. (ct textureFile outputFile)\n");
                                        continue;
                                }
                                PM_PRINT("Compressing texture [%s]...\n", args[1].c_str());
                                if(nthp::texture::compression::compressSoftwareTextureFile(args[1].c_str(), args[2].c_str())) {
                                        PM_PRINT_ERROR("Failed to compress softwareTexture [%s].\n", args[1].c_str());
                                        continue;
                                } 
                                PM_PRINT("Done.\n");
                                continue;
                        }



                        if(debuggingActiveProcess) {
                                if(args[0] == "break" || args[0] == "b") {
                                        g_access.lock();

                                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::BREAK;
                                        suspendExecution = true;
                                        PM_PRINT("Breakpoint read at instruction [%zu]; HEAD at [%zu], waiting for continue.\n", mainRuntime.data.currentNode, mainRuntime.data.currentNode);

                                        g_access.unlock();
                                        continue;

                                }
                                if(args[0] == "continue" || args[0] == "c") {
                                        g_access.lock();

                                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::CONTINUE;
                                        suspendExecution = false;
                                        PM_PRINT("Continuing from instruction [%zu]; HEAD at [%zu].\n", mainRuntime.data.currentNode, mainRuntime.data.currentNode);

                                        g_access.unlock();
                                        continue;

                                }


                                // Compiles a stage/source file and saves compiler definitions.
                                // i.e. reads symbols for debugging. Note that a different stage file can be used for symbols
                                // than the current debug target. Why you'd want to do that I have no idea.
                                if(args[0] == "import") {
                                        if(args.size() < 2) { PM_PRINT("Invalid arguments. syn; import sourcefile/stageconfig"); continue; }
                                        
                                        if(symbolData.compileStageConfig(args[1].c_str(), NULL, false, true)) {
                                                PM_PRINT_ERROR("Failed to import symbols from file [%s].\n", args[1].c_str());
                                                continue;
                                        }
                                        PM_PRINT("Imported [%zu] symbols from stage file [%s].\n", symbolData.globalList.size() + symbolData.macroList.size() + symbolData.constantList.size(), args[1].c_str());
                                        
                                        continue;
                                }

                                if(args[0] == "jump" || args[0] == "j") {
                                        if(args.size() < 2) {
                                                PM_PRINT_ERROR("jump failed; no jump location.\n");
                                                continue;
                                        }
                                        g_access.lock();

                                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::JUMP_TO;
                                        try {
                                                nthp::script::debug::debugInstructionCall.y = std::stoi(args[1]);
                                        }
                                        catch(std::invalid_argument) {
                                                PM_PRINT_ERROR("JUMP instruction takes a valid script instruction index as target.\n");
                                                nthp::script::debug::debugInstructionCall.x = -1;

                                                g_access.unlock();
                                                continue;
                                        }

                                        PM_PRINT("Continuing from instruction [%d]; HEAD at [%d].\n", std::stoi(args[1]), std::stoi(args[1]));
                                        g_access.unlock();
                                        continue;
                                }

                                if(args[0] == "step" || args[0] == "s") {
                                        if(!suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to step through.\n");
                                                continue;
                                        }

                                        g_access.lock();
                                        

                                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::STEP;
                                        PM_PRINT("Stepping to next instruction [%zu], [%zu] -> [%zu]\n", mainRuntime.data.currentNode + 1, mainRuntime.data.currentNode, mainRuntime.data.currentNode + 1);

                                        g_access.unlock();
                                        continue;

                                }
                                if(args[0] == "getvar" || args[0] == "gv") {
                                        if(!suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to read memory.\n");
                                                continue;
                                        }
                                        PM_PRINT("GLOBAL List :.\n[index, [>symbol] = [value]]\n");
                                        g_access.lock();

                                        bool printSymbols = false;
                                        size_t index;
                                        if(symbolData.globalList.size() > 0) { 
                                                printSymbols = true;
                                        }

                                        for(size_t i = 0; i < mainRuntime.data.globalMemBudget; ++i) {
                                                index = i;
                                                std::cout << "\t[" << i; 
                                                
                                                if(printSymbols) { std::cout << ", [>" << symbolData.globalList[i].varName; index = symbolData.globalList[i].relativeIndex; }
                                                std::cout << "] " << mainRuntime.data.globalVarSet + index << "; = [" << nthp::fixedToDouble(mainRuntime.data.globalVarSet[index]) << "] ]\n";
                                        }

                                        g_access.unlock();
                                        continue;
                                }

                                if(args[0] == "setvar" || args[0] == "sv") {
                                        if(!suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to write memory.\n");
                                                continue;
                                        }
                                        if(args.size() < 3) {
                                                PM_PRINT_ERROR("Invalid Argument. syn; setvar >symbol/index value");
                                                continue;
                                        }
                                        bool isIndex = false;
                                        if(args[1][0] != '>')
                                                isIndex = true;
                                        g_access.lock();
                                        
                                        try {
                                                if(isIndex) {
                                                        mainRuntime.data.globalVarSet[std::stoul(args[1])] = nthp::doubleToFixed(std::stod(args[2]));
                                                }
                                                else {
                                                        std::string reference = args[1];
                                                        reference.erase(reference.begin());
                                                        for(size_t i = 0; i < symbolData.globalList.size(); ++i) {
                                                                if(reference == symbolData.globalList[i].varName) {
                                                                        mainRuntime.data.globalVarSet[symbolData.globalList[i].relativeIndex] = nthp::doubleToFixed(std::stod(args[2]));
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                        catch(std::invalid_argument x) {
                                                PM_PRINT("Invalid argument numeral.\n");
                                                continue;
                                        }

                                        PM_PRINT("GLOBAL write success.\n");
                                        
                                        g_access.unlock();
                                        continue;
                                }
                                if(args[0] == "getblock" || args[0] == "gb") {
                                        if(!suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to access block memory.\n");
                                                continue;
                                        }

                                        if(args.size() < 2) {
                                                g_access.lock();

                                                PM_PRINT("List of Allocated Block data:\nChoose block \"gb [blockID]\"::\n");
                                                uint8_t b = 0;
                                                for(size_t i = 0; i < mainRuntime.data.blockDataSize; ++i) {
                                                        b = mainRuntime.data.blockData[i].isFree;
                                                        PM_PRINT("ID: %zu at [%p]. Contains [%zu] address space (Vacancy:%d).\n", i, mainRuntime.data.blockData[i].data, mainRuntime.data.blockData[i].size, b);
                                                }

                                                g_access.unlock();
                                                continue;
                                        }
                                        size_t index = 0;
                                        try {
                                                index = std::stoi(args[1]);
                                        }
                                        catch(std::invalid_argument) {
                                                PM_PRINT_ERROR("Invalid Argument; invalid blockID\n");
                                                continue;
                                        }

                                        g_access.lock();

                                        if(index > mainRuntime.data.blockDataSize) {
                                                PM_PRINT_ERROR("Invalid Argument; invalid blockID\n");
                                                g_access.unlock();
                                                continue;
                                        }
                                        PM_PRINT("Reading Memory from block %zu [%p]...\n", index, mainRuntime.data.blockData[index].data);
                                        for(size_t i = 0; i < mainRuntime.data.blockData[index].size; ++i) {
                                                PM_PRINT("[%04zX] = %lf,\n", i, nthp::fixedToDouble(mainRuntime.data.blockData[index].data[i]));
                                        }
                                        PM_PRINT("\tRead %zu entries.\n", mainRuntime.data.blockData[index].size);

                                        g_access.unlock();
                                        continue;

                                }

                                if(args[0] == "setblock" || args[0] == "sb") {
                                        if(!suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to access block memory.\n");
                                                continue;
                                        }

                                        if(args.size() < 4) {
                                                PM_PRINT_ERROR("Please specify blockID and address.\n\"sb [blockID] [address] [newValue]\" \n");
                                                continue;
                                        }

                                        unsigned int block = std::stoul(args[1], NULL, 0);
                                        unsigned int address = std::stoul(args[2], NULL, 0);
                                        nthp::script::stdVarWidth value = nthp::doubleToFixed(std::stod(args[3]));

                                        g_access.lock();

                                        if(block < mainRuntime.data.blockDataSize) {
                                                if(address < mainRuntime.data.blockData[block].size) {
                                                        mainRuntime.data.blockData[block].data[address] = value;
                                                        PM_PRINT("Write success; ID: %d at [%p], address %d; = %lf\n",block, mainRuntime.data.blockData + block, address, nthp::fixedToDouble(value));
                                                        
                                                        g_access.unlock();
                                                        continue;
                                                }
                                        }

                                        PM_PRINT_ERROR("Failure; ID or address out of bounds.\n");
                                        g_access.unlock();

                                        continue;
                                }

                        }


		} // if(input != "")


	} // while(isRunning)
	



        return 0;

}
