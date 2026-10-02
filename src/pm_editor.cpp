#include "pm.hpp"
#include "pm_editor.hpp"


using namespace nthp::pm;


// A graphical interface for initializing some kind of entity list.
// Will allow for -
//      creation and manipulation of entities
//      setting layering and other things
//      testing textures

// The program will generate a T_HIDDEN translation unit that will contain program
// data to set up the entities and textures imported and created in the editor,
// as well as the unit's symbols so they can be included in other scripts.






bool mouse1;
bool mouse2;
bool escapePressed;
bool wKey, sKey, aKey, dKey, gKey;

bool inEditor = false;

editor::Project currentProject;



void eventHandler(SDL_Event* eventList) {
        ImGui_ImplSDL2_ProcessEvent(eventList);
        switch(eventList->type) {
                case SDL_KEYDOWN:
                        if(eventList->key.keysym.sym == SDLK_w) {
                                wKey = true;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_s) {
                                sKey = true;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_a) {
                                aKey = true;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_d) {
                                dKey = true;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_g) {
                                gKey = true;
                        }
                        if(eventList->key.keysym.sym == SDLK_ESCAPE) {
                                escapePressed = true;
                        }

                        break;
                case SDL_KEYUP:
                        if(eventList->key.keysym.sym == SDLK_w) {
                                wKey = false;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_s) {
                                sKey = false;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_a) {
                                aKey = false;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_d) {
                                dKey = false;
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_g) {
                                gKey = false;
                        }
                        if(eventList->key.keysym.sym == SDLK_ESCAPE) {
                                escapePressed = false;
                        }

                        break;

                case SDL_MOUSEBUTTONDOWN:
                        if(eventList->button.button == SDL_BUTTON_LEFT) {
                                mouse1 = true;
                                break;
                        }
                        if(eventList->button.button == SDL_BUTTON_RIGHT) {
                                mouse2 = true;
                                break;
                        }
                        break;

                case SDL_MOUSEBUTTONUP:
                        if(eventList->button.button == SDL_BUTTON_LEFT) {
                                mouse1 = false;
                                break;
                        }
                        if(eventList->button.button == SDL_BUTTON_RIGHT) {
                                mouse2 = false;
                                break;
                        }
                        break;
                case SDL_WINDOWEVENT:
                        if(eventList->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                                nthp::core.p_coreDisplay.pxlResolution_x = eventList->window.data1;
                                nthp::core.p_coreDisplay.pxlResolution_y = eventList->window.data2;

                                nthp::core.p_coreDisplay.updateScaleFactor();
                        }
                        break;

        }

}




int nthp::pm::editor::editorRuntime() {


        if(nthp::core.init(nthp::RenderRuleSet(1080, 609, nthp::intToFixed(800), nthp::intToFixed(800), nthp::vectFixed(0,0)), "PM Editor", false, false, true)) {
                return 1;
        }
        SDL_StartTextInput();


        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
        

        ImGui::StyleColorsDark();
        ImGui_ImplSDLRenderer2_Init(nthp::core.getRenderer());
        ImGui_ImplSDL2_InitForSDLRenderer(nthp::core.getWindow(), nthp::core.getRenderer());

        // ==========================================================================================================


        nthp::texture::text::RenderText mouseXPosition;



        nthp::setMaxFPS(45);



        bool flashSelectedObject = false;



        char inputString_1[50] = {0};
        char inputString_2[50] = {0};
        std::string windowTitle;
        int sceneChange = 0;
        bool grabSelectedEntity = false;

        nthp::entity::cRect mouseRect;
        mouseRect.w = nthp::intToFixed(1);
        mouseRect.h = nthp::intToFixed(1);

        nthp::vectf64 entityPositionField;
        nthp::vectf64 entityRenderSizeField;
        nthp::vectf64 entityHitboxSizeField;
        nthp::vectf64 entityHitboxOffsetField;

        int x,y;

        typedef enum { DISABLED, PLACE, SELECT, EDIT, DELETE } cursormode;
        int cursorMode = cursormode::DISABLED;
        int cursorPlaceSchematic = -1;


        std::chrono::steady_clock tickTimer;
        std::chrono::microseconds frameTime;
        auto frameStart = tickTimer.now();


        // Shown window states.

        bool win_sceneControl = false;                          // control menu
        bool win_setActivePalette = false;

        bool win_newProjectConfig = false;
        bool win_openProjectConfig = false;

        bool win_deleteCurrentScene = false;

        bool win_loadTextureFile = false;
        bool win_textureList = false;                           // control menu

        bool win_editFrameset = false;
        bool win_framesetList = false;                          // control menu

        bool win_editSchematic = false;
        bool win_schematicList = false;                         // control menu
        unsigned int selectedSchematic = 0;

        bool win_editSelectedEntity = false;

        bool popup_confirmRegen = false;
        unsigned int selectedFrameset = 0;

        nthp::vectFixed nudgeStep = nthp::vectFixed(nthp::intToFixed(1), nthp::intToFixed(1));

        
        
        
        // Anyone would agree an infinite loop here is acceptable.
        while(true) {
                

                
                while(nthp::core.isRunning()) {
                        frameStart = tickTimer.now();

                        nthp::core.handleEvents(eventHandler);
                        SDL_GetMouseState(&x, &y);
                        mouseRect.x = nthp::mousePosition.x;
                        mouseRect.y = nthp::mousePosition.y;

                        ImGui_ImplSDLRenderer2_NewFrame();
                        ImGui_ImplSDL2_NewFrame();
                        ImGui::NewFrame();

                        
                        if(ImGui::BeginMainMenuBar()) {
                                if(ImGui::BeginMenu("Project")) {
                                        if (ImGui::MenuItem("New Project")) { 
                                                // New Project code.
                                                win_newProjectConfig = true;
                                                inputString_1[0] = '\000';
                                                inputString_2[0] = '\000';
                                        }
                                        if (ImGui::MenuItem("Open Project")) { 
                                                // Open Project code.
                                        }
                                        ImGui::Separator();
                                        if (ImGui::MenuItem("Exit")) { 
                                                nthp::core.stop();
                                        }
                                        ImGui::EndMenu();
                                }
                                if(ImGui::BeginMenu("Scene")) {
                                        
                                        if(ImGui::MenuItem("Show Scene Control", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                win_sceneControl = true;
                                        }
                                        ImGui::Separator();
                                        if(ImGui::MenuItem("New Scene", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                currentProject.addNewScene();
                                                win_sceneControl = true;
                                        }
                                        if(ImGui::MenuItem("Delete Scene", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                               
                                        }
                                        

                                        ImGui::EndMenu();
                                }
                                if(ImGui::BeginMenu("Texture")) {
                                        if(ImGui::MenuItem("Show Texture List", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                win_textureList = true;
                                        }
                                        ImGui::Separator();
                                        if(ImGui::MenuItem("Set Active Palette", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                win_setActivePalette = true;
                                                inputString_1[0] = '\000';
                                        }
                                        if(ImGui::MenuItem("Load Texture File", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                win_loadTextureFile = true;
                                                inputString_1[0] = '\000';
                                                inputString_2[0] = '\000';
                                        }
                                        if(ImGui::MenuItem("Regenerate Scene Textures", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                popup_confirmRegen = true;
                                        }

                                        ImGui::EndMenu();
                                }
                                if(ImGui::BeginMenu("Animation")) {

                                        if(ImGui::MenuItem("Show Frameset List", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                win_framesetList = true;
                                        }
                                        ImGui::Separator();
                                        if(ImGui::MenuItem("Create Frameset", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                currentProject.activeScene().addNewFrameset();
                                                win_editFrameset = true;
                                                selectedFrameset = currentProject.activeScene().frameSetList.size() - 1;
                                        }


                                        ImGui::EndMenu();
                                }

                                if(ImGui::BeginMenu("Entity")) {

                                        if(ImGui::MenuItem("Show Entity List", NULL, (bool*)nullptr, currentProject.allowModification)) {

                                        }
                                        if(ImGui::MenuItem("Show Schematic List", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                win_schematicList = true;
                                        }
                                        ImGui::Separator();
                                        if(ImGui::MenuItem("Create Entity Schematic", NULL, (bool*)nullptr, currentProject.allowModification)) {
                                                currentProject.activeScene().createSchematic(std::string("newShematic") + std::to_string(currentProject.activeScene().schematicList.size()));
                                                selectedSchematic = currentProject.activeScene().schematicList.size() - 1;
                                                win_editSchematic = true;
                                        }

                                        ImGui::EndMenu();
                                }
                                
                                ImGui::EndMainMenuBar();
                        }


                        // Mutually exclusive block with cascading priority. They all share input strings, so be warned.
                        do {
                        if(popup_confirmRegen) {
                                if(ImGui::Begin("Confirm Texture Regen", &popup_confirmRegen)) {
                                        ImGui::Text("Regenerate all texture currently loaded in the scene with the active palette?");
                                        if(ImGui::Button("Confirm")) {
                                                currentProject.activeScene().regenAllTextures();
                                                popup_confirmRegen = false;
                                        }
                                        ImGui::SameLine();
                                        if(ImGui::Button("Cancel")) {
                                                popup_confirmRegen = false;
                                        }
                                        
                                }
                                ImGui::End();
                                break;
                        }

                        if(win_newProjectConfig) {
                                ImGui::Begin("New Project", &win_newProjectConfig);

                                ImGui::Text("Create new empty project");
                                ImGui::InputTextWithHint("##Project Name", "Project Name", inputString_1, 50);
                                ImGui::InputTextWithHint("##Source Location", "Source Location", inputString_2, 50);
                                if(ImGui::Button("Create")) {
                                        currentProject.newEmptyProject(inputString_1, inputString_2);
                                        SDL_SetWindowTitle(nthp::core.getWindow(), std::string(std::string("PM Editor - ") + std::string(inputString_1)).c_str());
                                        win_newProjectConfig = false;
                                        win_sceneControl = true;
                                        cursorMode = cursormode::SELECT;
                                }
                                ImGui::End();
                                break;
                        }

                        if(win_setActivePalette) {
                                ImGui::Begin("Set Active Palette", &win_setActivePalette);

                                ImGui::Text("Import palette file");
                                ImGui::InputTextWithHint("##Palettefile", "Palette File Path", inputString_1, 50);
                                ImGui::Separator();
                                if(ImGui::Button("Import")) {
                                        nthp::script::activePalette.importPaletteFromFile(inputString_1);
                                        win_setActivePalette = false;
                                }

                                ImGui::End();
                                break;
                        }

                        if(win_loadTextureFile) {
                                ImGui::Begin("Load Texture File", &win_loadTextureFile);

                                ImGui::Text("Add texture file to current scene [%d]", currentProject.currentScene);
                                ImGui::InputTextWithHint("##texturename", "Name", inputString_1, 50);
                                ImGui::InputTextWithHint("##textureFile", "Texture File Path", inputString_2, 50);
                                ImGui::Separator();
                                if(ImGui::Button("Add")) {
                                        currentProject.activeScene().importNewTexture(inputString_2, inputString_1);
                                        win_loadTextureFile = false;
                                }

                                ImGui::End();
                                break;
                        }

                
                

                        } while(0);

                        if(win_sceneControl) {
                                ImGui::Begin("Scene Control", &win_sceneControl);
                                ImGui::Text("Currently loaded scene ID: [%d]", currentProject.currentScene);
                                ImGui::Separator();
                                if(ImGui::ArrowButton("prev" ,ImGuiDir::ImGuiDir_Left)) {
                                        if(currentProject.currentScene)
                                                --currentProject.currentScene;
                                }
                                ImGui::SameLine();
                                if(ImGui::InputInt("##Current Scene", &currentProject.currentScene, 0, 0)) {
                                        if(currentProject.currentScene >= currentProject.sceneList.size()) currentProject.currentScene = currentProject.sceneList.size() - 1;
                                }
                                ImGui::SameLine();
                                if(ImGui::ArrowButton("next", ImGuiDir::ImGuiDir_Right)) {
                                        ++sceneChange;
                                        if(currentProject.currentScene >= currentProject.sceneList.size()) currentProject.currentScene = currentProject.sceneList.size() - 1;
                                }

                                ImGui::Separator();
                                ImGui::Text("Current Scene Info:");
                                ImGui::Text("Name: "); ImGui::SameLine();
                                ImGui::InputText("##scenename", &currentProject.sceneList[currentProject.currentScene].name);
                                ImGui::Text("Object Count: %d total", 
                                        currentProject.activeScene().entityList.size() + 
                                        currentProject.activeScene().textureList.size() +
                                        currentProject.activeScene().frameSetList.size());
                                ImGui::Text("Schema count: %zu", currentProject.activeScene().schematicList.size());
                                ImGui::Text("Entity count: %zu", currentProject.activeScene().entityList.size());
                                ImGui::End();
                        }

                        if(win_editFrameset) {
                                do {
                                if(selectedFrameset >= currentProject.activeScene().frameSetList.size() || selectedFrameset < 0) { win_editFrameset = false; break; }
                                if(ImGui::Begin("Edit Frameset", &win_editFrameset)) {
                                        
                                        ImGui::Text("Frameset Index %d: %s, ID=%s", selectedFrameset, currentProject.activeScene().frameSetList[selectedFrameset].identifier.constName.c_str(), currentProject.activeScene().frameSetList[selectedFrameset].identifier.value.c_str());
                                        ImGui::Text("Name: "); ImGui::SameLine(); ImGui::InputText("##framesetname", &(currentProject.activeScene().frameSetList[selectedFrameset].identifier.constName));
                                        ImGui::Separator();
                                        ImGui::Text("Target Texture Name: "); 
                                        ImGui::SameLine(); 
                                        ImGui::SetNextItemWidth(80);
                                        if(ImGui::InputText("##targetexture", &(currentProject.activeScene().frameSetList[selectedFrameset].searchTextureName))) {
                                                bool matchedTexture = false;
                                                for(size_t i = 0; i < currentProject.activeScene().textureList.size(); ++i) {
                                                        if(currentProject.activeScene().textureList[i].identifier.constName ==  currentProject.activeScene().frameSetList[selectedFrameset].searchTextureName) {
                                                                currentProject.activeScene().frameSetList[selectedFrameset].textureID = i;
                                                                matchedTexture = true;
                                                                break;
                                                        }
                                                }
                                                if(!matchedTexture) { currentProject.activeScene().frameSetList[selectedFrameset].textureID = -1; }
                                        }
                                        if(currentProject.activeScene().frameSetList[selectedFrameset].textureID > -1 && currentProject.activeScene().frameSetList[selectedFrameset].textureID < currentProject.activeScene().textureList.size()) {
                                                ImGui::Image((ImTextureRef)currentProject.activeScene().textureList[currentProject.activeScene().frameSetList[selectedFrameset].textureID].texture.getTextureData().getTexture(), ImVec2(64, 64));
                                        }
                                        

                                        ImGui::Separator();
                                        ImGui::Text("Frame Data (x, y, w, h):");
                                        for(size_t i = 0; i < currentProject.activeScene().frameSetList[selectedFrameset].frameSet.size(); ++i) {
                                                ImGui::PushID(i);
                                                ImGui::Text("Frame #%zu", i);
                                                ImGui::SameLine();

                                                ImGui::SetNextItemWidth(40);
                                                ImGui::InputInt("##framedatax", &(currentProject.activeScene().frameSetList[selectedFrameset].frameSet[i].x), 0, 0);
                                                ImGui::SameLine();
                                                ImGui::SetNextItemWidth(40);
                                                ImGui::InputInt("##framedatay", &(currentProject.activeScene().frameSetList[selectedFrameset].frameSet[i].y), 0, 0);
                                                ImGui::SameLine();
                                                ImGui::SetNextItemWidth(40);
                                                ImGui::InputInt("##framedataw", &(currentProject.activeScene().frameSetList[selectedFrameset].frameSet[i].w), 0, 0);
                                                ImGui::SameLine();
                                                ImGui::SetNextItemWidth(40);
                                                ImGui::InputInt("##framedatah", &(currentProject.activeScene().frameSetList[selectedFrameset].frameSet[i].h), 0, 0);
                                                ImGui::SameLine();
                                                if(ImGui::Button("Delete")) {
                                                        ImGui::OpenPopup("confirmframedelete");
                                                }
                                                if(ImGui::BeginPopup("confirmframedelete")) {
                                                        ImGui::Text("Delete frame #%zu?", i);
                                                        if(ImGui::Button("Confirm")) {
                                                                currentProject.activeScene().frameSetList[selectedFrameset].frameSet.erase(currentProject.activeScene().frameSetList[selectedFrameset].frameSet.begin()+i);
                                                                ImGui::CloseCurrentPopup();
                                                        }
                                                        ImGui::SameLine();
                                                        if(ImGui::Button("Cancel")) {
                                                                ImGui::CloseCurrentPopup();
                                                        }

                                                        ImGui::EndPopup();
                                                }

                                                ImGui::PopID();
                                        }

                                        if(ImGui::Button("Add Frame")) {
                                                currentProject.activeScene().frameSetList[selectedFrameset].frameSet.push_back({0,0,0,0});
                                        }
                                
                                }
                                
                                ImGui::End();
                                } while(0);      // do, while0
                                
                        }

                        if(win_textureList) {
                                if(ImGui::Begin("Texture List", &win_textureList)) {
                                        ImGui::Text("Total Textures Imported in Scene: %zu", currentProject.sceneList[currentProject.currentScene].textureList.size());
                                        if(ImGui::BeginListBox("##Texture List", ImGui::GetContentRegionAvail())) {
                                                ImGui::Separator();
                                                for(size_t i = 0; i < currentProject.sceneList[currentProject.currentScene].textureList.size(); ++i) {
                                                        auto& targetTexture = (currentProject.sceneList[currentProject.currentScene].textureList[i].texture);
                                                        const ImVec2 thumbSize(64, 64);
                                                        
                                                        ImGui::PushID(i);

                                                        ImGui::BeginGroup();
                                                        ImGui::Image((ImTextureRef)targetTexture.getTextureData().getTexture(), thumbSize);
                                                        ImGui::Text("Name: %s | ID: %s (%u x %u)", currentProject.sceneList[currentProject.currentScene].textureList[i].identifier.constName.c_str(), currentProject.sceneList[currentProject.currentScene].textureList[i].identifier.value, targetTexture.getTextureData().getMetaData().x, targetTexture.getTextureData().getMetaData().y);
                                                        ImGui::SameLine();
                                                        if(ImGui::Button("Delete")) {
                                                                ImGui::OpenPopup("confirmTextureDelete");
                                                        }
                                                        ImGui::EndGroup();


                                                        if(ImGui::BeginPopup("confirmTextureDelete")) {
                                                                ImGui::Text("Delete texture [%s], ID: %s?", currentProject.sceneList[currentProject.currentScene].textureList[i].identifier.constName.c_str(), currentProject.sceneList[currentProject.currentScene].textureList[i].identifier.value.c_str());
                                                                if(ImGui::Button("Confirm")) {
                                                                        currentProject.sceneList[currentProject.currentScene].deleteTexture(i);
                                                                        ImGui::CloseCurrentPopup();

                                                                } ImGui::SameLine(); 
                                                                if(ImGui::Button("Cancel")) {
                                                                        ImGui::CloseCurrentPopup();
                                                                }

                                                                ImGui::EndPopup();
                                                        }

                                                        ImGui::Separator();

                                                        ImGui::PopID();
                                                }

                                                ImGui::EndListBox();
                                        }
                                }

                                ImGui::End();

                        }

                        if(win_framesetList) {
                                if(ImGui::Begin("Frameset List", &win_framesetList)) {
                                        ImGui::Text("Total Scene frameset Count: %zu", currentProject.activeScene().frameSetList.size());
                                        ImGui::Separator();
                                        if(ImGui::BeginListBox("##Frameset List", ImGui::GetContentRegionAvail())) {

                                                for(size_t i = 0; i < currentProject.activeScene().frameSetList.size(); ++i) {
                                                        ImGui::PushID(i);
                                                        
                                                        ImGui::Text("Frameset ID:%zu | #%s | %u total frames", i, currentProject.activeScene().frameSetList[i].identifier.constName.c_str(), currentProject.activeScene().frameSetList[i].frameSet.size());
                                                        ImGui::SameLine();
                                                        if(ImGui::Button("Edit")) {
                                                                win_editFrameset = true;
                                                                selectedFrameset = i;
                                                        }
                                                        ImGui::SameLine();
                                                        if(ImGui::Button("Delete")) {
                                                                ImGui::OpenPopup("deleteframeset");
                                                        }

                                                        if(ImGui::BeginPopup("deleteframeset")) {
                                                                ImGui::Text("Delete frameset [#%s]?", currentProject.activeScene().frameSetList[i].identifier.constName.c_str());
                                                                
                                                                if(ImGui::Button("Confirm")) {
                                                                        currentProject.activeScene().deleteFrameset(i);
                                                                        ImGui::CloseCurrentPopup();
                                                                }
                                                                if(ImGui::Button("Cancel")) {
                                                                        ImGui::CloseCurrentPopup();
                                                                }
                                                                ImGui::EndPopup();
                                                        }

                                                        ImGui::Separator();
                                                        ImGui::PopID();
                                                }



                                                ImGui::EndListBox();
                                        }
                                }

                                ImGui::End();
                        }
                        if(win_editSchematic) {
                                do {
                                if(selectedSchematic >= currentProject.activeScene().schematicList.size()) {
                                        win_editSchematic = false;
                                        break;
                                }
                                if(ImGui::Begin("Edit Entity Schematic", &win_editSchematic)) {
                                        
                                        auto& targetSchema = currentProject.activeScene().schematicList[selectedSchematic];
                                        ImGui::Text("Schema Name:");
                                        ImGui::SameLine();
                                        ImGui::InputText("##schemaname", &(targetSchema.name));
                                        ImGui::Text("Texture/Frameset target:");
                                        ImGui::SameLine();
                                        if(ImGui::InputText("##framesettarget", &targetSchema.searchFramesetName)) {
                                                bool matchedFrameset = false;
                                                for(size_t i = 0; i < currentProject.activeScene().frameSetList.size(); ++i) {
                                                        if(targetSchema.searchFramesetName == currentProject.activeScene().frameSetList[i].identifier.constName) {
                                                                targetSchema.framesetIndex = i;
                                                                matchedFrameset = true;
                                                                break;
                                                        }
                                                }
                                                if(!matchedFrameset) { targetSchema.framesetIndex = -1; }
                                        }
                                        ImGui::Text("Virtual Render Size (x,y):");
                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(150);
                                        ImGui::InputDouble("##schemarendersizex", &targetSchema.renderSize.x);
                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(150);
                                        ImGui::InputDouble("##schemarendersizey", &targetSchema.renderSize.y);
                                        ImGui::Text("Hitbox Size (x,y):");
                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(150);
                                        ImGui::InputDouble("##schemahitboxsizex", &targetSchema.hitboxSize.x);
                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(150);
                                        ImGui::InputDouble("##schemahitboxsizey", &targetSchema.hitboxSize.y);
                                        ImGui::Text("Hitbox Offset (x,y):");
                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(150);
                                        ImGui::InputDouble("##schemahitboxoffsetx", &targetSchema.hitboxOffset.x);
                                        ImGui::SameLine();
                                        ImGui::SetNextItemWidth(150);
                                        ImGui::InputDouble("##schemahitboxoffsety", &targetSchema.hitboxOffset.y);
                                }

                                ImGui::End();
                                } while(0);
                        }
                        if(win_schematicList) {
                                if(ImGui::Begin("Entity Schematic List", &win_schematicList)) {

                                        if(ImGui::BeginListBox("##schematicList", ImGui::GetContentRegionAvail())) {
                                                for(size_t i = 0; i < currentProject.activeScene().schematicList.size(); ++i) {
                                                        ImGui::PushID(i);
                                                        ImGui::Separator();
                                                        ImGui::Text("Name: %s", currentProject.activeScene().schematicList[i].name.c_str());
                                                        if(currentProject.activeScene().schematicList[i].framesetIndex >= 0) {
                                                                auto& textureData = currentProject.activeScene().textureList[currentProject.activeScene().frameSetList[currentProject.activeScene().schematicList[i].framesetIndex].textureID].texture.getTextureData();
                                                                auto& frameData = currentProject.activeScene().frameSetList[currentProject.activeScene().schematicList[i].framesetIndex];
                                                                
                                                                const ImVec2 uv0((float)frameData.frameSet[0].x / (float)textureData.getMetaData().x, (float)frameData.frameSet[0].y / (float)textureData.getMetaData().y);                  // Top-left UV
                                                                const ImVec2 uv1((float)(frameData.frameSet[0].x + frameData.frameSet[0].w) / (float)textureData.getMetaData().x, (float)(frameData.frameSet[0].y + frameData.frameSet[0].h) / (float)textureData.getMetaData().y);      // Bottom-right UV
                                                                ImGui::Image((ImTextureRef)currentProject.activeScene().textureList[currentProject.activeScene().frameSetList[currentProject.activeScene().schematicList[i].framesetIndex].textureID].texture.getTextureData().getTexture(), ImVec2(64, 64), uv0, uv1);
                                                                
                                                        }
                                                        if(ImGui::Button("+")) {
                                                                cursorMode = cursormode::PLACE;
                                                                cursorPlaceSchematic = i;
                                                                currentProject.activeScene().constructSchematicFrameset(i);
                                                        }
                                                        ImGui::SameLine();
                                                        if(ImGui::Button("Edit")) {
                                                                selectedSchematic = i;
                                                                win_editSchematic = true;
                                                        }
                                                        ImGui::SameLine();
                                                        if(ImGui::Button("Delete")) {
                                                                ImGui::OpenPopup("confirmschemadelete");
                                                        }

                                                        if(ImGui::BeginPopup("confirmschemadelete")) {
                                                                ImGui::Text("Delete Entity Schematic %s?", currentProject.activeScene().schematicList[i].name.c_str());
                                                                if(ImGui::Button("Confirm")) {
                                                                        currentProject.activeScene().deleteSchematic(i);
                                                                        ImGui::CloseCurrentPopup();
                                                                }
                                                                ImGui::SameLine();
                                                                if(ImGui::Button("Cancel")) {
                                                                        ImGui::CloseCurrentPopup();
                                                                }

                                                                ImGui::EndPopup();
                                                        }
                                                        



                                                        ImGui::PopID();
                                                }

                                        ImGui::EndListBox();
                                        }
                                }
                                ImGui::End();
                        }



                        if(!io.WantCaptureMouse) {
                                switch(cursorMode) {
                                        case PLACE:
                                        {
                                                if(mouse1) {
                                                
                                                        currentProject.activeScene().addEntity(cursorPlaceSchematic, nthp::mousePosition);
                                                        currentProject.activeScene().selectedEntity = (currentProject.activeScene().entityList.size() - 1);
                                                        mouse1 = false;
                                                }
                                                if(escapePressed) {
                                                        cursorMode = cursormode::SELECT;
                                                        escapePressed = false;
                                                }

                                        }
                                        break;

                                        case SELECT:
                                        {
                                                if(mouse1) {
                                                        
                                                        for(size_t i = 0; i < currentProject.activeScene().entityList.size(); ++i) {
                                                                if(nthp::entity::checkRectCollision(mouseRect, currentProject.activeScene().entityList[i].entity.getHitbox())) {
                                                                        currentProject.activeScene().selectedEntity = i;
                                                                        cursorMode = cursormode::EDIT;
                                                                        break;
                                                                }
                                                        }
                                                        mouse1 = false;
                                                }
                                        }
                                                break;
                                        case EDIT:
                                        {
                                                auto& selectedEntity = currentProject.activeScene().entityList[currentProject.activeScene().selectedEntity].entity;
                                                if(wKey) {
                                                        selectedEntity.move(nthp::vectFixed(0, -nudgeStep.y));
                                                        wKey = false;
                                                }
                                                if(sKey) {
                                                        selectedEntity.move(nthp::vectFixed(0, nudgeStep.y));
                                                        sKey = false;
                                                }
                                                if(aKey) {
                                                        selectedEntity.move(nthp::vectFixed(-nudgeStep.x, 0));
                                                        aKey = false;
                                                }
                                                if(dKey) {
                                                        selectedEntity.move(nthp::vectFixed(nudgeStep.x, 0));
                                                        dKey = false;
                                                }
                                                if(gKey) {
                                                        if(grabSelectedEntity) {
                                                                grabSelectedEntity = false;
                                                        }
                                                        else {
                                                                grabSelectedEntity = true;
                                                        }
                                                        gKey = false;
                                                }
                                                if(mouse1) {
                                                        for(size_t i = 0; i < currentProject.activeScene().entityList.size(); ++i) {
                                                                if(nthp::entity::checkRectCollision(mouseRect, currentProject.activeScene().entityList[i].entity.getHitbox())) {
                                                                        currentProject.activeScene().selectedEntity = i;
                                                                        cursorMode = cursormode::EDIT;
                                                                        break;
                                                                }
                                                        }
                                                        mouse1 = false;
                                                }


                                                if(escapePressed) {
                                                        cursorMode = cursormode::SELECT;
                                                        currentProject.activeScene().selectedEntity = -1;
                                                        escapePressed = false;
                                                }



                                                if(grabSelectedEntity) {
                                                        selectedEntity.setPosition(nthp::mousePosition);
                                                }


                                                break;
                                        }

                                                

                                        case DISABLED:
                                                break;
                                };
                        }





                        ImGui::Render();
                        nthp::core.clear();

                        // Only render entity list if a scene is loaded.
                        if(currentProject.allowModification) {
                                for(size_t i = 0; i < currentProject.activeScene().entityList.size(); ++i) {
                                        if(i == currentProject.activeScene().selectedEntity) {
                                                const auto renderPacket = currentProject.activeScene().entityList[currentProject.activeScene().selectedEntity].entity.getUpdateRenderPacket(&nthp::core.p_coreDisplay);
                                                SDL_SetRenderDrawColor(nthp::core.getRenderer(), 17, 255, 0, 150);
                                                SDL_RenderDrawRect(nthp::core.getRenderer(), &renderPacket.dstRect);
                                                SDL_SetRenderDrawColor(nthp::core.getRenderer(), DEFAULT_RENDER_COLOR);
                                                nthp::core.render(currentProject.activeScene().entityList[i].entity.getUpdateRenderPacket(&nthp::core.p_coreDisplay));
                                        }
                                        else  { nthp::core.render(currentProject.activeScene().entityList[i].entity.getUpdateRenderPacket(&nthp::core.p_coreDisplay)); }
                                }
                        }

                        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), nthp::core.getRenderer());

                        nthp::core.display();



                        frameTime = std::chrono::duration_cast<std::chrono::microseconds>(tickTimer.now() - frameStart);
                        nthp::deltaTime =  nthp::f_fixedProduct(nthp::deltaTime + nthp::f_fixedProduct(nthp::intToFixed(frameTime.count()), (nthp::doubleToFixed(0.001))), nthp::doubleToFixed(0.5));
                        if(nthp::deltaTime < nthp::frameDelay) {
                                std::this_thread::sleep_for(nthp::frameDelayMicroSecond - frameTime);
                                nthp::deltaTime = nthp::frameDelay;
                        }
                }
                
                
                nthp::core.cleanup();
                break;
        }


        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        return 0;
}