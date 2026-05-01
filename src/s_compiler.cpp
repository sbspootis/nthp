#ifdef PM
        #ifndef DEBUG
	        #define DEBUG
        #endif

        #define SUPPRESS_DEBUG_OUTPUT
#endif



#include "s_compiler.hpp"
using namespace nthp::script::instructions;





// I love this. Absolutely terrible. I should've switched it to a structure reference long
// ago, but I never got around to it. Now it's too annoying to fix. 
#define DEFINE_COMPILATION_BEHAVIOUR(instruction) int instruction (std::vector<nthp::script::Node>& nodeList,\
                                                                        std::fstream& file,\
                                                                        std::string& fileRead,\
                                                                        std::string& currentFile,\
                                                                        std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList,\
                                                                        std::vector<nthp::script::CompilerInstance::CONSTEVAL_DEF>& constevalList,\
                                                                        std::vector<nthp::script::CompilerInstance::MACRO_DEF>& macroList,\
                                                                        std::vector<nthp::script::CompilerInstance::GLOBAL_DEF>& globalList,\
                                                                        std::vector<nthp::script::CompilerInstance::LABEL_DEF>& labelList,\
                                                                        std::vector<nthp::script::CompilerInstance::GOTO_DEF>& gotoList,\
                                                                        std::vector<nthp::script::CompilerInstance::STR_DEF>& strList,\
                                                                        std::vector<nthp::script::CompilerInstance::STRUCT_DEF>& structList,\
                                                                        std::vector<size_t>& ifList,\
                                                                        std::vector<size_t>& endList,\
                                                                        std::vector<size_t>& skipList,\
                                                                        size_t& currentMacroPosition,\
                                                                        size_t& targetMacro,\
                                                                        bool& evaluateMacro,\
                                                                        const bool& buildSystemContext,\
                                                                        uint8_t dynamicOffsetCounter = 0\
                                                                        )


const inline size_t __getCurrentNode(std::vector<nthp::script::Node>& nodeList) {
        if(nodeList.back().access.ID == nthp::script::instructions::ID::DATA) { return nodeList.size() - 2; }
        else { return nodeList.size() - 1; }
}
// Returns the position of the current node. 99% of the time, it's the last (or vector.back()), unless a data
// node is at the back, then the one before it is returned.
#define currentNode __getCurrentNode(nodeList)

// Adds node with corresponding instruction ID and allocates node memory.
#define ADD_NODE(instruction) nodeList.push_back(nthp::script::Node(GET_INSTRUCTION_ID(instruction), GET_INSTRUCTION_SIZE(instruction)));\
        if(GET_INSTRUCTION_SIZE(instruction) > 0) nodeList.back().access.data = (char*)malloc(GET_INSTRUCTION_SIZE(instruction));\
        else nodeList.back().access.data = nullptr

#define VECT_END(vect) vect.size() - 1

#define READ_FILE() (file >> fileRead)

bool skipInstructionCheck = false;




#ifdef DEBUG

void PRINT_COMPILER(const char* format, ...) {
        va_list ap;
	
	va_start(ap, format);

	fprintf(NTHP_debug_output, "[t %u] COMPILER: ", SDL_GetTicks());	
	vfprintf(NTHP_debug_output, format, ap);


	va_end(ap);
        fflush(NTHP_debug_output);
}

void PRINT_COMPILER_ERROR(const char* format, ...) {
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

void PRINT_COMPILER_WARNING(const char* format, ...) {
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

void PRINT_COMPILER_DEPEND_ERROR(const char* format, ...) {
        va_list ap;
	
	va_start(ap, format);
#ifdef PM
        va_list pmOutputList;
        va_copy(pmOutputList, ap);
#endif

	fprintf(NTHP_debug_output, "[t %u] DEPENDENCY ERROR: ", SDL_GetTicks());	
	vfprintf(NTHP_debug_output, format, ap);

#ifdef PM
        printf("[t %u] DEPENDENCY ERROR: ", SDL_GetTicks());
        vprintf(format, pmOutputList);

        va_end(pmOutputList);
#endif

	va_end(ap);
        fflush(NTHP_debug_output);
}

#endif


#define PRINT_NODEDATA() NOVERB_PRINT_COMPILER("[%zu] [%p] Node ID: %u ; s=%u d[%p]\n", currentNode, (nodeList.data() + (currentNode)), nodeList[currentNode].access.ID, nodeList[currentNode].access.size, nodeList[currentNode].access.data) 



int EvaluateConst(std::string& expression, std::vector<nthp::script::CompilerInstance::CONST_DEF>& list) {
        if(expression[0] == '#') {

                for(size_t i = 0; i < list.size(); ++i) {
                        if(expression == list[i].constName) {
                                expression = list[i].value;
                                PRINT_COMPILER("Substituted main expression to [%s].\n", list[i].value.c_str());
                                
                                return EvaluateConst(expression, list); // Allows recursive CONST evals.
                        }
                }

                PRINT_COMPILER_ERROR("Unable to evaluate CONST substitution [%s]; CONST not declared.\n", expression.c_str());
                return 1;
        }
        return 0;
}


void destroyArgumentConsts(std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList) {
        std::string search;
        for(size_t i = 0; i < constantList.size(); ++i) {
                search = constantList[i].constName;
                search.erase(search.begin() + 3, search.end()); // Keeps the first 3 characters.
                if(search == "#ar") {
                        nthp::script::CompilerInstance::undefConstant(i, constantList);
                        --i;
                }
        }

        PRINT_DEBUG("Purged temporary CONSTs.\n");
}





int EvaluateMacro(std::fstream& file, std::string& expression, std::vector<nthp::script::CompilerInstance::MACRO_DEF>& list, std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList, size_t& mp, size_t& targetMacro, bool& beginMacroEval) {
        if(expression[0] == '@') {
                auto initialPosition = file.tellg();

                for(size_t i = 0; i < list.size(); ++i) {
                        if(expression == list[i].macroName) {
                                targetMacro = i;

                                
                                (file >> expression);
                                nthp::script::CompilerInstance::portable_evalConst(expression, constantList);
                                if(expression == "(") {
                                        // Evaluate Arguments
                                        std::string argument_const;
                                        size_t argumentsFound = 0;
                                        do {
                                                (file >> expression);
                                                nthp::script::CompilerInstance::portable_evalConst(expression, constantList);
                                                if(expression != ")") {        
                                                        ++argumentsFound;
                                                        if(argumentsFound > 255) { // Was at 500, thought that was too big. When are you going to have more than 255 arguments in a function?
                                                                PRINT_COMPILER_ERROR("Macro Argument data is too large; no ARG_END ')' character found.\n");
                                                                return 1;
                                                        }

                                                        argument_const = ("#ar" + std::to_string(argumentsFound - 1));

                                                        constantList.push_back(nthp::script::CompilerInstance::CONST_DEF());
                                                        constantList[constantList.size() - 1].constName = argument_const;
                                                        constantList[constantList.size() - 1].value = expression;
                                                        PRINT_COMPILER("Detected argument name[%s] = [%s];\n", constantList[constantList.size() - 1].constName.c_str(), constantList[constantList.size() - 1].value.c_str());

                                                }
                                        }
                                        while(expression != ")");
                                        PRINT_COMPILER("Evaluated Arguments.\n");

                                }
                                else {
                                        file.seekg(initialPosition);
                                }
                                // TODO: Test this shit




                                expression = list[i].macroData[0];
                                mp = 0;
                                beginMacroEval = true;


                                PRINT_COMPILER("Beginning Expansion of Macro [%s]...\n", list[targetMacro].macroName.c_str());
                                
                                return 0;
                        }
                }
                
                PRINT_COMPILER_ERROR("Unable to evaluate MACRO substitution [%s]; MACRO not declared.\n", expression.c_str());
                return 1;
        }
       return 0;
}



// Generic for reading from source file. Checks for MACRO or CONST refs, and evaluates
// MACRO substitutions. Sets 'expression' to whatever the next valid symbol is; regardless if it's from a MACRO or the source file.
int EvaluateSymbol(std::fstream& file, std::string& expression, std::vector<nthp::script::CompilerInstance::CONST_DEF>& constList, std::vector<nthp::script::CompilerInstance::MACRO_DEF>& macroList, size_t& currentMacroPosition, size_t& targetMacro, bool& evaluatingMacro) {
        bool waitForCommentEnd = false;
        do {
                if(evaluatingMacro) {
                        ++currentMacroPosition;


                        if(currentMacroPosition == macroList[targetMacro].macroData.size()) {
                                evaluatingMacro = false;
                                NOVERB_PRINT_COMPILER("\tCompleted expansion of macro [%s].\n", macroList[targetMacro].macroName.c_str());
                                destroyArgumentConsts(constList);
                        }
                        else {
                                expression = macroList[targetMacro].macroData[currentMacroPosition];
                                if(EvaluateConst(expression, constList)) return 1;
                                

                                return 0;
                        }
                }

                (file) >> (expression);

                if(expression == "/") {
                        if(waitForCommentEnd) {
                                waitForCommentEnd = false;
                                continue;
                        }
                        else {
                                waitForCommentEnd = true;
                                continue;
                        }
                }

                if(waitForCommentEnd) continue;

                // Allows MACROs to be called with CONSTs, and makes sure (if the first argument is a CONST) that another eval is done.
                if(EvaluateConst(expression, constList)) return 1;
                if(EvaluateMacro(file, expression, macroList, constList, currentMacroPosition, targetMacro, evaluatingMacro)) return 1;
                if(EvaluateConst(expression, constList)) return 1;
                

                break;
                
        } while(true);

        return 0;
}


// Substitues a VAR reference or parses numeral references (for compatibility).
// Returning REF without the IS_VALID bit set assumes a failure.
// dynamicOffsetCounter should be the optional argument 'dynamicOffsetCounter' defined in every COMPILATION_BEHAVIOUR function. A dynamicOffsetCounter value of -1 means dynamic offsets are disabled.
nthp::script::instructions::stdRef EvaluateReference(std::string expression, std::vector<nthp::script::Node>& nodeList, std::vector<nthp::script::CompilerInstance::CONST_DEF>& constantList, std::vector<nthp::script::CompilerInstance::GLOBAL_DEF>& globalList, std::vector<nthp::script::CompilerInstance::CONSTEVAL_DEF>& constevalList, std::vector<nthp::script::CompilerInstance::STR_DEF>& strList, std::vector<nthp::script::CompilerInstance::STRUCT_DEF>& structList,std::string& currentFile, size_t* globalRefIndex, bool buildSystemContext, uint8_t* dynamicOffsetCounter, bool suppressFailure) {
        stdRef ref;
        ref.metadata = 0;
        ref.value = 0;
        ref.offset = 0;

        bool ptr_reference = false;
        bool deref_ptr = false;
        bool get_size = false;
        bool is_fixed_ref = false;
        bool dynamic_offset = false;
        bool isConsteval = false;


        // Binary write; constant value, not converted to fixed point.
        if(expression[0] == '?') {
                expression.erase(expression.begin());
                
                if(expression[0] == 'b') { // Write a constant pointer descriptor; seperate b#a#
                        expression.erase(expression.begin());

                        size_t a_pos = expression.find('a');
                        if(a_pos == std::string::npos) {
                                PRINT_COMPILER_ERROR("Invalid pointer descriptor; address not specified.\n");
                                return ref;
                        }

                        std::string address_value = expression;
                        address_value.erase(0, a_pos);
                        address_value.erase(address_value.begin());

                        expression.erase(expression.begin()+a_pos, expression.end());

                        nthp::script::PtrDescriptor_st ptr;
                        try {
                                ptr.block = std::stoi(expression);
                                ptr.address = std::stoi(address_value);
                        }
                        catch(std::invalid_argument) {
                                PRINT_COMPILER_ERROR("Invalid pointer descriptor; unable to parse\n");
                                return ref;
                        }

                        ref.value = nthp::script::constructPtrDescriptor(ptr.block, ptr.address);
                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);

                        PRINT_COMPILER("Constructed constant ptr_descriptor [b%da%d].\n", ptr.block, ptr.address);

                        return ref;
                }



                try {
                        ref.value = std::stol(expression, nullptr, 0);
                }
                catch(std::invalid_argument) {
                        PRINT_COMPILER_ERROR("Binary value invalid. (%s) invalid binary expression.\n", expression.c_str());
                        return ref;
                }

                PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);
                return ref;
        }

        if(expression[0] == '^') {
                isConsteval = true;
                expression.erase(expression.begin());
        }



        if(expression[0] == '>') {
                get_size = true;
                expression.erase(expression.begin());
        }


        // Prefixes are evaluated IN ORDER of ; Negation (-), ptr_dereference (*), ptr_reference (&), Globality (> or $)
        // NOTE: reference (&) PTR prefixes can evaluate as constants, dereferences (*) cannot.
        do {
                if(expression[0] == '~') {

                        expression.erase(expression.begin());
                        // Check for the string name in the node string list. If none,
                        // assume it is a ptr_descriptor reference to allocated memory storing a string.
                        for(size_t i = 0; i < strList.size(); ++i) {
                                if(expression == strList[i].name) {

                                        if(get_size) {  // Allows the use of the size character '>', followed by a static string to substitute the length
                                                        // of that string as a constant ref.
                                                ref.value = nthp::intToFixed(strList[i].length);
                                                ref.metadata = 0;
                                                ref.offset = 0;

                                                PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);
                                                return ref;
                                        }

                                        ref.value = nthp::intToFixed(strList[i].objectPosition);
                                        ref.metadata = 0;                       // Reset flags to ensure only STRING_PTR is set.
                                        ref.offset = strList[i].length;         // the ref.offset is equal to the string length.
                                                                                // This only applies with STRING node references. Text written
                                                                                // to a block by the input buffer contains no indication of size.
                                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_NODE_STRING_PTR);
                                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);
                                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_STRING);

                                        return ref;
                                }
                        }

                        
                        // If no node can be matched, assume the reference is to a string inside a block.
                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_STRING);
                        continue;
                }
                
                if(expression[0] == '-') {
                        if(expression.size() < 2) {
                                PRINT_COMPILER_ERROR("Unable to evaluate reference [%s]; Invalid Argument.\n", expression.c_str());
                                return ref;
                        }
                                
                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_NEGATED);
                        expression.erase(expression.begin());
                        continue;
                }

                if(expression[0] == '*') {
                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_PTR);
                        PR_METADATA_SET(ref, nthp::script::flagBits::IS_REFERENCE);

                        deref_ptr = true;
                        expression.erase(expression.begin());
                        continue;
                }

                if(expression[0] == '&') {
                        // A ptr reference sets the P_Ref value to the index of the variable.

                        // If a dereference prefix '*' is present along with a ptr_descriptor call '&',
                        // simpify to reference '$'. NOTE: *&var ==== $var. *&var takes longer to evaluate on runtime.
                        if(deref_ptr) {
                                PR_METADATA_CLEAR(ref, nthp::script::flagBits::IS_PTR);
                        }
                        else {
                                ptr_reference = true;
                                PR_METADATA_SET (ref, nthp::script::flagBits::IS_REFERENCE);
                        }
                        
                        expression.erase(expression.begin());
                        continue;
                }

                

                // No need for 2 compares. Only reaches this point if dereference character is present ($ OR > checked prior).
                if(expression[0] == '$') {
                        PR_METADATA_SET (ref, nthp::script::flagBits::IS_REFERENCE);

                        expression.erase(expression.begin());
                        continue;
                }

                break;
        } while(true);

        if(get_size) {
                for(size_t i = 0; i < structList.size(); ++i) {
                        if(expression == structList[i].name) {
                                ref.value = nthp::intToFixed(((nthp::script::stdVarWidth)structList[i].members.size()));
                                PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);

                                return ref;
                        }
                }

                PRINT_COMPILER_ERROR("STRUCT size request invalid; STRUCT [%s] does not exist.\n", expression.c_str());
                return ref;
        }

                
        std::string structAccess;
        bool isStructAccess = false;

        // I've realized I'm being pretty sparse with my comments, and working on dynamic offsets has me scratching my head as to how everything works.
        // I'm going to try writing more, so I can explain to my future self why something is the way it is.


        // Separates the access from the main variable. (variable.access).
        if(PR_METADATA_GET(ref, nthp::script::IS_REFERENCE)) {
                auto accessCharacter = expression.find_first_of('.');
                if(accessCharacter != std::string::npos) {
                        structAccess = expression;

                        structAccess.erase(0, accessCharacter + 1);
                        expression.erase(accessCharacter, expression.size() - 1);
                        
                        // Check if the access is another reference; if so, write the reference to an appended data node and set the IS_OFFSET_DYNAMIC flag for the current target reference.
                        // Prepare a data node if none is present. (a dynamicOffset value of 0 means there is no node present; NULL means dynamic offsets are disabled).
                        if(dynamicOffsetCounter != NULL) {
                                auto checkRef = EvaluateReference(structAccess, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, NULL, buildSystemContext, dynamicOffsetCounter, true);
                                
                                // An invalid reference is not fatal here; If the reference can't be evaluated, it pushes the evaluation down to the substitution below,
                                // assuming the reference is a struct memeber. IsStructAccess is the flag that enables it. Make sure it's set to false if the offset
                                // is evaluated here.
                                if(PR_METADATA_GET(checkRef, nthp::script::flagBits::IS_REFERENCE) && PR_METADATA_GET(checkRef, nthp::script::flagBits::IS_VALID)) {
                                        if((*dynamicOffsetCounter) > 0) {

                                                // Use +1 here instead of an increment; Allows for a failure without incorrectly increasing the counter.
                                                auto temp = (char*)realloc(nodeList.back().access.data, (sizeof(stdRef) * ((*dynamicOffsetCounter) + 1)));
                                                if(temp == NULL) { PRINT_COMPILER_ERROR("Failed to resize data node for dynamic offset @ [%zu].\n", currentNode); return ref; }

                                                nodeList.back().access.data = temp;

                                                dynamic_offset = true;
                                                stdRef* output = (stdRef*)(nodeList.back().access.data + (sizeof(stdRef) * (*dynamicOffsetCounter)));
                                                *output = checkRef;
                                                nodeList.back().access.size = (sizeof(stdRef) * (*dynamicOffsetCounter + 1));
                                                ref.offset = (*dynamicOffsetCounter);

                                                ++(*dynamicOffsetCounter);
                                                PR_METADATA_SET(ref, nthp::script::flagBits::IS_OFFSET_DYNAMIC);

                                        }
                                        else {
                                                ADD_NODE(DATA); // Add a new data node. Now, 'currentNode' will point to whatever instruction this evalutation is targeting, not this data node.
                                                nodeList.back().access.data = (char*)malloc(sizeof(stdRef));
                                                if(nodeList.back().access.data == NULL) { PRINT_COMPILER_ERROR("Failed to resize data node for dynamic offset @ [%zu].\n", currentNode); return ref; }

                                                stdRef* output = (stdRef*)(nodeList.back().access.data);
                                                dynamic_offset = true;

                                                *output = checkRef;
                                                nodeList.back().access.size = (sizeof(stdRef));
                                                ref.offset = (*dynamicOffsetCounter);

                                                ++(*dynamicOffsetCounter);
                                                PR_METADATA_SET(ref, nthp::script::flagBits::IS_OFFSET_DYNAMIC);
                                        }
                                }
                                else { // IS_REFERENCE is false or IS_VALID is false
                                        isStructAccess = true;
                                }
                        }
                        else { // dynamicOffsetCounter == NULL
                                isStructAccess = true;
                        }
                }
        }

        if(isConsteval) {
                for(size_t i = 0; i < constevalList.size(); ++i) {
                        if(expression == constevalList[i].name) {
                                ref.value = constevalList[i].evaluation.value;
                                
                                // Cannot offset by a structure type, because structures cannot be assigned to constevals.
                                // Cannot be offset by dynamic reference.
                                if(isStructAccess) {
                                        nthp::script::CompilerInstance::portable_evalConst(structAccess, constantList);
                                        auto offsetExpression = EvaluateReference(structAccess, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, globalRefIndex, buildSystemContext, NULL, false);
                                        if(!PR_METADATA_GET(offsetExpression, nthp::script::flagBits::IS_VALID)) { return ref; }
                                        if(PR_METADATA_GET(offsetExpression, nthp::script::flagBits::IS_REFERENCE)) {
                                                PRINT_COMPILER_ERROR("Custom offset to unassigned object cannot be a reference.\n");
                                                return ref;
                                        }
                                        if(PR_METADATA_GET(offsetExpression, nthp::script::flagBits::IS_STRING)) {
                                                PRINT_COMPILER_ERROR("Custom offset to unassigned object cannot be a string reference.\n");
                                                return ref;
                                        }
                                        if(nthp::fixedToInt(offsetExpression.value) > UINT8_MAX) {
                                                PRINT_COMPILER_ERROR("Custom offset to unassigned object cannot be greater than 255.\n");
                                                return ref;
                                        }

                                        ref.offset = nthp::fixedToInt(offsetExpression.value);
                                        PRINT_COMPILER("Custom offset of [%u] applied to CONSTEVAL stdref.\n", ref.offset);
                                }

                                if(ptr_reference) { PR_METADATA_CLEAR(ref, nthp::script::flagBits::IS_REFERENCE); }
                                if((!PR_METADATA_GET(ref, nthp::script::flagBits::IS_REFERENCE)) && PR_METADATA_GET(ref, nthp::script::flagBits::IS_NEGATED)) { ref.value = -ref.value; }

                                PRINT_COMPILER("Evaluated CONSTEVAL [%s]: Value = %llu, IR = %u\n", expression.c_str(), ref.value, PR_METADATA_GET(ref, nthp::script::flagBits::IS_REFERENCE));

                                if(globalRefIndex != NULL) { *globalRefIndex = 0; }
                                PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);
                                return ref;
                        }
                }
        }
        
        // Evaluate Var.
        // If no VARNAME is referenced, assumes numeral reference type (instead of $VARNAME or >VARNAME, $2 or >2), or constant. Throws
        // Invalid argument if otherwise.
        
        // The above comments are almost 2 years old. The 'numeral reference type' specified predates the variable system; a relic of THP, this project's progenitor.
        // In theory they should still work, but I don't feel like testing it. Don't want to delete this artifact comment.
        if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_REFERENCE)) {
                bool validReference = false;

                for(size_t i = 0; i < globalList.size(); ++i) {
                        if(expression == globalList[i].varName) {
                                if(globalList[i].isPrivate && (globalList[i].definedIn != currentFile)) {
                                        continue;
                                }
                                if(globalRefIndex != NULL) { *globalRefIndex = i; }

                                if(isStructAccess) {
                                        if(globalList[i].isStruct) {
                                        
                                                bool validAccess = false;
                                                for(size_t j = 0; j < structList[globalList[i].structID].members.size(); ++j) {

                                                        if(structAccess == structList[globalList[i].structID].members[j]) {
                                                                if(globalList[i].isFixed) {
                                                                        is_fixed_ref = true;
                                                                        validAccess = true;
                                                                        ref.offset = j;

                                                                        PRINT_COMPILER("Assigned fixed offset [%u] (%s) ; %s\n", ref.offset, structList[globalList[i].structID].members[j].c_str(), globalList[i].varName.c_str());
                                                                        break;
                                                                }
                                                                validAccess = true;
                                                                ref.offset = j;
                                                                PRINT_COMPILER("Assigned offset [%u] (%s) ; %s\n", ref.offset, structList[globalList[i].structID].members[j].c_str(), globalList[i].varName.c_str());
                                                                break;
                                                        }
                                                }
                                                if(!validAccess) { 
                                                        PRINT_COMPILER_ERROR("STRUCT [%s] has no member [%s].\n", structList[globalList[i].structID].name.c_str(), structAccess.c_str());
                                                        return ref;
                                                }
                                        }
                                        else {
                                                if(structAccess[0] == '#') { nthp::script::CompilerInstance::portable_evalConst(structAccess, constantList); }
                                                
                                                // I use evalRef here because it's safe and has all the error-checking and handling built in. Much easier than try-ing here.
                                                auto offsetExpression = EvaluateReference(structAccess, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, globalRefIndex, buildSystemContext, NULL, false);
                                                if(!PR_METADATA_GET(offsetExpression, nthp::script::flagBits::IS_VALID)) { return ref; }
                                                if(PR_METADATA_GET(offsetExpression, nthp::script::flagBits::IS_REFERENCE)) {
                                                        PRINT_COMPILER_ERROR("Custom offset to unassigned object cannot be a reference.\n");
                                                        return ref;
                                                }
                                                if(PR_METADATA_GET(offsetExpression, nthp::script::flagBits::IS_STRING)) {
                                                        PRINT_COMPILER_ERROR("Custom offset to unassigned object cannot be a string reference.\n");
                                                        return ref;
                                                }
                                                if(nthp::fixedToInt(offsetExpression.value) > UINT8_MAX) {
                                                        PRINT_COMPILER_ERROR("Custom offset to unassigned object cannot be greater than 255.\n");
                                                        return ref;
                                                }
                                                
                                                ref.offset = nthp::fixedToInt(offsetExpression.value);
                                                PRINT_COMPILER("Custom offset of [%u] applied to stdRef.\n", ref.offset);
                                        }
                                }



                                validReference = true;
                                expression = std::to_string(globalList[i].relativeIndex);
                        }
                        
                }

                if(!(validReference)) {
                        PRINT_COMPILER_ERROR("De/referenced definition [$%s] doesn't exist or is outside of scope.\n", expression.c_str());
                        return ref;
                }
        }
        else { 
                if(PR_METADATA_GET(ref, nthp::script::flagBits::IS_NEGATED)) {
                        expression = "-" + expression;
                }
        }


        try {
                double value = std::stod(expression);


                if((std::abs(value) < nthp::fixedTypeConstants::FIXED_EPSILON) && (value != 0)) {
                        PRINT_COMPILER_WARNING("Expression [%s] cannot be expressed/approximated in current fixed point system configuration. Expression will be rounded to 0.\n", expression.c_str());
                }

                ref.value = nthp::doubleToFixed(value);
                if(ptr_reference || (PR_METADATA_GET(ref, nthp::script::flagBits::IS_REFERENCE))) {
                        // Convert the saved index into a ptr descriptor. Block 0 is the GLOBAL LIST.
                        ref.value = nthp::script::constructPtrDescriptor(0, nthp::fixedToInt(ref.value));
                }

        }
        catch(std::invalid_argument) {
                if(!suppressFailure) PRINT_COMPILER_ERROR("Caught; Unable to evaluate reference [%s]; Invalid Argument.\n", expression.c_str());
                return ref;
        }
 
        // Remove IS_REFERENCE flag if referenced with ptr_reference prefix (&). Because the expression is now the
        // relative index of a reference, removing the IS_REFERENCE flag makes it evalute as a constant, meaning the
        // unadultered index of the VAR, or a ptr_reference!
        if(ptr_reference) { PR_METADATA_CLEAR(ref, nthp::script::flagBits::IS_REFERENCE); }

        // In order for is_fixed_ref to pass this check, IS_REFERENCE is set, a valid struct and offset is assigned, and the global in question
        // was declared as a FIXED. ref.value therefor is not encoded as a fixed point number.
        if(is_fixed_ref) {
                ref.value += ref.offset;
                ref.offset = 0;
        }


        PR_METADATA_SET(ref, nthp::script::flagBits::IS_VALID);
        NOVERB_PRINT_COMPILER("Evaluated Reference [%s]: Value = %llu, IR = %u\n", expression.c_str(), ref.value, PR_METADATA_GET(ref, nthp::script::flagBits::IS_REFERENCE));
        
        return ref; 
}



#define ____S_EVAL(...) if(EvaluateSymbol(__VA_ARGS__)) return 1

// Generic conviencence macro to evaluate the next symbol in the stream. Automatically pulls the next
// symbol from a macro or source file into 'fileRead'
#define EVAL_SYMBOL() ____S_EVAL(file, fileRead, constantList, macroList, currentMacroPosition, targetMacro, evaluateMacro)
#define EVAL_PREF() EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, NULL, buildSystemContext, &dynamicOffsetCounter, false)

#define CHECK_REF(ref) if(!PR_METADATA_GET(ref, nthp::script::flagBits::IS_VALID)) return 1 

// DEFINE COMPILER BEHAVIOUR FOR EACH INSTRUCTION HERE ||
//                                                     VV







// The HEADER Contains the signature for a compiled script ID,
// as well as a list of all labels and their positions, and the memory requirement
// to execute the script. 
DEFINE_COMPILATION_BEHAVIOUR(HEADER) {
        ADD_NODE(HEADER);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(EXIT) {
        ADD_NODE(EXIT);


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(LABEL) {
        ADD_NODE(LABEL);

        EVAL_SYMBOL(); // Label ID
        uint32_t* labelID = (decltype(labelID))nodeList[currentNode].access.data;

        try {
                *labelID = std::stoul(fileRead);
        }
        catch(std::invalid_argument) { 
                PRINT_COMPILER_ERROR("Unable to evaluate numeral at [%zu]; Invalid Argument.\n", currentNode);
                return 1; 
        }

        // Check for duplicate IDs.
        for(size_t i = 0; i < labelList.size(); ++i) {

                if(*labelID == labelList[i].ID) {
                        PRINT_COMPILER_ERROR("Unable to evaluate LABEL Node; Duplicate Label ID found at position [%u].\n", labelList[i].label_position);
                        return 1;
                }

        }

        // Add label to list.
        nthp::script::CompilerInstance::LABEL_DEF l_def;
        l_def.ID = *labelID;
        l_def.label_position = nodeList.size();

        labelList.push_back(l_def);
        NOVERB_PRINT_COMPILER("[%zu] LABEL Node evaluated: ID = %u\n", currentNode, *labelID);

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(GOTO) {
        ADD_NODE(GOTO);

        EVAL_SYMBOL();
        uint32_t* labelID = (decltype(labelID))nodeList[currentNode].access.data;
        uint32_t static_label;

        try {
                static_label = std::stoul(fileRead);
        }
        catch(std::invalid_argument) {
                PRINT_COMPILER_ERROR("Unable to evaluate numeral at [%zu]; Invalid Argument.\n", currentNode);
                return 1; 
        }


        nthp::script::CompilerInstance::GOTO_DEF newDef;
        newDef.goto_position = currentNode;
        newDef.points_to = static_label;

        *labelID = static_label;

        gotoList.push_back(newDef);
        NOVERB_PRINT_COMPILER("[%zu] GOTO Node evaluated: ID = %u\n", currentNode, *labelID);
        PRINT_NODEDATA();
        return 0;      
}


DEFINE_COMPILATION_BEHAVIOUR(SUSPEND) {
        ADD_NODE(SUSPEND);

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(JUMP) {
        ADD_NODE(JUMP);


        EVAL_SYMBOL();
        auto instruction = EVAL_PREF();
        CHECK_REF(instruction);

       stdRef* _position = (stdRef*)(nodeList[currentNode].access.data);

       *_position = instruction;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(RETURN) {
        ADD_NODE(RETURN);


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(GETINDEX) {
        ADD_NODE(GETINDEX);
        
        EVAL_SYMBOL();
        auto static_ref = EVAL_PREF();
        CHECK_REF(static_ref);

        ptrRef* pointer = decltype(pointer)(nodeList[currentNode].access.data);

        *pointer = static_ref;
        PRINT_NODEDATA();

        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(INC) {
        ADD_NODE(INC);

        EVAL_SYMBOL();
        auto static_var = EVAL_PREF();
        CHECK_REF(static_var);

        ptrRef* var = decltype(var)(nodeList[currentNode].access.data);

        *var = static_var;
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(DEC) {
        ADD_NODE(DEC);

        EVAL_SYMBOL();
        auto static_var = EVAL_PREF();
        CHECK_REF(static_var);

        ptrRef* var = decltype(var)(nodeList[currentNode].access.data);

        *var = static_var;
        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(RSHIFT) {
        ADD_NODE(RSHIFT);

        EVAL_SYMBOL();
        auto ref = EVAL_PREF();
        CHECK_REF(ref);

        EVAL_SYMBOL();
        auto count = EVAL_PREF();
        CHECK_REF(count);

        ptrRef* var = (decltype(var))(nodeList[currentNode].access.data);
        stdRef* fcount = (decltype(fcount))(nodeList[currentNode].access.data + sizeof(ptrRef));
        

        *var = ref;
        *fcount = count;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(LSHIFT) {
        ADD_NODE(LSHIFT);

        EVAL_SYMBOL();
        auto ref = EVAL_PREF();
        CHECK_REF(ref);

        EVAL_SYMBOL();
        auto count = EVAL_PREF();
        CHECK_REF(count);

        ptrRef* var = (decltype(var))(nodeList[currentNode].access.data);
        stdRef* fcount = (decltype(fcount))(nodeList[currentNode].access.data + sizeof(ptrRef));

        *var = ref;
        *fcount = count;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(ADD) {
        

        EVAL_SYMBOL();
        auto static_op_A = EVAL_PREF();
        CHECK_REF(static_op_A);

        EVAL_SYMBOL();
        auto static_op_B = EVAL_PREF();
        CHECK_REF(static_op_B);

        EVAL_SYMBOL();
        auto static_output = EVAL_PREF();
        CHECK_REF(static_output);


        if((!PR_METADATA_GET(static_op_A, nthp::script::flagBits::IS_REFERENCE)) && (!PR_METADATA_GET(static_op_B, nthp::script::flagBits::IS_REFERENCE))) {
                ADD_NODE(SET);

                ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);
                stdRef* _value = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

                stdRef value;
                value.metadata = 0;
                value.offset = 0;
                value.value = static_op_A.value + static_op_B.value;

                *_value = value;
                *target = static_output;

                PRINT_NODEDATA();
                return 0;
        }


        ADD_NODE(ADD);

        stdRef* operandA = decltype(operandA)(nodeList[currentNode].access.data);
        stdRef* operandB = decltype(operandB)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* output = decltype(output)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *operandA = static_op_A;
        *operandB = static_op_B;
        *output = static_output;

        
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SUB) {
     

        EVAL_SYMBOL();
        auto static_op_A = EVAL_PREF();
        CHECK_REF(static_op_A);

        EVAL_SYMBOL();
        auto static_op_B = EVAL_PREF();
        CHECK_REF(static_op_B);

        EVAL_SYMBOL();
        auto static_output = EVAL_PREF();
        CHECK_REF(static_output);


        if((!PR_METADATA_GET(static_op_A, nthp::script::flagBits::IS_REFERENCE)) && (!PR_METADATA_GET(static_op_B, nthp::script::flagBits::IS_REFERENCE))) {
                ADD_NODE(SET);

                ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);
                stdRef* _value = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

                stdRef value;
                value.metadata = 0;
                value.offset = 0;
                value.value = static_op_A.value - static_op_B.value;

                *_value = value;
                *target = static_output;

                PRINT_NODEDATA();
                return 0;
        }


        ADD_NODE(SUB);

        stdRef* operandA = decltype(operandA)(nodeList[currentNode].access.data);
        stdRef* operandB = decltype(operandB)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* output = decltype(output)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *operandA = static_op_A;
        *operandB = static_op_B;
        *output = static_output;

        
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(MUL) {
            

        EVAL_SYMBOL();
        auto static_op_A = EVAL_PREF();
        CHECK_REF(static_op_A);

        EVAL_SYMBOL();
        auto static_op_B = EVAL_PREF();
        CHECK_REF(static_op_B);

        EVAL_SYMBOL();
        auto static_output = EVAL_PREF();
        CHECK_REF(static_output);


        if((!PR_METADATA_GET(static_op_A, nthp::script::flagBits::IS_REFERENCE)) && (!PR_METADATA_GET(static_op_B, nthp::script::flagBits::IS_REFERENCE))) {
                ADD_NODE(SET);

                ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);
                stdRef* _value = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

                stdRef value;
                value.metadata = 0;
                value.offset = 0;
                value.value = nthp::f_fixedProduct(static_op_A.value, static_op_B.value);

                *_value = value;
                *target = static_output;

                PRINT_NODEDATA();
                return 0;
        }


        ADD_NODE(MUL);

        stdRef* operandA = decltype(operandA)(nodeList[currentNode].access.data);
        stdRef* operandB = decltype(operandB)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* output = decltype(output)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *operandA = static_op_A;
        *operandB = static_op_B;
        *output = static_output;

        
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(DIV) {
            

        EVAL_SYMBOL();
        auto static_op_A = EVAL_PREF();
        CHECK_REF(static_op_A);

        EVAL_SYMBOL();
        auto static_op_B = EVAL_PREF();
        CHECK_REF(static_op_B);

        EVAL_SYMBOL();
        auto static_output = EVAL_PREF();
        CHECK_REF(static_output);


        if((!PR_METADATA_GET(static_op_A, nthp::script::flagBits::IS_REFERENCE)) && (!PR_METADATA_GET(static_op_B, nthp::script::flagBits::IS_REFERENCE))) {
                ADD_NODE(SET);

                ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);
                stdRef* _value = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

                stdRef value;
                value.metadata = 0;
                value.offset = 0;
                value.value = nthp::f_fixedQuotient(static_op_A.value, static_op_B.value);

                *_value = value;
                *target = static_output;

                PRINT_NODEDATA();
                return 0;
        }


        ADD_NODE(DIV);

        stdRef* operandA = decltype(operandA)(nodeList[currentNode].access.data);
        stdRef* operandB = decltype(operandB)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* output = decltype(output)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *operandA = static_op_A;
        *operandB = static_op_B;
        *output = static_output;

        
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SQRT) {
        ADD_NODE(SQRT);

        nthp::script::instructions::stdRef* base = (decltype(base))nodeList[currentNode].access.data;
        ptrRef* output = (decltype(output))(nodeList[currentNode].access.data + sizeof(nthp::script::instructions::stdRef));

        EVAL_SYMBOL();
        auto static_base = EVAL_PREF();
        CHECK_REF(static_base);

        EVAL_SYMBOL();
        auto static_output = EVAL_PREF();
        CHECK_REF(static_output);

        *base = static_base;
        *output = static_output;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(POW) {
        ADD_NODE(POW);

        EVAL_SYMBOL();
        auto base = EVAL_PREF();
        CHECK_REF(base);

        EVAL_SYMBOL();
        auto exponent = EVAL_PREF();
        CHECK_REF(exponent);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _base = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* _exponent = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* _output = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *_base = base;
        *_exponent = exponent;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ABS) {
        ADD_NODE(ABS);


        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto ptr = EVAL_PREF();
        CHECK_REF(ptr);


        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _ptr = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_ptr = ptr;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(MOD) {
        ADD_NODE(MOD);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto divisor = EVAL_PREF();
        CHECK_REF(divisor);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* _divisor = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* _output = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));


        *_value = value;
        *_divisor = divisor;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(RAND) {
        ADD_NODE(RAND);


        EVAL_SYMBOL();
        auto a = EVAL_PREF();
        CHECK_REF(a);

        EVAL_SYMBOL();
        auto b = EVAL_PREF();
        CHECK_REF(b);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);


        stdRef* _a = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* _b = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* _output = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *_a = a;
        *_b = b;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SIN) {
        ADD_NODE(SIN);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(COS) {
        ADD_NODE(COS);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(TAN) {
        ADD_NODE(TAN);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ASIN) {
        ADD_NODE(ASIN);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ACOS) {
        ADD_NODE(ACOS);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ATAN) {
        ADD_NODE(ATAN);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _value = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_value = value;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(END) {
        ADD_NODE(END);
        endList.push_back(currentNode);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(ELSE) {
        ADD_NODE(ELSE);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SKIP) {
        ADD_NODE(SKIP);

        skipList.push_back(currentNode);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SKIP_END) {
        ADD_NODE(SKIP_END);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(IF) {

        nthp::script::instructions::stdRef static_opB;

        EVAL_SYMBOL();
        auto static_opA = EVAL_PREF();
        CHECK_REF(static_opA);


        EVAL_SYMBOL();
        do {
                if (fileRead == "EQU") {
                        ADD_NODE(LOGIC_EQU);
                        break;
                }
                if (fileRead == "NOT") {
                        ADD_NODE(LOGIC_NOT);
                        break;
                }
                if (fileRead == "GRT") {
                        ADD_NODE(LOGIC_GRT);
                        break;
                }
                if (fileRead == "LST") {
                        ADD_NODE(LOGIC_LST);
                        break;
                }
                if (fileRead == "GRTE") {
                        ADD_NODE(LOGIC_GRTE);
                        break;
                }
                if (fileRead == "LSTE") {
                        ADD_NODE(LOGIC_LSTE);
                        break;
                }
                {
                        // Completely different compiler behaviour if using the BNE instruction.
                        // opA is the only data parsed here.
                        
                        
                        // Because opA is the only argument, EXPRESSION currently is the next instruction after the IF.
                        // 'skipInstructionCheck' will make sure the next argument is NOT read at the start of the next pass,
                        // allowing the current symbol to be properly handled as a new expression.
                        // The warning 'Skipping eval this pass' is printed whenever skipInstructionCheck causes a skip.

                        ADD_NODE(LOGIC_IF_TRUE);
                        skipInstructionCheck = true;


                        ifList.push_back(currentNode);
                        nthp::script::instructions::stdRef* opA = (decltype(opA))(nodeList[currentNode].access.data);

                        *opA = static_opA;

                        PRINT_NODEDATA();
                        return 0;
                }
        } while(0);

        EVAL_SYMBOL();
        static_opB = EVAL_PREF();
        CHECK_REF(static_opB);

        nthp::script::instructions::stdRef* opA = (decltype(opA))(nodeList[currentNode].access.data);
        nthp::script::instructions::stdRef* opB = (decltype(opB))(nodeList[currentNode].access.data + sizeof(nthp::script::instructions::stdRef));

        *opA = static_opA;
        *opB = static_opB;

        // End location (final argument) is parsed post-compilation. Add to list to speed up process.
        ifList.push_back(currentNode);

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(SET) {
        ADD_NODE(SET);

        EVAL_SYMBOL();
        auto ptr_target = EVAL_PREF();
        CHECK_REF(ptr_target);

        EVAL_SYMBOL();
        auto value = EVAL_PREF();
        CHECK_REF(value);


        ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);
        stdRef* val = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *target = ptr_target;
        *val = value;


        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(ALLOC) {
        ADD_NODE(ALLOC);

        EVAL_SYMBOL();
        auto s_size = EVAL_PREF();
        CHECK_REF(s_size);

        EVAL_SYMBOL();
        auto storage_ptr = EVAL_PREF();
        CHECK_REF(storage_ptr);

        stdRef* size = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* ptr = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *size = s_size;
        *ptr = storage_ptr;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(NEW) {
        ADD_NODE(NEW);
        
        EVAL_SYMBOL();
        std::string structName = fileRead;

        for(size_t i = 0; i < structList.size(); ++i) {
                if(structName == structList[i].name) {


                        EVAL_SYMBOL();
                        auto s_size = EVAL_PREF();
                        CHECK_REF(s_size);

                        size_t pos = 0;

                        EVAL_SYMBOL();
                        auto storage_ptr = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, &dynamicOffsetCounter, false);
                        CHECK_REF(storage_ptr);

                        // Auto-assign the structure.
                        globalList[pos].isStruct = true;
                        globalList[pos].structID = i;

                        PRINT_COMPILER("Assigned STRUCT [%s] to GLOBAL [%zu].\n", structList[i].name.c_str(), pos);


                        stdRef* size = (stdRef*)(nodeList[currentNode].access.data);
                        ptrRef* ptr = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
                        uint32_t* entrySize = (uint32_t*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(ptrRef));

                        *size = s_size;
                        *ptr = storage_ptr;
                        *entrySize = structList[i].members.size();

                        PRINT_NODEDATA();
                        return 0;
                }
        }


        PRINT_COMPILER_ERROR("NEW expression failed; STRUCT [%s] does not exist.\n", structName.c_str());
        return 1;
}


DEFINE_COMPILATION_BEHAVIOUR(FREE) {
        ADD_NODE(FREE);

        EVAL_SYMBOL();
        auto ptr = EVAL_PREF();
        CHECK_REF(ptr);

        ptrRef* p = (ptrRef*)(nodeList[currentNode].access.data);

        *p = ptr;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(COPY) {
        ADD_NODE(COPY);

        EVAL_SYMBOL();
        auto src = EVAL_PREF();
        CHECK_REF(src);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        EVAL_SYMBOL();
        auto dst = EVAL_PREF();
        CHECK_REF(dst);

        ptrRef* _src = (ptrRef*)(nodeList[currentNode].access.data);
        stdRef* _size = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));
        ptrRef* _dst = (ptrRef*)(nodeList[currentNode].access.data + sizeof(ptrRef) + sizeof(stdRef));

        *_src = src;
        *_size = size;
        *_dst = dst;


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(NEXT) {
        ADD_NODE(NEXT);

        size_t pos = 0;

        EVAL_SYMBOL();
        auto ptr = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, &dynamicOffsetCounter, false);
        CHECK_REF(ptr);

        ptrRef* p = (ptrRef*)(nodeList[currentNode].access.data);
        uint8_t* structSize = (uint8_t*)(nodeList[currentNode].access.data + sizeof(ptrRef));   // The offset of the structure in stdVarWidths.

        *p = ptr;

        if(!PR_METADATA_GET(ptr, nthp::script::flagBits::IS_REFERENCE)) {
                if(globalList[pos].isStruct && pos != 0) {
                        *structSize = structList[globalList[pos].structID].members.size();
                }
                else {
                        *structSize = 1;
                }

                PRINT_NODEDATA();
                return 0;
        }

        *structSize = 1;


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(PREV) {
        ADD_NODE(PREV);

        size_t pos = 0;

        EVAL_SYMBOL();
        auto ptr = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, &dynamicOffsetCounter, false);
        CHECK_REF(ptr);

        ptrRef* p = (ptrRef*)(nodeList[currentNode].access.data);
        uint8_t* structSize = (uint8_t*)(nodeList[currentNode].access.data + sizeof(ptrRef));   // The offset of the structure in stdVarWidths.

        *p = ptr;

        if(!PR_METADATA_GET(ptr, nthp::script::flagBits::IS_REFERENCE)) {
                if(globalList[pos].isStruct && pos != 0) {
                        *structSize = structList[globalList[pos].structID].members.size();
                }
                else {
                        *structSize = 1;
                }

                PRINT_NODEDATA();
                return 0;
        }

        *structSize = 1;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(INDEX) {
        ADD_NODE(INDEX);

        size_t pos;

        EVAL_SYMBOL();
        auto ptr = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, &dynamicOffsetCounter, false);
        CHECK_REF(ptr);

        EVAL_SYMBOL();
        auto addr = EVAL_PREF();
        CHECK_REF(addr);

        ptrRef* block = (ptrRef*)(nodeList[currentNode].access.data);
        stdRef* location = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));
        uint8_t* offsetSize = (uint8_t*)(nodeList[currentNode].access.data + sizeof(ptrRef) + sizeof(stdRef));

        if((globalList[pos].isStruct && (!globalList[pos].isFixed)) && pos != 0) {
                *offsetSize = structList[globalList[pos].structID].members.size();
        }
        else { *offsetSize = 1; }


        *block = ptr;
        *location = addr;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(LAST) {
        ADD_NODE(LAST);

        size_t pos;

        EVAL_SYMBOL();
        auto target = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, &dynamicOffsetCounter, false);
        CHECK_REF(target);

        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data);
        uint8_t* offsetSize = (uint8_t*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        if((globalList[pos].isStruct && (!globalList[pos].isFixed)) && pos != 0) {
                *offsetSize = structList[globalList[pos].structID].members.size();
        }
        else { *offsetSize = 1; }


        *_target = target;
        PRINT_NODEDATA();

        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(GET_BLOCKSIZE) {
        ADD_NODE(GET_BLOCKSIZE);

        EVAL_SYMBOL();
        auto block = EVAL_PREF();
        CHECK_REF(block);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _block = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _output = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_block = block;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(SET_BLOCKLISTSIZE) {
        ADD_NODE(SET_BLOCKLISTSIZE);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        stdRef* _size = (stdRef*)(nodeList[currentNode].access.data);

        *_size = size;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ALLOC_TARGET) {
        ADD_NODE(ALLOC_TARGET);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        EVAL_SYMBOL();
        auto block_target = EVAL_PREF();
        CHECK_REF(block_target);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* _size = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* _block_target = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        ptrRef* _output = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));

        *_size = size;
        *_block_target = block_target;
        *_output = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(TEXTURE_ALLOC) {
	ADD_NODE(TEXTURE_ALLOC);

	
	EVAL_SYMBOL(); // file
	auto ref = EVAL_PREF();
	CHECK_REF(ref);

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        stdRef* size = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

	*size = ref;
        *target = output;
	
        PRINT_NODEDATA();
	return 0;	
}

DEFINE_COMPILATION_BEHAVIOUR(TEXTURE_FREE) {
	ADD_NODE(TEXTURE_FREE);

        EVAL_SYMBOL();
        auto block = EVAL_PREF();
        CHECK_REF(block);

        ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);
        *target = block;

        PRINT_NODEDATA();
	return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(TEXTURE_LOAD) {
        ADD_NODE(TEXTURE_LOAD);
	
	EVAL_SYMBOL(); // output
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL(); // file
        auto string = EVAL_PREF();
        CHECK_REF(string);

        textureRef* _target = (textureRef*)(nodeList[currentNode].access.data);
        strRef* _filename = (strRef*)(nodeList[currentNode].access.data + sizeof(textureRef));

        *_target = target;
        *_filename = string;


        PRINT_NODEDATA();
	return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(SET_ACTIVE_PALETTE) {
        ADD_NODE(SET_ACTIVE_PALETTE);

        EVAL_SYMBOL();
        auto filename = EVAL_PREF();
        CHECK_REF(filename);
        
        strRef* _filename = (strRef*)(nodeList[currentNode].access.data);

        *_filename = filename;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(FRAME_ALLOC) {
        ADD_NODE(FRAME_ALLOC);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        stdRef* dSize = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* dTarget = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *dSize = size;
        *dTarget = target;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(FRAME_FREE) {
        ADD_NODE(FRAME_FREE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        ptrRef* dTarget = (ptrRef*)(nodeList[currentNode].access.data);
        *dTarget = target;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(FRAME_SET) {
        ADD_NODE(FRAME_SET);

        EVAL_SYMBOL();
        auto sframeIndex = EVAL_PREF();
        CHECK_REF(sframeIndex);

        EVAL_SYMBOL();
        auto sx = EVAL_PREF();
        CHECK_REF(sx);

        EVAL_SYMBOL();
        auto sy = EVAL_PREF();
        CHECK_REF(sy);
        
        EVAL_SYMBOL();
        auto sw = EVAL_PREF();
        CHECK_REF(sw);
        
        EVAL_SYMBOL();
        auto sh = EVAL_PREF();
        CHECK_REF(sh);

        EVAL_SYMBOL();
        auto stextureIndex = EVAL_PREF();
        CHECK_REF(stextureIndex);

        frameRef* frameIndex = (frameRef*)(nodeList[currentNode].access.data);
        stdRef* x = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        stdRef* y = (stdRef*)(nodeList[currentNode].access.data + (sizeof(stdRef) * 2));
        stdRef* w = (stdRef*)(nodeList[currentNode].access.data + (sizeof(stdRef) * 3));
        stdRef* h = (stdRef*)(nodeList[currentNode].access.data + (sizeof(stdRef) * 4));
        textureRef* textureIndex = (textureRef*)(nodeList[currentNode].access.data + (sizeof(stdRef) * 5));

        *frameIndex = sframeIndex;
        *x = sx;
        *y = sy;
        *w = sw;
        *h = sh;
        *textureIndex = stextureIndex;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(ENT_ALLOC) {
        ADD_NODE(ENT_ALLOC);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        stdRef* dsize = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* dtarget = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *dsize = size;
        *dtarget = target;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_FREE) {
        ADD_NODE(ENT_FREE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);
        
        ptrRef* dtarget = (ptrRef*)(nodeList[currentNode].access.data);
        *dtarget = target;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_SETCURRENTFRAME) {
        ADD_NODE(ENT_SETCURRENTFRAME);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto frameN = EVAL_PREF();
        CHECK_REF(frameN);

        entRef* tout = (entRef*)(nodeList[currentNode].access.data);
        stdRef* tframe = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));

        *tout = target;
        *tframe = frameN;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_SETPOS) {
        ADD_NODE(ENT_SETPOS);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto x = EVAL_PREF();
        CHECK_REF(x);

        EVAL_SYMBOL();
        auto y = EVAL_PREF();
        CHECK_REF(y);

        entRef* _target = (entRef*)(nodeList[currentNode].access.data);
        stdRef* _x = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        stdRef* _y = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(stdRef));


        *_target = target;
        *_x = x;
        *_y = y;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(ENT_MOVE) {
        ADD_NODE(ENT_MOVE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto x = EVAL_PREF();
        CHECK_REF(x);

        EVAL_SYMBOL();
        auto y = EVAL_PREF();
        CHECK_REF(y);

        entRef* _target = (entRef*)(nodeList[currentNode].access.data);
        stdRef* _x = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        stdRef* _y = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(stdRef));


        *_target = target;
        *_x = x;
        *_y = y;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_SETFRAMERANGE) {
        ADD_NODE(ENT_SETFRAMERANGE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto start = EVAL_PREF();
        CHECK_REF(start);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);


        entRef* _target = (entRef*)(nodeList[currentNode].access.data);
        frameRef* _start = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        stdRef* _size = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(frameRef));

        *_target = target;
        *_start = start;
        *_size = size;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_SETHITBOXSIZE) {
        ADD_NODE(ENT_SETHITBOXSIZE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto x = EVAL_PREF();
        CHECK_REF(x);

        EVAL_SYMBOL();
        auto y = EVAL_PREF();
        CHECK_REF(y);

        entRef* _target = (entRef*)(nodeList[currentNode].access.data);
        stdRef* _x = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        stdRef* _y = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(stdRef));


        *_target = target;
        *_x = x;
        *_y = y;
        PRINT_NODEDATA();

        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_SETHITBOXOFFSET) {
        ADD_NODE(ENT_SETHITBOXOFFSET);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto x = EVAL_PREF();
        CHECK_REF(x);

        EVAL_SYMBOL();
        auto y = EVAL_PREF();
        CHECK_REF(y);

        entRef* _target = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* _x = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        stdRef* _y = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(stdRef));


        *_target = target;
        *_x = x;
        *_y = y;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(ENT_SETRENDERSIZE) {
        ADD_NODE(ENT_SETRENDERSIZE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto x = EVAL_PREF();
        CHECK_REF(x);

        EVAL_SYMBOL();
        auto y = EVAL_PREF();
        CHECK_REF(y);

        entRef* _target = (entRef*)(nodeList[currentNode].access.data);
        stdRef* _x = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        stdRef* _y = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(stdRef));


        *_target = target;
        *_x = x;
        *_y = y;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ENT_CHECKCOLLISION) {
        ADD_NODE(ENT_CHECKCOLLISION);

        EVAL_SYMBOL();
        auto ent_a = EVAL_PREF();
        CHECK_REF(ent_a);

        EVAL_SYMBOL();
        auto ent_b = EVAL_PREF();
        CHECK_REF(ent_b);

        EVAL_SYMBOL();
        auto _output = EVAL_PREF();
        CHECK_REF(_output);


        entRef* enta = (entRef*)(nodeList[currentNode].access.data);
        stdRef* entb = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));
        ptrRef* output = (ptrRef*)(nodeList[currentNode].access.data + sizeof(entRef) + sizeof(stdRef));

        *enta = ent_a;
        *entb = ent_b;
        *output = _output;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(ENT_SETANGLE) {
        ADD_NODE(ENT_SETANGLE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto angle = EVAL_PREF();
        CHECK_REF(angle);


        entRef* _target = (entRef*)(nodeList[currentNode].access.data);
        stdRef* _angle = (stdRef*)(nodeList[currentNode].access.data + sizeof(entRef));

        *_target = target;
        *_angle = angle;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_ALLOC) {
        ADD_NODE(SP_ALLOC);
        
        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        stdRef* _size = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_size = size;
        *_target = target;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_FREE) {
        ADD_NODE(SP_FREE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data);
        *_target = target;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_SETRENDERSIZE) {
        ADD_NODE(SP_SETRENDERSIZE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto w = EVAL_PREF();
        CHECK_REF(w);

        EVAL_SYMBOL();
        auto h = EVAL_PREF();
        CHECK_REF(h);

        setpieceRef* _target = (setpieceRef*)(nodeList[currentNode].access.data);
        stdRef* _w = (stdRef*)(nodeList[currentNode].access.data + sizeof(setpieceRef));
        stdRef* _h = (stdRef*)(nodeList[currentNode].access.data + sizeof(setpieceRef) + sizeof(stdRef));

        *_target = target;
        *_w = w;
        *_h = h;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(SP_SETFRAMERANGE) {
        ADD_NODE(SP_SETFRAMERANGE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto start = EVAL_PREF();
        CHECK_REF(start);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        setpieceRef* _target = (setpieceRef*)(nodeList[currentNode].access.data);
        frameRef* _start = (frameRef*)(nodeList[currentNode].access.data + sizeof(setpieceRef));
        stdRef* _size = (stdRef*)(nodeList[currentNode].access.data + sizeof(setpieceRef) + sizeof(frameRef));

        *_target = target;
        *_start = start;
        *_size = size;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_SETCURRENTFRAME) {
        ADD_NODE(SP_SETCURRENTFRAME);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto cf = EVAL_PREF();
        CHECK_REF(cf);

        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data);
        stdRef* _cf = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *_target = target;
        *_cf = cf;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_SETPOS) {
        ADD_NODE(SP_SETPOS);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto x = EVAL_PREF();
        CHECK_REF(x);

        EVAL_SYMBOL();
        auto y = EVAL_PREF();
        CHECK_REF(y);

        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data);
        stdRef* _x = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));
        stdRef* _y = (stdRef*)(nodeList[currentNode].access.data + sizeof(ptrRef) + sizeof(stdRef));
        
        
        *_target = target;
        *_x = x;
        *_y = y;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_SETANGLE) {
        ADD_NODE(SP_SETANGLE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto angle = EVAL_PREF();
        CHECK_REF(angle);


        setpieceRef* _target = (setpieceRef*)(nodeList[currentNode].access.data);
        stdRef* _angle = (stdRef*)(nodeList[currentNode].access.data + sizeof(setpieceRef));

        *_target = target;
        *_angle = angle;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_COMPILE) {
        ADD_NODE(SP_COMPILE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data);
        *_target = target;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SP_ABS_COMPILE) {
        ADD_NODE(SP_ABS_COMPILE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        ptrRef* _target = (ptrRef*)(nodeList[currentNode].access.data);
        *_target = target;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(CORE_INIT) {
        ADD_NODE(CORE_INIT);


        EVAL_SYMBOL();
        auto spx = EVAL_PREF();
        CHECK_REF(spx);

        EVAL_SYMBOL();
        auto spy = EVAL_PREF();
        CHECK_REF(spy);

        EVAL_SYMBOL();
        auto stx = EVAL_PREF();
        CHECK_REF(stx);

        EVAL_SYMBOL();
        auto sty = EVAL_PREF();
        CHECK_REF(sty);

        EVAL_SYMBOL();
        auto scx = EVAL_PREF();
        CHECK_REF(scx);

        EVAL_SYMBOL();
        auto scy = EVAL_PREF();
        CHECK_REF(scy);


        EVAL_SYMBOL();
        auto fullscreen = EVAL_PREF();
        CHECK_REF(fullscreen);

        EVAL_SYMBOL();
        auto softwareRender = EVAL_PREF();
        CHECK_REF(softwareRender);

        EVAL_SYMBOL();
        auto titleString = EVAL_PREF();
        CHECK_REF(titleString);

        // px py tx ty cx cy fs sr title
        stdRef* px = (decltype(px))(nodeList[currentNode].access.data);
        stdRef* py = (decltype(py))(nodeList[currentNode].access.data + sizeof(stdRef));
        stdRef* tx = (decltype(tx))(nodeList[currentNode].access.data + (sizeof(stdRef) * 2));
        stdRef* ty = (decltype(ty))(nodeList[currentNode].access.data + (sizeof(stdRef) * 3));
        stdRef* cx = (decltype(cx))(nodeList[currentNode].access.data + (sizeof(stdRef) * 4));
        stdRef* cy = (decltype(cy))(nodeList[currentNode].access.data + (sizeof(stdRef) * 5));
        stdRef* fs = (decltype(fs))(nodeList[currentNode].access.data + (sizeof(stdRef) * 6));
        stdRef* sr = (decltype(fs))(nodeList[currentNode].access.data + (sizeof(stdRef) * 7));
        strRef* _title = (decltype(_title))(nodeList[currentNode].access.data + (sizeof(stdRef) * 8));

        *px = spx;
        *py = spy;
        *tx = stx;
        *ty = sty;
        *cx = scx;
        *cy = scy;
        *fs = fullscreen;
        *sr = softwareRender;
        *_title = titleString;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_QRENDER) {
        ADD_NODE(CORE_QRENDER);

        EVAL_SYMBOL();
        auto entity = EVAL_PREF();
        CHECK_REF(entity);

        entRef* ent = (entRef*)nodeList[currentNode].access.data;
        *ent = entity;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_ABS_QRENDER) {
        ADD_NODE(CORE_ABS_QRENDER);

        EVAL_SYMBOL();
        auto entity = EVAL_PREF();
        CHECK_REF(entity);

        entRef* ent = (entRef*)nodeList[currentNode].access.data;
        *ent = entity;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_SP_QRENDER) {
        ADD_NODE(CORE_SP_QRENDER);

        EVAL_SYMBOL();
        auto setpiece = EVAL_PREF();
        CHECK_REF(setpiece);

        setpieceRef* _setpiece = (setpieceRef*)(nodeList[currentNode].access.data);
        *_setpiece = setpiece;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_SP_QRENDER_BLOCK) {
        ADD_NODE(CORE_SP_QRENDER_BLOCK);

        EVAL_SYMBOL();
        auto setpiece = EVAL_PREF();
        CHECK_REF(setpiece);

        ptrRef* _setpieceBlock = (ptrRef*)(nodeList[currentNode].access.data);
        *_setpieceBlock = setpiece;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(CORE_CLEAR) {
        ADD_NODE(CORE_CLEAR);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_DISPLAY) {
        ADD_NODE(CORE_DISPLAY);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_SETMAXFPS) {
        ADD_NODE(CORE_SETMAXFPS);

        EVAL_SYMBOL();
        auto fps = EVAL_PREF();
        CHECK_REF(fps);

        stdRef* _fps = (stdRef*)(nodeList[currentNode].access.data);
        *_fps = fps;

        PRINT_NODEDATA();
        return 0;
}

#define cast_stdRef(offset) (stdRef*)(nodeList[currentNode].access.data + (offset))

DEFINE_COMPILATION_BEHAVIOUR(CORE_SETWINDOWRES) {
        ADD_NODE(CORE_SETWINDOWRES);

        EVAL_SYMBOL();
        auto xRes = EVAL_PREF();
        CHECK_REF(xRes);

        EVAL_SYMBOL();
        auto yRes = EVAL_PREF();
        CHECK_REF(yRes);

        stdRef* x = cast_stdRef(0);
        stdRef* y = cast_stdRef(sizeof(stdRef));

        *x = xRes;
        *y = yRes;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_SETCAMERARES) {
        ADD_NODE(CORE_SETCAMERARES);

        EVAL_SYMBOL();
        auto xRes = EVAL_PREF();
        CHECK_REF(xRes);

        EVAL_SYMBOL();
        auto yRes = EVAL_PREF();
        CHECK_REF(yRes);

        stdRef* x = cast_stdRef(0);
        stdRef* y = cast_stdRef(sizeof(stdRef));

        *x = xRes;
        *y = yRes;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_SETCAMERAPOSITION) {
        ADD_NODE(CORE_SETCAMERAPOSITION);

        EVAL_SYMBOL();
        auto xRes = EVAL_PREF();
        CHECK_REF(xRes);

        EVAL_SYMBOL();
        auto yRes = EVAL_PREF();
        CHECK_REF(yRes);

        stdRef* x = cast_stdRef(0);
        stdRef* y = cast_stdRef(sizeof(stdRef));

        *x = xRes;
        *y = yRes;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_MOVECAMERA) {
        ADD_NODE(CORE_MOVECAMERA);

        EVAL_SYMBOL();
        auto xRes = EVAL_PREF();
        CHECK_REF(xRes);

        EVAL_SYMBOL();
        auto yRes = EVAL_PREF();
        CHECK_REF(yRes);

        stdRef* x = cast_stdRef(0);
        stdRef* y = cast_stdRef(sizeof(stdRef));

        *x = xRes;
        *y = yRes;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_GETMOUSEPOSITION) {
        ADD_NODE(CORE_GETMOUSEPOSITION);

        EVAL_SYMBOL();
        auto x_output = EVAL_PREF();
        CHECK_REF(x_output);

        EVAL_SYMBOL();
        auto y_output = EVAL_PREF();
        CHECK_REF(y_output);

        
        ptrRef* _xOutput = (ptrRef*)(nodeList[currentNode].access.data);
        ptrRef* _yOutput = (ptrRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *_xOutput = x_output;
        *_yOutput = y_output;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_ABS_GETMOUSEPOSITION) {
        ADD_NODE(CORE_ABS_GETMOUSEPOSITION);

        EVAL_SYMBOL();
        auto x_output = EVAL_PREF();
        CHECK_REF(x_output);

        EVAL_SYMBOL();
        auto y_output = EVAL_PREF();
        CHECK_REF(y_output);

        
        ptrRef* _xOutput = (ptrRef*)(nodeList[currentNode].access.data);
        ptrRef* _yOutput = (ptrRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *_xOutput = x_output;
        *_yOutput = y_output;

        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(ACTION_DEFINE) {
        ADD_NODE(ACTION_DEFINE);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        stdRef* psize = (stdRef*)(nodeList[currentNode].access.data);

        *psize = size;


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ACTION_CLEAR) {
        ADD_NODE(ACTION_CLEAR);


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(ACTION_BIND) {
        ADD_NODE(ACTION_BIND);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto var = EVAL_PREF(); // Global to bind it to.
        CHECK_REF(var);


        EVAL_SYMBOL();
        int32_t key = fileRead[0]; // should correspond to keycode. idk -_-

        do {
                
                if(fileRead == "ESCAPE")        { key = SDLK_ESCAPE; break; }
                if(fileRead == "TAB")           { key = SDLK_TAB; break; }
                if(fileRead == "RSHIFT")        { key = SDLK_RSHIFT; break; }
                if(fileRead == "LSHIFT")        { key = SDLK_LSHIFT; break; }
                if(fileRead == "RCTRL")         { key = SDLK_RCTRL; break; }
                if(fileRead == "LCTRL")         { key = SDLK_LCTRL; break; }
                if(fileRead == "RETURN")        { key = SDLK_RETURN; break; }
                if(fileRead == "UP")            { key = SDLK_UP; break; }
                if(fileRead == "DOWN")          { key = SDLK_DOWN; break; }
                if(fileRead == "LEFT")          { key = SDLK_LEFT; break; }
                if(fileRead == "RIGHT")         { key = SDLK_RIGHT; break; }
                if(fileRead == "SPACE")         { key = SDLK_SPACE; break; }
                if(fileRead == "BACKSPACE")     { key = SDLK_BACKSPACE; break; }
                if(fileRead == "CAPSLOCK")      { key = SDLK_CAPSLOCK; break; }
                if(fileRead == "LALT")          { key = SDLK_LALT; break; }
                if(fileRead == "RALT")          { key = SDLK_RALT; break; }

        } while(0);

        stdRef* _target = (stdRef*)(nodeList[currentNode].access.data);
        ptrRef* _varIndex = (ptrRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        int32_t* _key = (int32_t*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(ptrRef));

        *_target = target;
        *_varIndex = var;
        *_key = key;


        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(CORE_STOP) {
        ADD_NODE(CORE_STOP);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(POLL) {

        EVAL_SYMBOL();
        auto entity = EVAL_PREF();
        CHECK_REF(entity);

        // What to check for
        EVAL_SYMBOL();
        do {
                if(fileRead == "POSITION")              { ADD_NODE(POLL_ENT_POSITION); break; }
                if(fileRead == "HITBOX")                { ADD_NODE(POLL_ENT_HITBOX); break; }
                if(fileRead == "CURRENTFRAME")          { ADD_NODE(POLL_ENT_CURRENTFRAME); break; }
                if(fileRead == "RENDERSIZE")            { ADD_NODE(POLL_ENT_RENDERSIZE); break; }
                if(fileRead == "ANGLE")                 { ADD_NODE(POLL_ENT_ANGLE); break; }

                PRINT_COMPILER_ERROR("[%s] Invalid POLL request.", fileRead.c_str());
                return 1;

        } while(0);

        entRef* ent = (entRef*)(nodeList[currentNode].access.data);

        *ent = entity;
        PRINT_NODEDATA();

        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(DRAW_SETCOLOR) {
        ADD_NODE(DRAW_SETCOLOR);

        EVAL_SYMBOL();
        auto _colorIndex = EVAL_PREF();
        CHECK_REF(_colorIndex);

        _colorIndex.value = nthp::getFixedInteger(_colorIndex.value);

        stdRef* colorIndex = (stdRef*)(nodeList[currentNode].access.data);
        *colorIndex = _colorIndex;


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(DRAW_LINE) {
        ADD_NODE(DRAW_LINE);

        EVAL_SYMBOL();
        auto _x1 = EVAL_PREF();
        CHECK_REF(_x1);

        EVAL_SYMBOL();
        auto _y1 = EVAL_PREF();
        CHECK_REF(_y1);

        EVAL_SYMBOL();
        auto _x2 = EVAL_PREF();
        CHECK_REF(_x2);

        EVAL_SYMBOL();
        auto _y2 = EVAL_PREF();
        CHECK_REF(_y2);

        stdRef* x1 = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* y1 = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));
        stdRef* x2 = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef));
        stdRef* y2 = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef) + sizeof(stdRef) + sizeof(stdRef));

        *x1 = _x1;
        *y1 = _y1;
        *x2 = _x2;
        *y2 = _y2;


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(AUDIOCHANNEL_DEFINE) {
        ADD_NODE(AUDIOCHANNEL_DEFINE);

        EVAL_SYMBOL();
        auto channelCount = EVAL_PREF();
        CHECK_REF(channelCount);

        stdRef* _channelCount = (stdRef*)(nodeList[currentNode].access.data);

        *_channelCount = channelCount;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(SOUND_DEFINE) {
        ADD_NODE(SOUND_DEFINE);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        stdRef* s = (stdRef*)(nodeList[currentNode].access.data);

        *s = size;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SOUND_CLEAR) {
        ADD_NODE(SOUND_CLEAR);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SOUND_STOP) {
        ADD_NODE(SOUND_STOP);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        stdRef* _target = (stdRef*)(nodeList[currentNode].access.data);

        *_target = target;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(MUSIC_DEFINE) {
        ADD_NODE(MUSIC_DEFINE);

        EVAL_SYMBOL();
        auto size = EVAL_PREF();
        CHECK_REF(size);

        stdRef* s = (stdRef*)(nodeList[currentNode].access.data);

        *s = size;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(MUSIC_CLEAR) {
        ADD_NODE(MUSIC_CLEAR);


        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(MUSIC_LOAD) {
        ADD_NODE(MUSIC_LOAD);


        EVAL_SYMBOL();
        auto output_i = EVAL_PREF();
        CHECK_REF(output_i);


        EVAL_SYMBOL();
        auto filename = EVAL_PREF();
        CHECK_REF(filename);


        stdRef* output = (stdRef*)(nodeList[currentNode].access.data);
        strRef* _filename = (strRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *output = output_i;
        *_filename = filename;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(SOUND_LOAD) {
        ADD_NODE(SOUND_LOAD);

        EVAL_SYMBOL();
        auto output_i = EVAL_PREF();
        CHECK_REF(output_i);


        EVAL_SYMBOL();
        auto filename = EVAL_PREF();
        CHECK_REF(filename);


        stdRef* output = (stdRef*)(nodeList[currentNode].access.data);
        strRef* _filename = (strRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *output = output_i;
        *_filename = filename;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SOUND_PLAY) {
        ADD_NODE(SOUND_PLAY);

        EVAL_SYMBOL();
        auto index = EVAL_PREF();
        CHECK_REF(index);

        stdRef* output = (stdRef*)nodeList[currentNode].access.data;

        *output = index;
        
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SOUND_SETCHANNEL) {
        ADD_NODE(SOUND_SETCHANNEL);

        EVAL_SYMBOL();
        auto soundID = EVAL_PREF();
        CHECK_REF(soundID);

        EVAL_SYMBOL();
        auto channelID = EVAL_PREF();
        CHECK_REF(channelID);

        stdRef* _sound = (stdRef*)(nodeList[currentNode].access.data);
        stdRef* _channel = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *_sound = soundID;
        *_channel = channelID;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(MUSIC_START) {
        ADD_NODE(MUSIC_START);

        EVAL_SYMBOL();
        auto index = EVAL_PREF();
        CHECK_REF(index);

        stdRef* output = (stdRef*)nodeList[currentNode].access.data;

        *output = index;
        
        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(MUSIC_STOP) {
        ADD_NODE(MUSIC_STOP);

        
        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(MUSIC_PAUSE) {
        ADD_NODE(MUSIC_PAUSE);

        
        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(MUSIC_RESUME) {
        ADD_NODE(MUSIC_RESUME);

        
        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(MUSIC_SETVOLUME) {
        ADD_NODE(MUSIC_SETVOLUME);

        EVAL_SYMBOL();
        auto vol = EVAL_PREF();
        CHECK_REF(vol);

        stdRef* volume = (stdRef*)nodeList[currentNode].access.data;

        *volume = vol;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(SOUND_SETVOLUME) {
        ADD_NODE(SOUND_SETVOLUME);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto vol = EVAL_PREF();
        CHECK_REF(vol);

        stdRef* _target = (stdRef*)nodeList[currentNode].access.data;
        stdRef* volume = (stdRef*)(nodeList[currentNode].access.data + sizeof(stdRef));

        *volume = vol;
        *_target = target;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(DFILE_READ) {
        ADD_NODE(DFILE_READ);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto filename = EVAL_PREF();
        CHECK_REF(filename);


        ptrRef* _target = (strRef*)(nodeList[currentNode].access.data);
        strRef* _filename = (strRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *_target = target;
        *_filename = filename;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(DFILE_WRITE) {
        ADD_NODE(DFILE_WRITE);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto filename = EVAL_PREF();
        CHECK_REF(filename);


        ptrRef* _target = (strRef*)(nodeList[currentNode].access.data);
        strRef* _filename = (strRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *_target = target;
        *_filename = filename;

        
        PRINT_NODEDATA();
        return 0;
}



DEFINE_COMPILATION_BEHAVIOUR(PRINT) {

        EVAL_SYMBOL();
        auto output = EVAL_PREF();
        CHECK_REF(output);

        if(PR_METADATA_GET(output, nthp::script::flagBits::IS_STRING)) {
                ADD_NODE(PRINT_STRING);

                strRef* out = (strRef*)(nodeList[currentNode].access.data);
                *out = output;

                PRINT_NODEDATA();
                return 0;
        }

        ADD_NODE(PRINT_REF);


        stdRef* out = (stdRef*)(nodeList[currentNode].access.data);
        *out = output;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(STRING) {
        ADD_NODE(STRING);

        EVAL_SYMBOL();
        std::string name = fileRead;

        auto pos = file.tellg();
        file.close();
        file.open(currentFile, std::ios::in | std::ios::binary);
        file.seekg(pos);

        char buf[256]; // Max string length because of Node size being 8-bits.
        int count = 0;

        char file_read = 0;

        do { file.get(file_read); } while(file_read != '\"'); // Look for the first quote.
        file.get(file_read);
        do {
                buf[count] = file_read;
                ++count;
                file.get(file_read);
                
                if(count > 255) {
                        PRINT_COMPILER_ERROR("STRING out of bounds. MAX_STR=255\n");
                        return 1;
                }
        } while(file_read != '\"');
        buf[count] = '\0';

        pos = file.tellg();


        file.close();
        file.open(currentFile, std::ios::in);
        file.seekg(pos);


        nodeList[currentNode].access.size = count + 1;
        nodeList[currentNode].access.data = (char*)malloc(nodeList[currentNode].access.size);
        memcpy(nodeList[currentNode].access.data, buf, count + 1);


        nthp::script::CompilerInstance::STR_DEF def;
        def.objectPosition = currentNode;
        def.name = name;
        def.length = nodeList[currentNode].access.size;

        strList.push_back(def);

        PRINT_COMPILER("New STRING defined at [%u]; (%s)\n", def.objectPosition, buf);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(IB_SET_TARGET) {
        ADD_NODE(IB_SET_TARGET);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        ptrRef* _targ = (ptrRef*)(nodeList[currentNode].access.data);

        *_targ = target;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(IB_WRITE_STRING) {
        ADD_NODE(IB_WRITE_STRING);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(IB_STOP) {
        ADD_NODE(IB_STOP);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(TEXTINPUT_START) {
        ADD_NODE(TEXTINPUT_START);

        EVAL_SYMBOL();
        auto ref = EVAL_PREF();
        CHECK_REF(ref);

        ptrRef* target = (ptrRef*)(nodeList[currentNode].access.data);

        *target = ref;

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(TEXTINPUT_STOP) {
        ADD_NODE(TEXTINPUT_STOP);

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(STRING_COPY) {
        ADD_NODE(STRING_COPY);

        EVAL_SYMBOL();
        auto target = EVAL_PREF();
        CHECK_REF(target);

        EVAL_SYMBOL();
        auto stringRef = EVAL_PREF();
        CHECK_REF(stringRef);

        ptrRef* p_target = (ptrRef*)(nodeList[currentNode].access.data);
        strRef* p_string = (strRef*)(nodeList[currentNode].access.data + sizeof(ptrRef));

        *p_target = target;
        *p_string = stringRef;

        PRINT_NODEDATA();
        return 0;
}


DEFINE_COMPILATION_BEHAVIOUR(DEBUG_BREAK) {
        ADD_NODE(DEBUG_BREAK);

        PRINT_NODEDATA();
        return 0;
}

DEFINE_COMPILATION_BEHAVIOUR(ERROR_CLEAR) {
        ADD_NODE(ERROR_CLEAR);

        PRINT_NODEDATA();
        return 0;
}



// COMPILER INSTANCE BEHAVIOUR GOES HERE                ||
//                                                      VV



int nthp::script::CompilerInstance::compileSourceFile(const char* inputFile, const char* outputFile, bool buildSystemContext, uint8_t executionFlags, const bool ignoreInstructionData, bool createSymbolFile) {
        NOVERB_PRINT_COMPILER("\n\tCompiling Source File [%s]...\n\n", inputFile);
        
        std::fstream file(inputFile, std::ios::in);
        if(file.fail()) {
                PRINT_COMPILER_ERROR("Compilation failed; input source file not found.\n");
                return 1;
        }
        std::string fileRead;
        
        if(!buildSystemContext) { 
                globalList.clear(); 
                macroList.clear();
                constantList.clear();
        }
        labelList.clear();
        gotoList.clear();
        strList.clear();


        nthp::script::cleanNodeSet(nodeList);

        size_t globalAlloc = 0;

        std::fstream symbolFile;
        if(createSymbolFile) {
                std::string symbolFileName = outputFile;
                auto ext = symbolFileName.find_last_of('.');

                if(ext != std::string::npos)
                        symbolFileName.erase(symbolFileName.begin()+ext, symbolFileName.end());
                
                symbolFileName += ".sym";

                symbolFile.open(symbolFileName, std::ios::out);
                if(symbolFile.fail()) {
                        PRINT_COMPILER_ERROR("Failed to create symbol file.\n");
                        return 1;
                }
        }

        
        std::vector<size_t> ifLocations;
        std::vector<size_t> endLocations;
        std::vector<size_t> skipList;


        struct callStackObj {
                std::string file;
                std::streampos pos;
                bool importing;
        };
        std::vector<callStackObj> callStack;
        std::string currentFile = inputFile;
        bool inCalledFile = false;


        size_t currentMacroPosition = 0;
        size_t targetMacro = 0;
        bool evaluateMacro = false;

        bool waitingForFuncScopeReturn = false;


        #define COMPILE(instruction) if( instruction ( nodeList, file, fileRead, currentFile, constantList, constevalList, macroList, globalList, labelList, gotoList, strList, structList, ifLocations, endLocations, skipList, currentMacroPosition, targetMacro, evaluateMacro, buildSystemContext) ) return 1
        #define CHECK_COMP(instruction) if(fileRead == #instruction) { COMPILE(instruction); continue; }

        bool operationOngoing = true;

        if(!ignoreInstructionData) COMPILE(HEADER);

        while(operationOngoing) {
                COMP_START:
                

                if(skipInstructionCheck) {
                        PRINT_COMPILER_WARNING("Skipping EVAL this pass...\n");

                        skipInstructionCheck = false;
                }
                else {
                        EVAL_SYMBOL();
                }

                PRINT_DEBUG("Eval. Symbol; [%s] from [%s]\n", fileRead.c_str(), currentFile.c_str());

                
                
                if(fileRead == "FUNC") {
                        EVAL_SYMBOL();

                        PRINT_COMPILER("Defining new FUNC [%s]...\n", fileRead.c_str());

                        FUNC_DEF newFunc;
                        newFunc.name = fileRead;
                        newFunc.func_start = nodeList.size();
                        PRINT_COMPILER("Defined func_%s at [%u].\n", fileRead.c_str(), newFunc.func_start);

                        funcList.push_back(newFunc);

                        if(createSymbolFile) { newFunc.writeToFile(symbolFile); }

                        if(!ignoreInstructionData) {
                                ADD_NODE(FUNC_START);
                                uint32_t* ID = (uint32_t*)nodeList[currentNode].access.data;
                                *ID = funcList.size() - 1;

                                PRINT_NODEDATA();
                                

                                EVAL_SYMBOL();
                                if(fileRead == "{") {
                                        waitingForFuncScopeReturn = true;
                                }
                        }

                        continue;
                }


                if(fileRead == "STRUCT") {
                        structList.push_back(STRUCT_DEF());
                        EVAL_SYMBOL();

                        PRINT_COMPILER("Defining STRUCT [%s]...\n", fileRead.c_str());

                        structList.back().name = fileRead;
                        
                        EVAL_SYMBOL();
                        if(fileRead == "{") {
                                EVAL_SYMBOL();
                                if(fileRead == "}") {
                                        PRINT_COMPILER_ERROR("STRUCT cannot be defined; No members defined.\n");
                                        return 1;
                                }

                                while(fileRead != "}") {
                                        structList.back().members.push_back(fileRead);
                                        PRINT_COMPILER("\tAdded entry [%s] at [%02zX],\n", fileRead.c_str(), structList.back().members.size() - 1);
                                        EVAL_SYMBOL();
                                }

                                if(createSymbolFile) { structList.back().writeToFile(symbolFile); }
                                PRINT_COMPILER("Finished definition of STRUCT [%s].\n", structList.back().name.c_str());
                        }
                        else {
                                PRINT_COMPILER_ERROR("STRUCT cannot be defined; scope required.\n");
                                return 1;
                        }

                        continue;
                }

                if(fileRead == "EXIT") {
                        if(inCalledFile) {
                                if(callStack.size() > 0) {
                                        currentFile = callStack.back().file;
                                        file.close();

                                        file.open(currentFile, std::ios::in);
                                        if(file.fail()) { PRINT_COMPILER_ERROR("Critical Failure returning CALL;\n"); return 1; }
                                        file.seekg(callStack.back().pos);
                                        
                                        callStack.pop_back();
                                        if(callStack.size() == 0) { inCalledFile = false; currentFile = inputFile; }
                                }
                                PRINT_COMPILER("CALL/IMPORT Complete.\n");
                        }
                        else {
                                if(!ignoreInstructionData) COMPILE(EXIT);
                                operationOngoing = false;
                                break;
                        }

                        continue;
                }

                if(fileRead == "VAR") {
                        
                        // Define a new variable.
                        EVAL_SYMBOL();
                        bool invalidDefine = false;

                        for(size_t i = 0; i < globalList.size(); ++i) {
                                if(fileRead == globalList[i].varName) {
                                        PRINT_COMPILER_WARNING("GLOBAL [$%s] already declared; Ignoring redefinition.\n", fileRead.c_str());
                                        invalidDefine = true;
                                        break;
                                }
                        }

                        if(invalidDefine) continue;

                        PRINT_COMPILER("Defined GLOBAL [%s].\n", fileRead.c_str());
                        if(createSymbolFile) { symbolFile << "VAR " << fileRead << '\n'; }

                        addGlobalDef(fileRead.c_str(), currentFile.c_str());
                        ++globalAlloc;
                        continue;
                }

                if(fileRead == "CONSTEVAL") {
                        EVAL_SYMBOL();
                        std::string name = fileRead;

                        EVAL_SYMBOL();
                        std::string original = fileRead;
                        auto eval = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, NULL, buildSystemContext, NULL, false);

                        if(PR_METADATA_GET(eval, nthp::script::flagBits::IS_REFERENCE)) {
                                PRINT_COMPILER_ERROR("CONSTEVAL expression cannot be a reference; [%s] references writable memory.\n", original.c_str());
                                return 1;
                        }
                        if(!PR_METADATA_GET(eval, nthp::script::flagBits::IS_VALID)) {
                                PRINT_COMPILER_ERROR("CONSTEVAL expression could not be evaluated; Invalid CONSTEVAL [%s].\n", original.c_str());
                                return 1;
                        }

                        // If the expression is a pointer descriptor with an offset, the offset is added to the address.
                        // If not, than the offset cannot be non-zero, therefore this is okay; only applies to memory-mapped pointers
                        // that are hardcoded, which are not recommended anyhow.
                        eval.value += eval.offset;
                        eval.offset = 0;

                        CONSTEVAL_DEF newCst;
                        newCst.name = name;
                        newCst.evaluation = eval;
                        newCst.original_expression = original;

                        if(createSymbolFile) { newCst.writeToFile(symbolFile); }

                        bool duplicateDef = false;
                        for(size_t i = 0; i < constevalList.size(); ++i) {
                                if(name == constevalList[i].name) {
                                        PRINT_COMPILER("Redefining CONSTEVAL [%s]...\n", name.c_str());

                                        constevalList[i].original_expression = original;
                                        constevalList[i].evaluation = eval;

                                        duplicateDef = true;
                                }
                        }

                        if(!duplicateDef) constevalList.push_back(newCst);

                        PRINT_COMPILER("Evaluated CONSTEVAL [%s] = %d.\n", name.c_str(), newCst.evaluation.value);
                        continue;
                }

                if(fileRead == "ASSIGN") {
                        EVAL_SYMBOL();
                        bool complete = false;

                        for(size_t i = 0; i < structList.size(); ++i) {
                                if(fileRead == structList[i].name) {
                                        size_t pos = 0;

                                        EVAL_SYMBOL();
                                        auto eval_target = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, NULL, false);
                                        CHECK_REF(eval_target);

                                        if(PR_METADATA_GET(eval_target, nthp::script::flagBits::IS_REFERENCE)) {
                                                PRINT_COMPILER_ERROR("ASSIGN instruction assignment must be a constant ptr_descriptor.\n");
                                                return 1;
                                        }
                                        
                                        if(createSymbolFile) { symbolFile << "ASSIGN " << structList[i].name << " " << fileRead << '\n'; }
                                        
                                        globalList[pos].isStruct = true;
                                        globalList[pos].structID = i;

                                        PRINT_COMPILER("Assigned STRUCT [%s] to GLOBAL [%s].\n", structList[i].name.c_str(), globalList[pos].varName.c_str());
                                        complete = true;
                                        break;
                                }
                        }

                        if(!complete) {
                                PRINT_COMPILER_ERROR("Failure to match STRUCT; May or may not exist.\n");
                                return 1;
                        }

                        continue;
                }

                if(fileRead == "UNASSIGN") {
                        size_t pos = 0;

                        EVAL_SYMBOL();
                        auto eval_target = EvaluateReference(fileRead, nodeList, constantList, globalList, constevalList, strList, structList, currentFile, &pos, buildSystemContext, NULL, false);
                        CHECK_REF(eval_target);

                        if(PR_METADATA_GET(eval_target, nthp::script::flagBits::IS_REFERENCE)) {
                                PRINT_COMPILER_ERROR("UNASSIGN instruction assignment must be a constant ptr_descriptor.\n");
                                return 1;
                        }

                        if(createSymbolFile) { symbolFile << "UNASSIGN " << fileRead << '\n'; }

                        globalList[pos].isStruct = false;
                        globalList[pos].structID = 0;

                        PRINT_COMPILER("Removed STRUCT assignment from [%s].\n", fileRead.c_str());
                        continue;
                }

                if(fileRead == "FIXED") {

                        EVAL_SYMBOL();
                        bool validStruct = false;
                        size_t newFixedPosition = 0;
                        for(size_t i = 0; i < structList.size(); ++i) {
                                if(fileRead == structList[i].name) {
                                        EVAL_SYMBOL();

                                        for(size_t j = 0; j < globalList.size(); ++j) {
                                                if(fileRead == globalList[j].varName) {
                                                        if((globalList[j].isPrivate) && globalList[j].definedIn != currentFile) {
                                                                break;
                                                        }
                                                        
                                                        PRINT_COMPILER_ERROR("Defined duplicate VAR/PRIVATE [%s] in [%s].\n", fileRead.c_str(), currentFile.c_str());
                                                        return 1;
                                                }
                                        }

                                        GLOBAL_DEF newFixed;
                                        newFixed.varName = fileRead;
                                        newFixed.definedIn = currentFile;
                                        newFixed.relativeIndex = globalList.size();
                                        newFixedPosition = globalList.size();
                                        newFixed.isPrivate = false;
                                        newFixed.structID = i;
                                        newFixed.isStruct = true;
                                        newFixed.isFixed = true;

                                        globalList.push_back(newFixed);
                                        ++globalAlloc;

                                        std::string globalAdd;
                                        for(size_t k = 1; k < structList[i].members.size(); ++k) {
                                                globalAdd = "__." + (structList[i].members[k]);
                                                addGlobalDef(globalAdd.c_str(), currentFile.c_str());
                                                ++globalAlloc;
                                        }

                                        if(createSymbolFile) { symbolFile << "FIXED " << structList[i].name << " " << fileRead << '\n'; }
                                        
                                        validStruct = true;
                                }
                        }

                        if(!validStruct) {
                                PRINT_COMPILER_ERROR("Unable to define FIXED; [%s] Invalid struct.\n", fileRead.c_str());
                                return 1;
                        }

                        PRINT_COMPILER("Added new FIXED [%s] to GLOBAL list; struct=[%s] size=[%zu].\n", fileRead.c_str(), structList[globalList[newFixedPosition].structID].name.c_str(), structList[globalList[newFixedPosition].structID].members.size());
                        continue;
                }


                if(fileRead == "PRIVATE") {
                        // Define a new variable.
                        EVAL_SYMBOL();
                        bool invalidDefine = false;
 
                        for(size_t i = 0; i < globalList.size(); ++i) {
                                if(fileRead == globalList[i].varName) {
                                        if(!(globalList[i].isPrivate)) {
                                                PRINT_COMPILER_WARNING("GLOBAL [$%s] already declared; Ignoring redefinition.\n", fileRead.c_str());
                                                invalidDefine = true;
                                                break;
                                        }
                                        if(globalList[i].definedIn == currentFile) {
                                                PRINT_COMPILER_WARNING("PRIVATE [$%s] already declared in current scope; Ignoring redefinition.\n", fileRead.c_str());
                                                invalidDefine = true;
                                                break;
                                        }
                                }
                        }
                        if(invalidDefine) continue;
 
                        if(createSymbolFile) { symbolFile << "PRIVATE " << fileRead; }
                        PRINT_COMPILER("Defined PRIVATE GLOBAL [%s].\n", fileRead.c_str());
                        

                        addPrivateGlobalDef(fileRead.c_str(), currentFile.c_str());
                        ++globalAlloc;
                        continue;
                }


                if(fileRead == "CONST") {
                        // Define new Const sub.

                        EVAL_SYMBOL();  // Name
                        CONST_DEF newDef;

                        fileRead = '#' + fileRead;
                        newDef.constName = fileRead;
                        for(size_t i = 0; i < constantList.size(); ++i) {
                                if(newDef.constName == constantList[i].constName) {
                                        PRINT_COMPILER("Redefinition of CONST [%s];", newDef.constName.c_str());
                                        EVAL_SYMBOL(); // Substitution
                                        
                                        if(fileRead == newDef.constName) {
                                                PRINT_COMPILER_ERROR("CONST [%s] cannot substitute itself.\n", newDef.constName.c_str());
                                                return 1;
                                        }

                                        constantList[i].value = fileRead;
                                        NOVERB_PRINT_COMPILER(" sub.= %s\n", fileRead.c_str());
                                        goto COMP_START;
                                }
                        }

                        EVAL_SYMBOL(); // Substitution
                        newDef.value = fileRead;

                        if(createSymbolFile) { newDef.writeToFile(symbolFile); }

                        constantList.push_back(newDef);
                        PRINT_COMPILER("New CONST Definition; n=%s sub=%s\n", newDef.constName.c_str(), newDef.value.c_str());
                        continue;
                }

                if(fileRead == "UNDEF") {
                        EVAL_SYMBOL();
                        undefConstant(fileRead.c_str(), constantList);

                        if(createSymbolFile) { symbolFile << "UNDEF " << fileRead << '\n'; }

                        continue;
                }

                if(fileRead == "ENUM") {
                       
                        EVAL_SYMBOL();
                        if(fileRead != "{") {
                                PRINT_COMPILER_ERROR("ENUM must be scoped; scope missing at ENUM [~%zu].\n", currentNode);
                                return 1;
                        }
                        PRINT_COMPILER("Starting new AUTOENUM...\n");

                        CONST_DEF tempConst;
                        for(size_t size = 0; fileRead != "}"; ++size) {
                                EVAL_SYMBOL();
                                
                                tempConst.constName = "#" + fileRead;
                                tempConst.value = std::to_string(size);

                                if(fileRead != "}") { constantList.push_back(tempConst); tempConst.writeToFile(symbolFile); }
                                else { break; }

                                NOVERB_PRINT_COMPILER("\t%s / %zu,\n", fileRead.c_str(), size);
                        }

                        PRINT_COMPILER("done.\n");
                        continue;
                }

                if(fileRead == "MACRO") {
                        // Define new Macro.
                        EVAL_SYMBOL();          // Name
                        MACRO_DEF newDef;

                        newDef.macroName = '@' + fileRead;


                        for(size_t i = 0; i < macroList.size(); ++i) {
                                if(macroList[i].macroName == newDef.macroName) {
                                        PRINT_COMPILER_WARNING("Duplicate MACRO [%s] at [~%zu]; Ignoring Definition.\n", macroList[i].macroName.c_str(), nodeList.size());
                                        do {file >> fileRead; } while(fileRead != "}");
                                        goto COMP_START;
                                }
                        }

                        PRINT_COMPILER("Defining new MACRO [%s]...", fileRead.c_str());

                        READ_FILE();     // Gets rid of the '{'
                        for(size_t i = 0; fileRead != "}"; ++i) {
                                READ_FILE();
                                if(fileRead == "/") { do { READ_FILE(); } while(fileRead != "/"); EVAL_SYMBOL(); }
                                newDef.macroData.push_back(fileRead);
                                
                                if((i % 5) == 0) { NOVERB_PRINT_COMPILER("\n\t"); }

                                NOVERB_PRINT_COMPILER(" [%s]", fileRead.c_str());
                        }
                        newDef.macroData.pop_back(); // Remove the '}'
                        macroList.push_back(newDef);

                        if(createSymbolFile) { newDef.writeToFile(symbolFile); }

                        NOVERB_PRINT_COMPILER("\n");
                        PRINT_COMPILER("Added MACRO [%s] to MACRO list.\n", macroList.back().macroName.c_str());
                        continue;

                }

                if(fileRead == "CALL") {
                        EVAL_SYMBOL(); //filename 

                        callStackObj newFile;
                        newFile.file = currentFile;
                        newFile.pos = file.tellg();
                        newFile.importing = false;

                        callStack.push_back(newFile);
                        PRINT_COMPILER("Added file [%s] to CallStack.\n", callStack.back().file.c_str());
                        
                        file.close();

                        PRINT_COMPILER("Beginning CALL Operation to file [%s]...\n", fileRead.c_str());

                        file.open(fileRead, std::ios::in);
                        if(file.fail()) {
                                PRINT_COMPILER_ERROR("Unable to IMPORT; File not found.\n");
                                return 1;
                        }
                        currentFile = fileRead;
                        inCalledFile = true;

                        continue;
                }

                if(fileRead == "IMPORT") {
                        EVAL_SYMBOL(); // name

                        callStackObj newFile;
                        newFile.file = currentFile;
                        newFile.pos = file.tellg();
                        newFile.importing = true;

                        callStack.push_back(newFile);
                
                        file.close();
                        
                        PRINT_COMPILER("Beginning IMPORT Operation to file [%s]...\n", fileRead.c_str());

                        file.open(fileRead, std::ios::in);
                        if(file.fail()) {
                                PRINT_COMPILER_ERROR("Unable to IMPORT; File not found.\n");
                                return 1;
                        }

                        currentFile = fileRead;
                        inCalledFile = true;

                        continue;
                }

                if(fileRead == "}") {
                        if(waitingForFuncScopeReturn) {
                                waitingForFuncScopeReturn = false;

                                ADD_NODE(RETURN);
                                continue;
                        }
                }

                if(fileRead == "DEPEND") {
                        EVAL_SYMBOL();

                        do {
                                if(fileRead == "CONST") {
                                        EVAL_SYMBOL();
                                        fileRead = "#" + fileRead;

                                        bool success = false;
                                        for(size_t i = 0; i < constantList.size(); ++i) {
                                                if(fileRead == constantList[i].constName) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) break;
                                        else {
                                                PRINT_COMPILER_DEPEND_ERROR("CONST dependency [%s] not declared; dependency check failed.\n", fileRead.c_str());
                                                return 1;
                                        }

                                        break;
                                }

                                if(fileRead == "MACRO") {
                                        EVAL_SYMBOL();
                                        bool success = false;
                                        
                                        for(size_t i = 0; i < macroList.size(); ++i) {
                                                if(fileRead == macroList[i].macroName) {
                                                        success = true;
                                                        break;
                                                }
                                        }
                                        if(success) break;
                                        else {
                                                PRINT_COMPILER_DEPEND_ERROR("MACRO dependency [%s] not declared; dependency check failed.\n", fileRead.c_str());
                                                return 1;
                                        }

                                        break;
                                }

                                if(fileRead == "VAR") {
                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < globalList.size(); ++i) {
                                                if(fileRead == globalList[i].varName) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) break;
                                        else {
                                                PRINT_COMPILER_DEPEND_ERROR("VAR dependency [%s] not declared; dependency check failed.\n", fileRead.c_str());
                                                return 1;
                                        }

                                        break;
                                }

                                if(fileRead == "STRUCT") {
                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < structList.size(); ++i) {
                                                if(fileRead == structList[i].name) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) break;
                                        else {
                                                PRINT_COMPILER_DEPEND_ERROR("STRUCT dependency [%s] not declared; dependency check failed.\n", fileRead.c_str());
                                                return 1;
                                        }

                                        break;
                                }

                                if(fileRead == "FUNC") {
                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < funcList.size(); ++i) {
                                                if(fileRead == funcList[i].name) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) break;
                                        else {
                                                PRINT_COMPILER_DEPEND_ERROR("FUNC dependency [%s] not declared; dependency check failed.\n", fileRead.c_str());
                                                return 1;
                                        }

                                        break;
                                }

                                if(fileRead == "STRING") {

                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < strList.size(); ++i) {
                                                if(fileRead == strList[i].name) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) break;
                                        else {
                                                PRINT_COMPILER_DEPEND_ERROR("STRING dependency [%s] not declared; dependency check failed.\n", fileRead.c_str());
                                                return 1;
                                        }

                                        break;
                                }

                        } while(0);

                        continue;
                }

                if(fileRead == "IFDEF") {
                        EVAL_SYMBOL();

                        do {
                                if(fileRead == "CONST") {
                                        EVAL_SYMBOL();
                                        fileRead = "#" + fileRead;

                                        bool success = false;
                                        for(size_t i = 0; i < constantList.size(); ++i) {
                                                if(fileRead == constantList[i].constName) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) {
                                                PRINT_COMPILER("CD IFDEF check passed; [%s] defined.\n", fileRead);
                                                break;
                                        }
                                        else {
                                                do { READ_FILE(); } while(fileRead != "IFDEF_END");
                                                break;
                                        }

                                        break;
                                }

                                if(fileRead == "MACRO") {
                                        EVAL_SYMBOL();
                                        bool success = false;
                                        
                                        for(size_t i = 0; i < macroList.size(); ++i) {
                                                if(fileRead == macroList[i].macroName) {
                                                        success = true;
                                                        break;
                                                }
                                        }
                                        if(success) {
                                                PRINT_COMPILER("CD IFDEF check passed; [%s] defined.\n", fileRead);
                                                break;
                                        }
                                        else {
                                                do { READ_FILE(); } while(fileRead != "IFDEF_END");
                                                break;
                                        }

                                        break;
                                }

                                if(fileRead == "VAR") {
                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < globalList.size(); ++i) {
                                                if(fileRead == globalList[i].varName) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                       if(success) {
                                                PRINT_COMPILER("CD IFDEF check passed; [%s] defined.\n", fileRead);
                                                break;
                                        }
                                        else {
                                                do { READ_FILE(); } while(fileRead != "IFDEF_END");
                                                break;
                                        }
                                }

                                if(fileRead == "STRUCT") {
                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < structList.size(); ++i) {
                                                if(fileRead == structList[i].name) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) {
                                                PRINT_COMPILER("CD IFDEF check passed; [%s] defined.\n", fileRead);
                                                break;
                                        }
                                        else {
                                                do { READ_FILE(); } while(fileRead != "IFDEF_END");
                                                break;
                                        }
                                }

                                if(fileRead == "FUNC") {
                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < funcList.size(); ++i) {
                                                if(fileRead == funcList[i].name) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) {
                                                PRINT_COMPILER("CD IFDEF check passed; [%s] defined.\n", fileRead);
                                                break;
                                        }
                                        else {
                                                do { READ_FILE(); } while(fileRead != "IFDEF_END");
                                                break;
                                        }
                                }

                                if(fileRead == "STRING") {

                                        EVAL_SYMBOL();
                                        bool success = false;

                                         for(size_t i = 0; i < strList.size(); ++i) {
                                                if(fileRead == strList[i].name) {
                                                        success = true;
                                                        break;
                                                }
                                        }

                                        if(success) {
                                                PRINT_COMPILER("CD IFDEF check passed; [%s] defined.\n", fileRead);
                                                break;
                                        }
                                        else {
                                                do { READ_FILE(); } while(fileRead != "IFDEF_END");
                                                break;
                                        }

                                        break;
                                }

                                PRINT_COMPILER_ERROR("Invalid definition for IFDEF check.\n");
                                return 1;

                        } while(0);

                        continue;
                }


                if(ignoreInstructionData) {
                        continue;
                }
                if(callStack.size() > 0) {
                        if(callStack.back().importing) continue;
                }
                
                // Symbol for a FUNC_CALL
                if(fileRead[0] == '%' || fileRead == "FUNC_CALL") {
                        if(fileRead[0] == '%') fileRead.erase(fileRead.begin());
                        else { EVAL_SYMBOL(); }

                        bool matchedFunc = false;
                        for(size_t i = 0; i < funcList.size(); ++i) {
                                if(fileRead == funcList[i].name) {
                                        ADD_NODE(FUNC_CALL);

                                        uint32_t* ID = (uint32_t*)nodeList[currentNode].access.data;
                                        *ID = i;
                                        matchedFunc = true;

                                        break;
                                }
                        }

                        if(!matchedFunc) {
                                PRINT_COMPILER_ERROR("Unable to match FUNC_CALL [%s]; FUNC not found.\n", fileRead);
                                return 1;
                        }

                        continue;
                }


                CHECK_COMP(LABEL);
                CHECK_COMP(GOTO);
                CHECK_COMP(SUSPEND);
                CHECK_COMP(JUMP);
                CHECK_COMP(GETINDEX);
                CHECK_COMP(RETURN);

                CHECK_COMP(INC);
                CHECK_COMP(DEC);
                CHECK_COMP(RSHIFT);
                CHECK_COMP(LSHIFT);

                CHECK_COMP(ADD);
                CHECK_COMP(SUB);
                CHECK_COMP(MUL);
                CHECK_COMP(DIV);
                CHECK_COMP(SQRT);
                CHECK_COMP(POW);
                CHECK_COMP(ABS);
                CHECK_COMP(MOD);
                CHECK_COMP(RAND);
                CHECK_COMP(SIN);
                CHECK_COMP(COS);
                CHECK_COMP(TAN);
                CHECK_COMP(ASIN);
                CHECK_COMP(ACOS);
                CHECK_COMP(ATAN);

                CHECK_COMP(IF);
                CHECK_COMP(END);
                CHECK_COMP(ELSE);
                CHECK_COMP(SKIP);
                CHECK_COMP(SKIP_END);

                CHECK_COMP(SET);
                CHECK_COMP(ALLOC);
                CHECK_COMP(NEW);
                CHECK_COMP(COPY);
                CHECK_COMP(FREE);
                CHECK_COMP(NEXT);
                CHECK_COMP(PREV);
                CHECK_COMP(INDEX);
                CHECK_COMP(LAST);
                CHECK_COMP(GET_BLOCKSIZE);
                CHECK_COMP(SET_BLOCKLISTSIZE);
                CHECK_COMP(ALLOC_TARGET);

		CHECK_COMP(TEXTURE_ALLOC);
		CHECK_COMP(TEXTURE_FREE);
		CHECK_COMP(TEXTURE_LOAD);
                CHECK_COMP(SET_ACTIVE_PALETTE);


                CHECK_COMP(FRAME_ALLOC);
                CHECK_COMP(FRAME_FREE);
                CHECK_COMP(FRAME_SET);

                CHECK_COMP(ENT_ALLOC);
                CHECK_COMP(ENT_FREE);

                CHECK_COMP(ENT_SETCURRENTFRAME);
                CHECK_COMP(ENT_SETPOS);
                CHECK_COMP(ENT_MOVE);
                CHECK_COMP(ENT_SETFRAMERANGE);
                CHECK_COMP(ENT_SETHITBOXSIZE);
                CHECK_COMP(ENT_SETHITBOXOFFSET);
                CHECK_COMP(ENT_SETRENDERSIZE);
                CHECK_COMP(ENT_CHECKCOLLISION);
                CHECK_COMP(ENT_SETANGLE);

                CHECK_COMP(SP_ALLOC);
                CHECK_COMP(SP_FREE);
                CHECK_COMP(SP_SETRENDERSIZE);
                CHECK_COMP(SP_SETFRAMERANGE);
                CHECK_COMP(SP_SETCURRENTFRAME);
                CHECK_COMP(SP_SETPOS);
                CHECK_COMP(SP_SETANGLE);
                CHECK_COMP(SP_COMPILE);
                CHECK_COMP(SP_ABS_COMPILE);

                CHECK_COMP(CORE_INIT);
                CHECK_COMP(CORE_QRENDER);
                CHECK_COMP(CORE_ABS_QRENDER);
                CHECK_COMP(CORE_SP_QRENDER);
                CHECK_COMP(CORE_SP_QRENDER_BLOCK);
                CHECK_COMP(CORE_CLEAR);
                CHECK_COMP(CORE_DISPLAY);
                CHECK_COMP(CORE_SETMAXFPS);
                CHECK_COMP(CORE_SETWINDOWRES);
                CHECK_COMP(CORE_SETCAMERARES);
                CHECK_COMP(CORE_SETCAMERAPOSITION);
                CHECK_COMP(CORE_MOVECAMERA);
                CHECK_COMP(CORE_STOP);
                CHECK_COMP(CORE_GETMOUSEPOSITION);
                CHECK_COMP(CORE_ABS_GETMOUSEPOSITION);

                CHECK_COMP(ACTION_BIND);
                CHECK_COMP(ACTION_DEFINE);
                CHECK_COMP(ACTION_CLEAR);

                CHECK_COMP(POLL);


                CHECK_COMP(DRAW_SETCOLOR);
                CHECK_COMP(DRAW_LINE);

                CHECK_COMP(AUDIOCHANNEL_DEFINE);
                CHECK_COMP(SOUND_DEFINE);
                CHECK_COMP(SOUND_CLEAR);
                CHECK_COMP(MUSIC_DEFINE);
                CHECK_COMP(MUSIC_CLEAR);
                CHECK_COMP(MUSIC_LOAD);
                CHECK_COMP(SOUND_LOAD);
                CHECK_COMP(SOUND_PLAY);
                CHECK_COMP(SOUND_SETCHANNEL);
                CHECK_COMP(SOUND_STOP);
                CHECK_COMP(MUSIC_START);
                CHECK_COMP(MUSIC_STOP);
                CHECK_COMP(MUSIC_PAUSE);
                CHECK_COMP(MUSIC_RESUME);
                CHECK_COMP(MUSIC_SETVOLUME);
                CHECK_COMP(SOUND_SETVOLUME);

                CHECK_COMP(DFILE_READ);
                CHECK_COMP(DFILE_WRITE);
                

                CHECK_COMP(PRINT);
                CHECK_COMP(STRING);
                CHECK_COMP(STRING_COPY);
                CHECK_COMP(IB_SET_TARGET);
                CHECK_COMP(IB_WRITE_STRING);
                CHECK_COMP(IB_STOP);
                CHECK_COMP(TEXTINPUT_START);
                CHECK_COMP(TEXTINPUT_STOP);

                CHECK_COMP(DEBUG_BREAK);
                CHECK_COMP(ERROR_CLEAR);

                PRINT_COMPILER_ERROR("Unknown symbol [%s];\n", fileRead.c_str());
                return 1;

        } // Main loop

        NOVERB_PRINT_COMPILER("\tSuccessfully compiled source file [%s].\n", inputFile);
        file.close();

        if(createSymbolFile) { 
                symbolFile << "\nEXIT";
                symbolFile.close();
        }

        if(!ignoreInstructionData) {

                if(ifLocations.size() != endLocations.size()) {
                        PRINT_COMPILER_ERROR("Unequal IF and END statements.\n");
                        return 1;
                }
        
                size_t labelIndex = 0;

                // Match GOTOs to LABELs.
                for(size_t gotoIndex = 0; gotoIndex < gotoList.size(); ++gotoIndex) {
                        for(labelIndex = 0; labelIndex < labelList.size(); ++labelIndex) {

                                if(gotoList[gotoIndex].points_to == labelList[labelIndex].ID) {

                                        uint32_t* location = (decltype(location))nodeList[gotoList[gotoIndex].goto_position].access.data;
                                        *location = labelList[labelIndex].label_position;
                                        break;
                                }
                                
                        }
                        if(labelIndex == labelList.size()) {
                                PRINT_DEBUG_ERROR("Failed to link GOTO [%zu] to LABEL block. Broken GOTO created.\n", gotoList[gotoIndex].goto_position);
                                return 1;
                        }
                }

                
                // Match IFs, ENDs, and ELSEs
                {

                unsigned int numberOfIfsFound = 1;
                unsigned int numberOfEndsFound = 0;
                unsigned int finalEndIndex = 0;
                unsigned int finalElseIndex = 0;
                int waitForElse = 0;
                bool matchedElse = false;
                uint32_t* endIndex = nullptr;
                uint32_t* elseIndex = nullptr;

                for (size_t i = 0; i < ifLocations.size(); i++) {
                        NOVERB_PRINT_COMPILER("Checking IF [%zu]\n", ifLocations[i]);
                        for (finalEndIndex = ifLocations[i] + 1; numberOfIfsFound != numberOfEndsFound && finalEndIndex < nodeList.size(); ++finalEndIndex) {

                                // If an IF statement is found before and END statement, the program requires that many more ENDs
                                // to break the loop. The corresponding END will be the one found when there are equal IFs and ENDs found.
                                if (nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::END) {
                                        ++numberOfEndsFound;
                                        --waitForElse;
                                        continue;
                                }
                                if (    nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_EQU ||
                                        nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_NOT ||
                                        nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_GRT ||
                                        nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_LST ||
                                        nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_GRTE ||
                                        nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_LSTE ||
                                        nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::LOGIC_IF_TRUE
                                )
                                {
                                        ++numberOfIfsFound;
                                        ++waitForElse;
                                        continue;
                                }
                                if((nodeList[finalEndIndex].access.ID == nthp::script::instructions::ID::ELSE) && (waitForElse == 0)) {
                                        if(matchedElse) { PRINT_COMPILER_ERROR("Duplicate unmatched ELSE found while evaluating IF [%zu].\n", ifLocations[i]); return 1; }

                                        finalElseIndex = finalEndIndex;
                                        matchedElse = true;
                                        continue;
                                }

                        }
                        if(finalEndIndex == nodeList.size()) {
                                PRINT_COMPILER_ERROR("Unable to find matching END. The only reason this would print is if you put an END before an IF. Absolute moron.\n");
                                return 1;
                        }

                        // Important, not a mistake. Don't go blaming this 3 months from now.
                        --finalEndIndex;

                        if(!(matchedElse)) { finalElseIndex = 0; }
                        else {
                                // No need to worry; ELSE data is allocated when the symbol is compiled.
                                (*(uint32_t*)nodeList[finalElseIndex].access.data) = finalEndIndex; // Stores the END location in the ELSE for easy redirection.

                        }


                        
                        // Assigns the pointer to the last 4 bytes of the node to store the end index (Unless a BNE instruction).
                        if(nodeList[ifLocations[i]].access.ID == nthp::script::instructions::ID::LOGIC_IF_TRUE) {
                                endIndex = (uint32_t*)(nodeList[ifLocations[i]].access.data + sizeof(nthp::script::instructions::stdRef));
                                elseIndex = (uint32_t*)(nodeList[ifLocations[i]].access.data + sizeof(nthp::script::instructions::stdRef) + sizeof(uint32_t));

                        }
                        else {
                                endIndex = (uint32_t*)(nodeList[ifLocations[i]].access.data + sizeof(nthp::script::instructions::stdRef) + sizeof(nthp::script::instructions::stdRef));
                                elseIndex = (uint32_t*)(nodeList[ifLocations[i]].access.data + sizeof(nthp::script::instructions::stdRef) + sizeof(nthp::script::instructions::stdRef) + sizeof(uint32_t));
                        }

                        NOVERB_PRINT_COMPILER("Matched IF to END at [%u].\n", finalEndIndex);


                        *endIndex = finalEndIndex;
                        *elseIndex = finalElseIndex;
                        numberOfIfsFound = 1;
                        numberOfEndsFound = 0;
                        waitForElse = 0;
                        matchedElse = false;

                } // For

                }

                {
                        uint32_t skip_endLocation = 0;
                        uint32_t* skipEndWrite = NULL;

                       // Match SKIPs and SKIP_ENDs.
                        for(size_t i = 0; i < skipList.size(); ++i) {
                                for(skip_endLocation = skipList[i]; (nodeList[skip_endLocation].access.ID != nthp::script::instructions::ID::SKIP_END) && (skip_endLocation < nodeList.size()); ++skip_endLocation) 
                                {
                                        continue;
                                }

                                if(skip_endLocation == nodeList.size()) {
                                        PRINT_COMPILER_ERROR("SKIP at [%zu] has no matching SKIP_END flag.\n", skipList[i]);
                                        return 1;
                                }

                                skipEndWrite = (uint32_t*)(nodeList[skipList[i]].access.data);
                                *skipEndWrite = skip_endLocation;

                        }

                }

                // Set up header with:
                //      - Global Memory Budget (if applicable)
                //      - Label List

                if(nodeList.size() > 0) {
                        nodeList[0].access.size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + (sizeof(uint32_t) * labelList.size() * 2);
                        nodeList[0].access.data = (char*)malloc(nodeList[0].access.size);

                        if(nodeList[0].access.data == NULL) {
                                FATAL_PRINT(nthp::FATAL_ERROR::Memory_Fault, "Memory Fault in Compiler.\n");
                        }

                        uint32_t* globalmem = (decltype(globalmem))(nodeList[0].access.data);
                        uint32_t* labelSize = (decltype(labelSize))(nodeList[0].access.data + (sizeof(uint32_t)));
                        uint8_t* executionType = (decltype(executionType))(nodeList[0].access.data + (sizeof(uint32_t) + sizeof(uint32_t)));
                        uint32_t* labelstart = (decltype(labelstart))(nodeList[0].access.data + (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t)));
                        *labelSize = labelList.size();


                        *globalmem = (uint32_t)globalAlloc;
                        *executionType = executionFlags;

                        // Writes label data to header. For use in JUMP.
                        for(size_t i = 0; i < labelList.size(); ++i) {
                                labelstart[i + i] = labelList[i].ID;
                                labelstart[(i + i) + 1] = labelList[i].label_position;
                        }
                }


                if(outputFile != NULL) {
                        if(buildSystemContext)
                                return exportToFile(outputFile, &nodeList, buildSystemContext);
                        else
                                return exportToFile(outputFile, NULL, buildSystemContext);
                }
        }
        
        return 0;

}

int nthp::script::CompilerInstance::exportToFile(const char* outputFile, std::vector<nthp::script::Node>* nodeList, bool buildSystemContext) {
        std::fstream file;
        file.open(outputFile, std::ios::out | std::ios::binary);

        if(file.fail()) {
                PRINT_COMPILER_ERROR("Unable to write compiled binary to output file.\n");
                return 1;
        }

        nthp::script::Node* target = nodeList->data();
        size_t cont_size = nodeList->size();
        if(nodeList != NULL) {
                target = nodeList->data();
                cont_size = nodeList->size();
        }

        for(size_t i = 0; i < cont_size; ++i) {
                file.write((char*)&target[i], sizeof(nthp::script::Node::n_file_t));
                if(target[i].access.size != 0) file.write(target[i].access.data, target[i].access.size);
        }

        file.close();

        return 0;
}



int nthp::script::CompilerInstance::compileStageConfig(const char* stageConfigFile, std::vector<std::string>* targetList, bool forceBuild, const bool ignoreInstructionData) {
        std::fstream file(stageConfigFile, std::ios::in);
        if(file.fail()) {
                PRINT_COMPILER_ERROR("Failed to compile StageConfig [%s]; File not found.\n", stageConfigFile);
                return 1;
        }

        clean();

        std::string fileRead;
        bool operationComplete = false;
        PRINT_COMPILER("Building Script System [%s]: force=%u ignore=%u\n\n", stageConfigFile, forceBuild, ignoreInstructionData);

        
        // Add constant runtime globals.
        addGlobalDef("null",            "predefined");
        addGlobalDef("deltaTime",       "predefined");
        addGlobalDef("mouse1",          "predefined");
        addGlobalDef("mouse2",          "predefined");
        addGlobalDef("mouse3",          "predefined");
        addGlobalDef("r_poll1",         "predefined");
        addGlobalDef("r_poll2",         "predefined");
        addGlobalDef("r_poll3",         "predefined");
        addGlobalDef("r_poll4",         "predefined");

        while(!operationComplete) {

        BS_BEGIN: // Love, do not hate
        
                file >> fileRead;

                if(fileRead == "BUILD_SYSTEM") {


                        while(!file.eof()) {
                                file >> fileRead;
                                if(fileRead == "END") { break; }

                                {
                                        std::string output = fileRead;
                                        auto ext = output.find_last_of('.');
                                        output.erase(output.begin()+ext, output.end());

                                        output += ".thpcs";

                                        std::string type;
                                        file >> type;

                                        uint8_t execFlags = 0;

                                        // Originally an do while (0) for a more efficient 'if else' but I decided it could be useful for scripts to have
                                        // multiple execution flags (i.e a script could run once in the INIT phase and then once again in the EXIT phase, without any
                                        // fucky function shit). So the do while(0) here is pointless.
                                        do {
                                                if(type == "T_INIT") {  execFlags |= (1 << nthp::script::CompilerInstance::TriggerBits::T_INIT);}
                                                if(type == "T_TICK") {  execFlags |= (1 << nthp::script::CompilerInstance::TriggerBits::T_TICK); }
                                                if(type == "T_EXIT") {  execFlags |= (1 << nthp::script::CompilerInstance::TriggerBits::T_EXIT);}
                                                if(type == "T_HIDDEN") { execFlags |= (1 << nthp::script::CompilerInstance::TriggerBits::T_HIDDEN);}
                                        } while(0);

                                        

                                        // Ignore output file of compilation; no instructions to write.
                                        if(ignoreInstructionData) {
                                                if(compileSourceFile(fileRead.c_str(), NULL, true, execFlags, ignoreInstructionData, false)) {
                                                        PRINT_DEBUG_ERROR("Compiler failure in source file [%s]; aborting.\n", fileRead.c_str());
                                                        return 1;
                                                }
                                        }
                                        else {
                                                if(compileSourceFile(fileRead.c_str(), output.c_str(), true, execFlags, false, false)) {
                                                        if(forceBuild) {
                                                                PRINT_DEBUG_WARNING("Compiler failure in source file [%s]; forcing continue...\n", fileRead.c_str());
                                                        }
                                                        else {
                                                                PRINT_DEBUG_ERROR("Compiler failure in source file [%s]; aborting.\n", fileRead.c_str());
                                                                return 1;
                                                        }
                                                }
                                                PRINT_COMPILER("Target Script execflags = [%02zX]\n", execFlags);
                                        }

                                        if(targetList != NULL) targetList->push_back(output);
                                }
                        }
                        


                } // if (BUILD_SYSTEM)

                // Include a pre-built translation unit without any symbols. Note that the execution order is defined in the unit,
                // not the build system.
                if(fileRead == "INCLUDE") {
                        READ_FILE();
                        PRINT_COMPILER("Included unit [%s] into build.\n", fileRead.c_str());

                        if(targetList != NULL) targetList->push_back(fileRead);
                        continue;
                }


                if(fileRead == "MODULE") {
                        READ_FILE();

                        std::string mod = fileRead;
                        std::string symbol = fileRead;

                        auto ext = symbol.find_last_of('.');
                        if(ext != std::string::npos)
                                symbol.erase(symbol.begin()+ext, symbol.end());

                        symbol += ".sym";

                        PRINT_COMPILER("Importing module [%s]...\n", mod.c_str());


                        if(compileSourceFile(symbol.c_str(), NULL, true, 1 << nthp::script::CompilerInstance::TriggerBits::T_NONE, true, false)) {
                                PRINT_COMPILER_ERROR("Failed to import symbol data for module [%s].\n", symbol.c_str());
                                return 1;
                        }
                        PRINT_COMPILER("Successfully imported symbols.\n");

                        if(targetList != NULL ) {
                                targetList->push_back(mod);
                                PRINT_COMPILER("Included module unit [%s] into build.\n", mod.c_str());
                        }

                        continue;
                }

                if(fileRead == "CONST") {
                        file >> fileRead;

                        nthp::script::CompilerInstance::CONST_DEF newConst;
                        newConst.constName = "#" + fileRead;

                        file >> fileRead;
                        newConst.value = fileRead;

                        if(fileRead == newConst.constName) {
                                PRINT_COMPILER_ERROR("CONST [%s] cannot substitute itself.\n", newConst.constName.c_str());
                                return 1;
                        }

                        constantList.push_back(newConst);
                        PRINT_COMPILER("BS CONST defined; n=%s sub=%s\n", newConst.constName.c_str(), newConst.value.c_str());

                        continue;
                }

                if(fileRead == "MACRO") {
                         // Define new Macro.
                        READ_FILE();        // Name
                        MACRO_DEF newDef;

                        newDef.macroName = '@' + fileRead;


                        for(size_t i = 0; i < macroList.size(); ++i) {
                                if(macroList[i].macroName == newDef.macroName) {
                                        PRINT_COMPILER_WARNING("Duplicate MACRO [%s] at [~%zu]; Ignoring Definition.\n", macroList[i].macroName.c_str(), nodeList.size());
                                        do {file >> fileRead; } while(fileRead != "}");
                                        goto BS_BEGIN;
                                }
                        }

                        PRINT_COMPILER("Defining new BS MACRO [%s]...", fileRead.c_str());

                        READ_FILE();     // Gets rid of the '{'
                        for(size_t i = 0; fileRead != "}"; ++i) {
                                READ_FILE();
                                if(fileRead == "/") { do { READ_FILE(); } while(fileRead != "/"); READ_FILE(); }
                                newDef.macroData.push_back(fileRead);
                                
                                if((i % 5) == 0) { NOVERB_PRINT_COMPILER("\n\t"); }

                                NOVERB_PRINT_COMPILER(" [%s]", fileRead.c_str());
                        }
                        newDef.macroData.pop_back(); // Remove the '}'
                        macroList.push_back(newDef);

                        NOVERB_PRINT_COMPILER("\n");
                        PRINT_COMPILER("Added MACRO [%s] to MACRO list.\n", macroList.back().macroName.c_str());
                        continue;
                }


                if(fileRead == "STRUCT") {
                        structList.push_back(STRUCT_DEF());
                        READ_FILE();

                        PRINT_COMPILER("Defining BS STRUCT [%s]...\n", fileRead.c_str());

                        structList.back().name = fileRead;
                        
                        READ_FILE();
                        if(fileRead == "{") {
                                READ_FILE();

                                while(fileRead != "}") {
                                        structList.back().members.push_back(fileRead);
                                        PRINT_COMPILER("\tAdded entry [%s] at [%02zX],\n", fileRead.c_str(), structList.back().members.size() - 1);
                                        READ_FILE();
                                }

                                PRINT_COMPILER("Finished definition of STRUCT [%s].\n", structList.back().name.c_str());
                        }
                        else {
                                PRINT_COMPILER_ERROR("STRUCT cannot be defined; scope required.\n");
                                structList.pop_back();
                                continue;
                        }
                }




                if(fileRead == "EXIT" || file.eof()) {
                        operationComplete = true;
                }
        } // while(!operationComplete)

        file.close();


        return 0;

}




int nthp::script::CompilerInstance::buildModule(const char* source) {
        clean();

        std::string outputFile = source;
        auto ext = outputFile.find_last_of('.');
        
        if(ext != std::string::npos)
                outputFile.erase(outputFile.begin()+ext, outputFile.end());

        outputFile += ".mod";

        // Add constant runtime globals.
        addGlobalDef("null",            "predefined");
        addGlobalDef("deltaTime",       "predefined");
        addGlobalDef("mouse1",          "predefined");
        addGlobalDef("mouse2",          "predefined");
        addGlobalDef("mouse3",          "predefined");
        addGlobalDef("r_poll1",         "predefined");
        addGlobalDef("r_poll2",         "predefined");
        addGlobalDef("r_poll3",         "predefined");
        addGlobalDef("r_poll4",         "predefined");


        if(compileSourceFile(source, outputFile.c_str(), true, 1 << nthp::script::CompilerInstance::TriggerBits::T_MODULE, false, true)) {
                PRINT_DEBUG_ERROR("Failed to build module [%s]; compiler failure.\n", source);
                return 1;
        }

        return 0;
}






nthp::script::CompilerInstance::~CompilerInstance() {
        clean();
}
