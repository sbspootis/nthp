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









SDL_SysWMinfo windowsInfo;

const char* textureFileFilters = "NTHP Texture File (.st)\0*.st\0Compressed Texture File (.cst)\0*.cst\0All Files (*.*)\0*.*\0";
const char* paletteFileFilters = "NTHP Texture Palette (.pal)\0*.pal\0All Files (*.*)\0*.*\0";
const char* projectFileFilters = "Editor Project Files (.ep)\0*.ep\0All Files (*.*)\0*.*\0";
const char* allFileFilter = "All Files (*.*)\0*.*\0";

std::string filePicker(const char* filters) {
        char fileString[500] = {0};

        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = windowsInfo.info.win.window;
        ofn.lpstrFile = fileString;
        ofn.nMaxFile = 500;

        ofn.lpstrFilter = filters;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;

        ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;
        if(GetOpenFileNameA(&ofn)) {
                return std::string(fileString);
        }
        else {
                return "";
        }

}




bool mouse1;
bool mouse2;

bool inEditor = false;
int cursorMode = 0;

editor::Project currentProject;



void eventHandler(SDL_Event* eventList) {
        ImGui_ImplSDL2_ProcessEvent(eventList);
        switch(eventList->type) {
                case SDL_KEYDOWN:
                        if(eventList->key.keysym.sym == SDLK_w) {
                                
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_s) {

                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_a) {
                                
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_d) {
                                break;
                        }

                        break;
                case SDL_KEYUP:
                        if(eventList->key.keysym.sym == SDLK_w) {
                                
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_s) {

                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_a) {
                                
                                break;
                        }
                        if(eventList->key.keysym.sym == SDLK_d) {
                                break;
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
        }

}




int nthp::pm::editor::editorRuntime() {


        if(nthp::core.init(nthp::RenderRuleSet(1080, 609, nthp::intToFixed(800), nthp::intToFixed(800), nthp::vectFixed(0,0)), "PM Editor", false, false)) {
                return 1;
        }


        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
        

        ImGui::StyleColorsDark();
        ImGui_ImplSDLRenderer2_Init(nthp::core.getRenderer());
        ImGui_ImplSDL2_InitForSDLRenderer(nthp::core.getWindow(), nthp::core.getRenderer());

        // ==========================================================================================================


        nthp::texture::text::RenderText mouseXPosition;



        nthp::setMaxFPS(60);














        nthp::vectFixed mouseRect;

        int x,y;

        std::chrono::steady_clock tickTimer;
        std::chrono::microseconds frameTime;
        auto frameStart = tickTimer.now();


        // Shown window states.

        bool win_setCurrentScene = false;
        bool win_setActivePalette = false;

        bool win_newProjectConfig = false;
        bool win_openProjectConfig = false;

        bool win_deleteCurrentScene = false;

        bool win_loadTextureFile = false;

        
        
        // Anyone would agree an infinite loop here is acceptable.
        while(true) {
                

                
                while(nthp::core.isRunning()) {
                        frameStart = tickTimer.now();

                        nthp::core.handleEvents(eventHandler);
                        SDL_GetMouseState(&x, &y);
                        mouseRect.x = nthp::intToFixed(x);
                        mouseRect.y = nthp::intToFixed(y);

                        ImGui_ImplSDLRenderer2_NewFrame();
                        ImGui_ImplSDL2_NewFrame();
                        ImGui::NewFrame();

                        
                        if(ImGui::BeginMainMenuBar()) {
                                if(ImGui::BeginMenu("Project")) {
                                        if (ImGui::MenuItem("New Project")) { 
                                                // New Project code.
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
                                        if(ImGui::MenuItem("New Scene")) {

                                        }
                                        if(ImGui::MenuItem("Delete Scene")) {
                                               
                                        }
                                        ImGui::Separator();

                                        if(ImGui::MenuItem("Set Current Scene")) {

                                        }

                                        ImGui::EndMenu();
                                }
                                if(ImGui::BeginMenu("Texture")) {
                                        if(ImGui::MenuItem("Show Texture List")) {

                                        }
                                        ImGui::Separator();
                                        if(ImGui::MenuItem("Set Active Palette")) {

                                        }
                                        if(ImGui::MenuItem("Load Texture File")) {

                                        }

                                        ImGui::EndMenu();
                                }
                                
                                ImGui::EndMainMenuBar();
                        }


















                        ImGui::Render();
                        nthp::core.clear();

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