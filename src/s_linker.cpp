#include "s_linker.hpp"




nthp::script::LinkerInstance::LinkerInstance(std::vector<std::string>& targets, const char* output) {
        if(linkFiles(targets, output)) {
                PRINT_DEBUG_ERROR("Failed to link compiled source files.\n");

        }


}


int nthp::script::LinkerInstance::linkFiles(std::vector<std::string>& targets, const char* output) {
        const size_t scriptDataSize = targets.size();
        try {
                scriptData = new std::vector<nthp::script::Node>[scriptDataSize];
        } 
        catch(std::bad_alloc x) {
                PRINT_DEBUG_ERROR("Failed to allocate scriptData in linker [%p]; %s\n", this, x.what());
                return 1;
        }


        std::fstream file;
        nthp::script::Node read;
        uint32_t finalSize = 0;
        
        for(size_t cObject = 0; cObject < scriptDataSize; ++cObject) {
                PRINT_DEBUG("Importing file [%s]...\n", targets[cObject].c_str());
                file.open(targets[cObject], std::ios::in | std::ios::binary);

                if(file.fail()) {
                        PRINT_DEBUG_ERROR("Unable to access file [%s] to link.\n", targets[cObject].c_str());
                        return 1;
                }

                
                for(size_t i = 0; read.access.ID != GET_INSTRUCTION_ID(EXIT); ++i) {
                        ++finalSize;

                        file.read((char*)&read, sizeof(nthp::script::Node::n_file_t));
                        scriptData[cObject].push_back(read);
                        NOVERB_PRINT_DEBUG("\t[%zu] ID:%u Size:%u\n", i, read.access.ID, read.access.size);
                        
                        if(scriptData[cObject].back().access.size != 0) {
                                scriptData[cObject].back().access.data = (char*)malloc(scriptData[cObject].back().access.size);
                                file.read(scriptData[cObject].back().access.data, scriptData[cObject].back().access.size);
                        }

                        else scriptData[cObject].back().access.data = nullptr;

                }

                read.access.ID = GET_INSTRUCTION_ID(HEADER);
                file.close();
        }

        nthp::script::LinkerInstance::link_header header;
        header.scriptCount = scriptDataSize;
        header.totalNodeCount = finalSize;

        


        // Copy all definitions to a single buffer
        PRINT_DEBUG("Copying structure to new container... ");

        nthp::script::Node* tempStorage = new nthp::script::Node[header.totalNodeCount];

        size_t counter = 0;
        for(size_t cObject = 0; cObject < scriptDataSize; ++cObject) {
                for(size_t i = 0; i < scriptData[cObject].size(); ++i) {
                        tempStorage[counter] = scriptData[cObject].data()[i];
                        ++counter;
                }
        }
        
        NOVERB_PRINT_DEBUG("Done.\n");

        // Find and match headers for each FUNC!
        for(uint32_t i = 0; i < header.totalNodeCount; ++i) {
                if(tempStorage[i].access.ID == GET_INSTRUCTION_ID(FUNC_START)) {
                        uint32_t* headerLocation = (uint32_t*)(tempStorage[i].access.data + sizeof(uint32_t));

                        uint32_t finder = i;
                        for(; tempStorage[finder].access.ID != GET_INSTRUCTION_ID(HEADER); --finder);

                        *headerLocation = finder;
                }
        }


        // Match FUNC defs
        for(uint32_t i = 0; i < header.totalNodeCount; ++i) {
                if(tempStorage[i].access.ID == GET_INSTRUCTION_ID(FUNC_CALL)) {

                        // Match FUNC_CALL to corresponding FUNC_START!
                        for(uint32_t j = 0; j < header.totalNodeCount; ++j) {
                                if(tempStorage[j].access.ID == GET_INSTRUCTION_ID(FUNC_START)) {
                                        uint32_t* callID = (uint32_t*)(tempStorage[i].access.data);
                                        uint32_t* startID = (uint32_t*)(tempStorage[j].access.data);



                                
                                        if((*callID) == (*startID)) {
                                                // Matched! Set the absolute position (j) in callID!
                                                // The -1 allows the FUNC_START node to be executed, which sets up
                                                // the new header location.
                                                *callID = j - 1;
                                                break;
                                        }
                                }
                        }

                }
        }


        file.open(output, std::ios::out | std::ios::binary);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Unable to open file [%s].\n", output);
                return 1;
        }


        PRINT_DEBUG("Writing executable to file... ");
        // Write linked data to a new file.
        file.write((char*)&header, sizeof(header));


        for(size_t i = 0; i < header.totalNodeCount; ++i) {
                file.write((char*)&tempStorage[i], sizeof(nthp::script::Node::n_file_t));
                if(tempStorage[i].access.size != 0) file.write(tempStorage[i].access.data, tempStorage[i].access.size);
        }

        file.close();

        NOVERB_PRINT_DEBUG("Done.\n");


        // NOTE that the loaded script data is only freed here, but allocated twice!
        // This is because the data block of each node is SHARED, not COPIED!
        // Keep this in mind to avoid a double-free.
        //
        // The scriptData vector is freed in the destructor.
        nthp::script::cleanNodeSet(tempStorage, header.totalNodeCount);
        

        return 0;
}



nthp::script::LinkerInstance::~LinkerInstance() {
        delete[] scriptData;
}