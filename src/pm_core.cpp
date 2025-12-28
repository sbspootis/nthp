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

                        // Init phase.
                        PRINT_DEBUG("Beginning INIT phase...\n");
               
                    

                        if(mainRuntime.execInit()) {
                                g_access.lock();

                                mainRuntime.clean();
                                nthp::core.cleanup();
                                symbolData.clean();

                                debuggingActiveProcess = false;
                                nthp::script::debug::suspendExecution = false;

                                g_access.unlock();
                                
                                return 1;
                        }
                        g_access.lock();

                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::DEBUG_CALLS::BREAK;
                        nthp::script::debug::suspendExecution = true;
                        PM_PRINT("Ready. Waiting for continue (c)...\n");

                        g_access.unlock();

                        
                        while((nthp::core.isRunning()) && debuggingActiveProcess) {
                                frameStart = SDL_GetTicks();
                                
                                mainRuntime.handleEvents();


                                
                                // Tick phase.
                                if(mainRuntime.execTick()) {
                                        g_access.lock();

                                        mainRuntime.clean();
                                        nthp::core.cleanup();
                                        symbolData.clean();

                                        debuggingActiveProcess = false;
                                        nthp::script::debug::suspendExecution = false;

                                        g_access.unlock();

                                        return 1;
                                }

                                deltaTime = nthp::intToFixed(SDL_GetTicks() - frameStart);
                        
                                if(nthp::deltaTime < nthp::frameDelay) {
                                        SDL_Delay(nthp::fixedToInt(nthp::frameDelay - nthp::deltaTime));
                                        nthp::deltaTime = nthp::getFixedInteger(nthp::frameDelay);
                                }
                                
                                g_access.lock();
                                        mainRuntime.data.blockData[0].data[nthp::script::predefined_globals::DELTATIME_GLOBAL_INDEX] = nthp::deltaTime;
                                g_access.unlock();
                        }

                        // Exit Phase
                        PRINT_DEBUG("Beginning EXIT phase...\n");

                        
                        if(mainRuntime.execExit()) {
                                g_access.lock();

                                mainRuntime.clean();
                                nthp::core.cleanup();
                                symbolData.clean();

                                debuggingActiveProcess = false;
                                nthp::script::debug::suspendExecution = false;

                                g_access.unlock();
                        }

                        
                        debuggingActiveProcess = false;
                        nthp::script::debug::suspendExecution = false;
                        break;
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


        std::string debugOutput;
        FILE* debug_fd = stdout;

	if(argv > 1) {
                if(std::string(argc[1]) == "-h") {
                        inHeadlessMode = true;
                        std::thread debuggerThread(headless_runtime);

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

        std::thread debuggerThread(headless_runtime);


	
	NTHP_GEN_DEBUG_INIT(debug_fd);


        // Checks every frame if the debugger requests a session.
        do {
                std::this_thread::sleep_for(std::chrono::milliseconds(nthp::fixedToInt(nthp::frameDelay)));
                
                if(debuggingActiveProcess) {

                                               

                        int ret = nthp::debuggerBehaviour(testTarget, debug_fd);
                        if(ret) {
                                PM_PRINT_ERROR("Critical failure in debugger; return code %d\n", ret);
                                debuggingActiveProcess = false;
                        }
                        else {
                                PM_PRINT("\nCompleted debugging session without critical errors.\n");
                                debuggingActiveProcess = false;
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

typedef enum {
        STD,    // 0
        PTR,    // 1
        STR     // 2
} MEM_DISPLAY_FORMAT;


int headless_runtime() {
        PM_PRINT("Pm.exe;\nNTHP Game Engine project manager v." NTHP_VERSION "\nType 'help' for instructions.\n\n");
        if(inHeadlessMode) PM_PRINT("Headless mode; all graphics and audio disabled.\n");
	std::vector<std::string> args;
	std::mutex g_access;

	std::string input, arg, configTestingTarget = "";
	bool isRunning = true;

        int displayFormat = MEM_DISPLAY_FORMAT::STD;
        bool executingPMScript = false;
        std::fstream scriptFile;



	while(isRunning) {
L_BEGIN:

		args.clear();
		std::cin.clear();

                if(!executingPMScript) {
                        if(debuggingActiveProcess) std::cout << "debug> ";
                        else std::cout << "> ";

                        std::getline(std::cin, input);
                }
                else {
                        if(!scriptFile.eof()) {
                                std::getline(scriptFile, input);
                        }
                        else {
                                executingPMScript = false;
                                scriptFile.close();

                                PM_PRINT("Script complete.\n");
                                continue;
                        }
                }

		if(input != "") {
			{ // Separates all input symbols into the args vector.
                                
				std::istringstream inputStream(input);
				while(std::getline(inputStream, arg, ' '))
					args.push_back(arg);

			}


                        if(executingPMScript) { 
                                if(!(args[0] == "rem"))
                                        PM_PRINT("\texec. %s\n", input.c_str());
                                else
                                        continue;
                        }

                        if(args[0] == "load") {
                                if(args.size() < 2) {
                                        PM_PRINT_ERROR("Need target pm script file to execute. (load scriptFile)\n");
                                        continue;
                                }

                                scriptFile.open(args[1], std::ios::in);
                                if(scriptFile.fail()) {
                                        PM_PRINT_ERROR("Unable to open script file [%s].\n", args[1].c_str());
                                        scriptFile.clear();

                                        continue;
                                }

                                executingPMScript = true;
                                PM_PRINT("Executing script [%s]...\n", args[1].c_str());
                                continue;
                        }

                        if(args[0] == "rem") {
                                continue;
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
                                if(args.size() < 2) {
                                        PM_PRINT_ERROR("Linker requires at least 1 target file.\n");
                                        continue;
                                }
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

                                continue;
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
                                        PM_PRINT_ERROR("Target in active debugging session; unable to start.\n");
                                        continue;
                                }
                                if(inHeadlessMode) {
                                        PM_PRINT_ERROR("Currently running in headless mode; No target.\n");
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
                                                nthp::script::debug::debugInstructionCall.x = -1;
                                                nthp::script::debug::suspendExecution = false;

                                        g_access.unlock();
                                }
                                else {
                                        PM_PRINT("Not in active debug session.\n");
                                }
                                continue;

                        } 


                        if(args[0] == "gt") {
                                if(args.size() < 3) {
                                        PM_PRINT_ERROR("Invalid command argument. (gt paletteFile imageFileA imageFileB imageFileC ...)\n");
                                        continue;
                                }

                                nthp::texture::Palette tempPal;
                                if(tempPal.importPaletteFromFile(args[1].c_str())) {
                                        PM_PRINT_ERROR("Unable to import palette [%s].\n", args[1].c_str());
                                        continue;
                                }
                                std::string filename;

                                for(size_t i = 2; i < args.size(); ++i) {
                                        {
                                                filename = args[i];
                                                auto pos = filename.rfind('.');
                                                if(pos != std::string::npos) 
                                                        filename.erase(filename.begin()+pos, filename.end());
                                                
                                                filename += ".st";
                                        }

                                        PM_PRINT("Generating texture from file [%s] with palette [%s]...\n", args[i].c_str(), args[1].c_str());
                                        PM_PRINT("Calculating color approximations (this may take a while)...\n");
                                        if(nthp::texture::tools::generateSoftwareTextureFromImage(args[i].c_str(), &tempPal, filename.c_str())) {
                                                PM_PRINT_ERROR("Failed to generate new softwareTexture with file [%s].\n", args[i].c_str());
                                                goto L_BEGIN;   // hell yeah.
                                        }
                                        PM_PRINT("done.\n");
                                }
                                continue;
                        }

                        if(args[0] == "ct") {
                               if(args.size() < 2) {
                                        PM_PRINT_ERROR("Invalid command argument. (ct textureFileA textureFileB ...)\n");
                                        continue;
                                }

                                std::string filename;
                                
                                for(size_t i = 1; i < args.size(); ++i) {
                                        {
                                                filename = args[i];
                                                auto pos = filename.rfind('.');
                                                if(pos != std::string::npos) 
                                                        filename.erase(filename.begin()+pos, filename.end());
                                                
                                                filename += ".cst";
                                        }
                                        PM_PRINT("Compressing texture [%s]...\n", args[i].c_str());
                                        if(nthp::texture::compression::compressSoftwareTextureFile(args[i].c_str(), filename.c_str())) {
                                                PM_PRINT_ERROR("Failed to compress softwareTexture file [%s].\n", args[i]);
                                                goto L_BEGIN;
                                        }
                                        PM_PRINT("done.\n");
                                        
                                }
                                continue;

                        }
                        
                   
                        // Joins an indefinite series of ST files into a single file; joins the output of the last operation with the next target
                        // until every target is ordered into a sheet.
                        if(args[0] == "jt") {
                                if(args.size() < 5) {
                                        PM_PRINT_ERROR("Invalid command argument. (js w/h textureA textureB textureC ... outputTexture)\n");
                                        continue;
                                }

                                bool method;        
                                nthp::texture::SoftwareTexture::STdata output;
                                nthp::texture::SoftwareTexture::STdata a;
                                nthp::texture::SoftwareTexture::STdata b;

                                if(args[1] == "w") {
                                        method = nthp::texture::tools::JOIN_WIDTH;
                                        PM_PRINT("Attempting to join texture series by width; %zu targets...\n", args.size() - 3);
                                }
                                else {
                                        method = nthp::texture::tools::JOIN_HEIGHT;
                                        PM_PRINT("Attempting to join texture series by height; %zu targets...\n", args.size() - 3);
                                }

                                a = nthp::texture::tools::readTextureData(args[2].c_str());
                                b = nthp::texture::tools::readTextureData(args[3].c_str());

                                if(a.header.signature != nthp::texture::SoftwareTexture::STheaderSignature || b.header.signature != nthp::texture::SoftwareTexture::STheaderSignature) {
                                        PM_PRINT_ERROR("Failed to join textures.\n");
                                        nthp::texture::tools::destroySTdata(&a);
                                        nthp::texture::tools::destroySTdata(&b);

                                        continue; 
                                }

                                output = nthp::texture::tools::joinSoftwareTextures(a, b, method);
                                if(output.header.signature != nthp::texture::SoftwareTexture::STheaderSignature) {
                                        PM_PRINT_ERROR("Failed to join textures.\n");
                                        continue; 
                                }

                                nthp::texture::tools::destroySTdata(&a);
                                nthp::texture::tools::destroySTdata(&b);

                                for(size_t i = 4; i < args.size() - 1; ++i) {
                                        // TODO

                                        a = output;
                                        b = nthp::texture::tools::readTextureData(args[i].c_str());
                                        
                                        output = nthp::texture::tools::joinSoftwareTextures(a, b, method);
                                        if(b.header.signature != nthp::texture::SoftwareTexture::STheaderSignature || output.header.signature != nthp::texture::SoftwareTexture::STheaderSignature) {
                                                output.header.signature = 0;
                                                nthp::texture::tools::destroySTdata(&a);
                                                nthp::texture::tools::destroySTdata(&b);
                                                nthp::texture::tools::destroySTdata(&output);

                                                break;
                                        }

                                        nthp::texture::tools::destroySTdata(&a);
                                        nthp::texture::tools::destroySTdata(&b);
                                }

                                nthp::texture::tools::destroySTdata(&a);                // It is safe to call destroySTdata on a destroyed structure.
                                nthp::texture::tools::destroySTdata(&b);                // This is done in case only 2 textures are passed in the series.

                                if(nthp::texture::tools::writeTextureData(output, args[args.size() - 1].c_str())) {
                                        PM_PRINT_ERROR("Failed to join texture series.\n");
                                }
                                else {
                                        PM_PRINT("Successfully joined texture series into sheet @ file [%s].\n", args[args.size() - 1].c_str());
                                }
                                
                                nthp::texture::tools::destroySTdata(&output);
                                continue;
                        }

                        if(args[0] == "tcheck") {
                                if(args.size() < 2) {
                                        PM_PRINT_ERROR("Invalid command argument. (tcheck filename)\n");
                                        continue;
                                }

                                nthp::texture::SoftwareTexture::software_texture_header target;
                                std::fstream file(args[1], std::ios::in | std::ios::binary);

                                if(file.fail()) {
                                        PM_PRINT_ERROR("Unable to check texture file [%s]; file not found.\n", args[1].c_str());
                                        continue;
                                }

                                file.read((char*)&target, sizeof(target));
                                file.close();

                                switch(target.signature) {
                                case nthp::texture::SoftwareTexture::STheaderSignature:
                                        PM_PRINT("Uncompressed ST file [%s];\nx=%u y=%u\ndataSize=%zu\n", args[1].c_str(), target.x, target.y, target.x * target.y);
                                        break;

                                case nthp::texture::compression::CSTHeaderSignature:
                                        PM_PRINT("Compressed ST file [%s];\nx=%u y=%u\ndataSize=%zu\n", args[1].c_str(), target.x, target.y, target.x * target.y);
                                        break;

                                default:
                                        PM_PRINT_ERROR("Invalid ST texture file format.\n");
                                        continue;
                                }
                                continue;
                        }



                        if(debuggingActiveProcess) {
                                if(args[0] == "break" || args[0] == "b") {
                                        g_access.lock();

                                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::BREAK;
                                        nthp::script::debug::suspendExecution = true;
                                        PM_PRINT("Breakpoint read at instruction [%zu]; HEAD at [%zu], waiting for continue.\n", mainRuntime.data.currentNode, mainRuntime.data.currentNode);

                                        g_access.unlock();
                                        continue;

                                }
                                if(args[0] == "continue" || args[0] == "c") {
                                        g_access.lock();

                                        nthp::script::debug::debugInstructionCall.x = nthp::script::debug::CONTINUE;
                                        nthp::script::debug::suspendExecution = false;
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
                                        if(!nthp::script::debug::suspendExecution) {
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
                                        if(!nthp::script::debug::suspendExecution) {
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
                                                printf ("\t[%04zX (%zu), ", i, i);
                                                if(printSymbols) { std::cout << "o." << symbolData.globalList[i].definedIn << " [>" << symbolData.globalList[i].varName; index = symbolData.globalList[i].relativeIndex; }
                                                std::cout << "] " << mainRuntime.data.blockData[0].data + index << "; = [";
                                                switch(displayFormat) {
                                                        case MEM_DISPLAY_FORMAT::STD:
                                                                {
									std::cout << nthp::fixedToDouble(mainRuntime.data.blockData[0].data[index]) << "]\n";
                                                                	break;
                                                        	}
							case MEM_DISPLAY_FORMAT::PTR:
                                                                {
									const nthp::script::PtrDescriptor_st ptr = nthp::script::parsePtrDescriptor(mainRuntime.data.blockData[0].data[index]);
                                                                	std::cout << 'b' << ptr.block << 'a' << ptr.address << "]\n";
                                                                	break;
                                                        	}
							default:
                                                                {
									std::cout << nthp::fixedToDouble(mainRuntime.data.blockData[0].data[index]) << "]\n";
                                                                	break;
                                                		}
						}
                                                
                                        }

                                        g_access.unlock();
                                        continue;
                                }

                                if(args[0] == "setvar" || args[0] == "sv") {
                                        if(!nthp::script::debug::suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to write memory.\n");
                                                continue;
                                        }
                                        if(args.size() < 3) {
                                                PM_PRINT_ERROR("Invalid Argument. syn; setvar >symbol/index value\n");
                                                continue;
                                        }

                                        bool isIndex = false;
                                        unsigned long accessIndex;
                                        
                                        if(args[1][0] != '>') {
                                                isIndex = true;
                                                try {
                                                        accessIndex = std::stoul(args[1], NULL, 0);
                                                }
                                                catch(std::exception x) {
                                                        PM_PRINT_ERROR("Invalid argument numeral.\n");
                                                        continue;
                                                }
                                        }
                                        g_access.lock();
                                        bool found = false;

                                        try {
                                                if(isIndex) {
                                                        if(accessIndex >= mainRuntime.data.blockData[0].size) {
                                                                PM_PRINT_ERROR("Invalid GLOBAL index [%u].\n", accessIndex);

                                                                g_access.unlock();
                                                                continue;
                                                        }

                                                        mainRuntime.data.blockData[0].data[accessIndex] = nthp::doubleToFixed(std::stod(args[2]));
                                                        found = true;
                                                }
                                                else {
                                                        std::string reference = args[1];
                                                        
                                                        reference.erase(reference.begin());
                                                        for(size_t i = 0; i < symbolData.globalList.size(); ++i) {
                                                                if(reference == symbolData.globalList[i].varName) {
                                                                        mainRuntime.data.blockData[0].data[symbolData.globalList[i].relativeIndex] = nthp::doubleToFixed(std::stod(args[2]));
                                                                        found = true;
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                        catch(std::invalid_argument x) {
                                                PM_PRINT_ERROR("Invalid argument numeral.\n");
                                                continue;
                                        }

                                        if(found) PM_PRINT("GLOBAL write success; VAR [%s] = [%lf].\n", args[1].c_str(), nthp::fixedToDouble(nthp::doubleToFixed(std::stod(args[2]))));
                                        else PM_PRINT_ERROR("GLOBAL [%s] not found.\n", args[1].c_str());
                                        
                                        g_access.unlock();
                                        continue;
                                }
                                if(args[0] == "getblock" || args[0] == "gb") {
                                        if(!nthp::script::debug::suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to access block memory.\n");
                                                continue;
                                        }

                                        if(args.size() < 2) {
                                                g_access.lock();

                                                PM_PRINT("List of Allocated Block data:\nChoose block \"gb [blockID]\"::\n");
                                                uint8_t b = 0;
                                                for(size_t i = 0; i < mainRuntime.data.blockDataSize; ++i) {
                                                        b = mainRuntime.data.blockData[i].isFree;
                                                        PM_PRINT("ID: %zu(b%zu) at [%p]. Contains [%zu] address space (Vacancy:%d).\n", i, (i), mainRuntime.data.blockData[i].data, mainRuntime.data.blockData[i].size, b);
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

                                        if(index >= mainRuntime.data.blockDataSize) {
                                                PM_PRINT_ERROR("Invalid Argument; invalid blockID\n");
                                                g_access.unlock();
                                                continue;
                                        }
                                        PM_PRINT("Reading Memory from block %zu [%p]...\n", index, mainRuntime.data.blockData[index].data);
                                        bool carry_string = true;
                                        for(size_t i = 0; i < mainRuntime.data.blockData[index].size; ++i) {
                                                switch(displayFormat) {
                                                        case MEM_DISPLAY_FORMAT::STD:
                                                                PM_PRINT("[%04zX] = %lf,\n", i, nthp::fixedToDouble(mainRuntime.data.blockData[index].data[i]));
                                                        break;
                                                        case MEM_DISPLAY_FORMAT::PTR:
                                                        {
                                                                const nthp::script::PtrDescriptor_st ptr = nthp::script::parsePtrDescriptor(mainRuntime.data.blockData[index].data[i]);
                                                                PM_PRINT("[%04zX] = [b%da%d]\n", i, ptr.block, ptr.address);
                                                        }
                                                        break;
                                                        case MEM_DISPLAY_FORMAT::STR:
                                                        {
                                                                if(carry_string) { PM_PRINT("[%04zX] ", i); carry_string = false; }
                                                                for(size_t ch = 0; ch < sizeof(nthp::script::stdVarWidth); ++ch) {
                                                                        if(((char*)(&mainRuntime.data.blockData[index].data[i]))[ch] == '\0') { PM_PRINT("\n"); carry_string = true; }
                                                                        else PM_PRINT("%c", ((char*)(&mainRuntime.data.blockData[index].data[i]))[ch]);
                                                                }
                                                        }
                                                        break;

                                                }
                                        }
                                        PM_PRINT("\tRead %zu entries.\n", mainRuntime.data.blockData[index].size);

                                        g_access.unlock();
                                        continue;

                                }

                                if(args[0] == "setblock" || args[0] == "sb") {
                                        if(!nthp::script::debug::suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to access block memory.\n");
                                                continue;
                                        }

                                        if(args.size() < 4) {
                                                PM_PRINT_ERROR("Please specify blockID and address.\n\"sb [blockID] [address] [newValue]\" \n");
                                                continue;
                                        }
                                        unsigned int block;
                                        unsigned int address;
                                        nthp::script::stdVarWidth value;
                                        try {
                                                block = std::stoul(args[1], NULL, 0);
                                                address = std::stoul(args[2], NULL, 0);
                                                value = nthp::doubleToFixed(std::stod(args[3]));
                                        }
                                        catch(std::invalid_argument) {
                                                PM_PRINT_ERROR("Please specify blockID and address.\n\"sb [blockID] [address] [newValue]\" \n");
                                                continue;
                                        }
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

                                if(args[0] == "info") {
                                        if(!nthp::script::debug::suspendExecution) {
                                                PM_PRINT_ERROR("Process must be suspended (break, b) to get program info.\n");
                                                continue;
                                        }

                                        PM_PRINT("\nRuntime script @ [%p]:\nHEAD at [%zu], current script header @ [%zu]\nRunning for %dms\n\n", &mainRuntime, mainRuntime.data.currentNode, mainRuntime.data.currentScriptHeaderLocation, SDL_GetTicks());
                                        if(mainRuntime.data.stackPointer) {
                                              PM_PRINT("ReturnStack state:\n\n");
                                              for(size_t i = 0; i < mainRuntime.data.stackPointer; ++i) {
                                                for(size_t k = 0; k < i; ++k) PM_PRINT("   ");
                                                PM_PRINT("[%zu] Waiting for RETURN @ FUNC [%u]; Will return to [%zu] (H:%u).\n", i, (*(uint32_t*)(mainRuntime.data.nodeSet[mainRuntime.data.returnStack[i].sourceDestination - 1].access.data)) - 1, mainRuntime.data.returnStack[i].sourceDestination, mainRuntime.data.returnStack[i].sourceHeaderLocation);
                                              }
                                        }

                                        continue;
                                }

                                if(args[0] == "df") {
                                        if(args.size() < 2) {
                                                PM_PRINT_ERROR("No display format provided.\n");
                                                continue;
                                        }
                                        do {
                                                if(args[1] == "std" || args[1] == "stdRef") {
                                                        displayFormat = MEM_DISPLAY_FORMAT::STD;
                                                        PM_PRINT("Set memory display format to [stdRef] (decimal).\n");
                                                        break;
                                                }

                                                if(args[1] == "ptr" || args[1] == "ptrRef") {
                                                        displayFormat = MEM_DISPLAY_FORMAT::PTR;
                                                        PM_PRINT("Set memory display format to [ptrRef] (pointer descriptor).\n");
                                                        break;
                                                }

                                                if(args[1] == "str" || args[1] == "strRef") {
                                                        displayFormat = MEM_DISPLAY_FORMAT::STR;
                                                        PM_PRINT("Set memory display format to [strRef] (string).\n");
                                                        break;
                                                }

                                                PM_PRINT_ERROR("Invalid memory display format. (std, ptr, str)\n");
                                        }while (0);

                                        continue;
                                }

                        }
                        std::cout << "\"" << args[0] << "\", unknown command.\n";
                        continue; 
                
		} // if(input != "")


	} // while(isRunning)
	



        return 0;

}
