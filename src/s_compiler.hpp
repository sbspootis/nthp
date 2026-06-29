#pragma once

#include "s_instructions.hpp"

#ifdef DEBUG 

        extern void     PRINT_COMPILER                  (const char* format, ...);
        extern void     PRINT_COMPILER_ERROR            (const char* format, ...);
        extern void     PRINT_COMPILER_WARNING          (const char* format, ...);
        extern void     PRINT_COMPILER_DEPEND_ERROR     (const char* format, ...);

        #define         NOVERB_PRINT_COMPILER(...)      fprintf(NTHP_debug_output, __VA_ARGS__)

#else
        #define         PRINT_COMPILER(...)
        #define         PRINT_COMPILER_ERROR(...)
        #define         PRINT_COMPILER_WARNING(...)
        #define         NOVERB_PRINT_COMPILER(...)
        #define         PRINT_COMPILER_DEPEND_ERROR(...)
#endif




namespace nthp { 
        namespace script {

                

                class CompilerInstance {
                public:



                struct GLOBAL_DEF {
                        std::string varName;
                        uint32_t relativeIndex;

                        bool isPrivate;
                        std::string definedIn;

                        bool isStruct;
                        bool isFixed;
                        size_t structID;
                };

                struct CONSTEVAL_DEF {
                        std::string name;
                        std::string original_expression;
                        nthp::script::instructions::stdRef evaluation;

                        void writeToFile(std::fstream& symbolFile) {
                                symbolFile << "CONSTEVAL " << name << " " << original_expression << '\n';
                        }
                };

                // Compiler-Only; Represents a macro-like substitution.
                struct CONST_DEF {
                        std::string constName;
                        std::string value;

                        void writeToFile(std::fstream& symbolFile) {
                                std::string name = constName;
                                constName.erase(constName.begin());
                                symbolFile << "CONST " << name << " " << value << '\n';
                        }
                };

                // Compiler-Only; Represents a series of macro-like substitutions.
                struct MACRO_DEF {
                        std::string macroName;
                        std::vector<std::string> macroData;

                        void writeToFile(std::fstream& symbolFile) {
                                std::string newName = macroName;
                                newName.erase(newName.begin());

                                symbolFile << "MACRO " << newName << " { ";
                                for(size_t i = 0; i < macroData.size(); ++i) symbolFile << macroData[i] << ' ';

                                symbolFile << "}\n";
                        }
                };


                struct GOTO_DEF {
                        size_t goto_position;
                        uint32_t points_to;
                };

                struct LABEL_DEF {
                        uint32_t label_position;
                        uint32_t ID;
                };

                struct STR_DEF {
                        std::string name;
                        uint32_t objectPosition;
                        uint8_t length;
                };

                // Structure for declaring complex variables (STRUCT). Defines a VAR (PRIVATE or otherwise) structure map,
                // allowing constant offsets from ptr_descriptors to be written to without INDEX, NEXT, or PREV. The structure's
                // origin is the set address value of the assigned VARs ptr_descriptor. The exact size (or multiples) of a STRUCT def
                // can be allocted with NEW (NEW [structname] [count] [&targetToPopulate]). Already existing VARs can be assigned
                // a struct type with ASSIGN (ASSIGN [structname] [target]); this does not change its value. Note that no prefix is used when referencing the VAR;
                // this means only defined VARs (GLOBAL list) can be assigned as structure types. ASSIGN and UNASSIGN are NOT instructions;
                // no readable script data is generated from their use.
                struct STRUCT_DEF {
                        std::string name;
                        std::vector<std::string> members;

                        void writeToFile(std::fstream& symbolFile) {
                                symbolFile << "STRUCT " << name << " { ";
                                for(size_t i = 0; i < members.size(); ++i) symbolFile << members[i] << ' ';

                                symbolFile << "}\n";
                        }
                };

                // Contrary to how it may seem based on the name, a FUNC is just a cross-script label that's evaluated by the
                // LINKER, not the compiler. FUNC_START instructions are placed before a function, and the order in which they appear in
                // the linked executable is how they are defined. FUNC_CALL instructions (with "%funcname") will match the named function, provided
                // the FUNC compiler symbol is made available in a different source file (either by BUILD_SYSTEM or IMPORT). FUNCs cannot be referenced OOO.
                struct FUNC_DEF {
                        std::string name;
                        uint32_t func_start;

                        void writeToFile(std::fstream& symbolFile) {
                                symbolFile << "FUNC " << name << '\n';
                        }
                };

                // Acts as a function pointer list; compiler tracks different lists here and substitutes if the offset is constant.
                // Otherwise, FUNC_LIST nodes store  
                struct FLIST_DEF {
                        std::string listName;
                        std::vector<uint32_t> funcIDList;

                        void writeToFile(std::fstream& symbolFile, std::vector<FUNC_DEF>& fList) {
                                symbolFile << "FUNC_LIST " << listName << " { ";
                                for(auto i = 0; i < funcIDList.size(); ++i) {
                                        symbolFile << fList[funcIDList[i]].name << ' ';
                                }
                                symbolFile << "}\n";
                        }
                };


                // Takes source script as input, writes compiled script to output.
                // output file can be NULL, where the compiler won't write anything.
                int compileSourceFile(const char* inputFile, const char* outputFile, bool buildSystemContext, uint8_t executionFlags, const bool ignoreInstructionData, bool createSymbolFile);
                
                // Writes stored Node data to the target output file.
                // Is called by 'compileSourceFile' if the 'outputfile' is non-NULL.
                int exportToFile(const char* outputFile, std::vector<nthp::script::Node>* nodeList, bool buildSystemContext);

                int compileStageConfig(const char* stageConfigFile, std::vector<std::string>* targetFiles, bool forceBuild, const bool ignoreInstructionData);

                // Compiles a single translation unit as a T_MODULE and creates a symbol file (.sym) for it. Allows pre-compiled code to be imported
                // to other projects without rebuilding all the source code (using MODULE in a build system).
                int buildModule(const char* source);

                std::vector<nthp::script::CompilerInstance::CONST_DEF>  constantList;
                std::vector<nthp::script::CompilerInstance::CONSTEVAL_DEF> constevalList;
                std::vector<nthp::script::CompilerInstance::MACRO_DEF>  macroList;
                std::vector<nthp::script::CompilerInstance::GLOBAL_DEF>    globalList;
                std::vector<nthp::script::CompilerInstance::STR_DEF> strList;
                std::vector<nthp::script::CompilerInstance::FUNC_DEF> funcList;
                std::vector<nthp::script::CompilerInstance::FLIST_DEF> flists;
                std::vector<nthp::script::CompilerInstance::STRUCT_DEF> structList;

                static inline void undefConstant(const char* constName, std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList) {
                        size_t i = 0;
                        for(; i < constantList.size(); ++i) {
                                if(constName == constantList[i].constName) {
                                        constantList.erase(constantList.begin() + i);
                                        break;
                                }
                        }
                }
                static inline void undefConstant(size_t index, std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList) {
                        constantList.erase(constantList.begin() + index);
                }
                static inline void portable_evalConst(std::string& expression, std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList) {
                        for(size_t i = 0; i < constantList.size(); ++i) {
                                if(expression == constantList[i].constName) {
                                        expression = constantList[i].value;
                                        break;
                                }
                        }
                }

                const std::string getStageOutputTarget() { return stageOutputTarget; }

                // Up to 8 bits can be used for execution data.
                        typedef enum {
                                T_NONE = 0,     // #0
                                T_INIT,         // #1
                                T_TICK,         // #2
                                T_EXIT,         // #3
                                T_HIDDEN,       // #4
                                T_MODULE        // #5
                        } TriggerBits;

                static inline uint8_t getScriptTriggerFlags(const nthp::script::Node& node) {
                        return *(uint8_t*)(node.access.data + (sizeof(uint32_t)));
                }
                static inline uint32_t getScriptGlobalRequest(const nthp::script::Node& node) {
                        return *(uint32_t*)(node.access.data);
                }


                inline void cleanSymbolData() {
                        globalList.clear(); 
                        macroList.clear();
                        constantList.clear();
                        labelList.clear();
                        gotoList.clear();
                        strList.clear();
                        structList.clear();
                        funcList.clear();
                        flists.clear();
                }

                void clean() {
                        cleanSymbolData();
                        nthp::script::cleanNodeSet(nodeList);
                }

                ~CompilerInstance();
                private:
                        std::string stageOutputTarget;


                        // Labels and Goto's are matched post-compilation.
                        std::vector<nthp::script::CompilerInstance::LABEL_DEF>  labelList;
                        std::vector<nthp::script::CompilerInstance::GOTO_DEF>   gotoList;

                        std::vector<nthp::script::Node>                         nodeList;


                        inline void addGlobalDef(const char* name, const char* definedInFile) {
                                GLOBAL_DEF def;
                                def.relativeIndex = globalList.size();
                                def.varName = name;

                                def.isPrivate = false;
                                def.definedIn = definedInFile;

                                def.isStruct = false;
                                def.isFixed = false;

                                globalList.push_back(def);
                        }
                        inline void addPrivateGlobalDef(const char* name, const char* definedInFile) {
                                GLOBAL_DEF def;
                                def.relativeIndex = globalList.size();
                                def.varName = name;

                                def.isPrivate = true;
                                def.definedIn = definedInFile;

                                def.isStruct = false;
                                def.isFixed = false;

                                globalList.push_back(def);
                        }
                
                };

        }
}