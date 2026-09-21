#include "pm.hpp"
#include "pm_editor.hpp"

using namespace nthp::pm;


// A graphical interface for initializing some kind of entity list.
// Will allow for -
//      creation and manipulation of entities
//      setting layering and other things
//      testing textures

// The program will generate a T_INIT translation unit that will contain program
// data to set up the entities and textures imported and created in the editor,
// as well as the unit's symbols so they can be included in other scripts.






struct entityObject {
        nthp::entity::gEntity entity;
        nthp::script::CompilerInstance::CONST_DEF identifier;
        int layer;
};

struct textureObject {
        nthp::texture::SoftwareTexture texture;
};
struct frameSetObject {
        std::vector<nthp::texture::Frame> frameSet;
};

struct objectTypeSchematic {
        unsigned int frameSet;

        nthp::vectFixed renderSize;
        nthp::vectFixed hitboxSize;
        nthp::vectFixed hitboxOffset;
        nthp::vectFixed position;
};

std::vector<entityObject> entityList;
std::vector<textureObject> textureList;
std::vector<frameSetObject> frameSetList;
std::vector<objectTypeSchematic> schematicList;







const char* textureFileFilters = "NTHP Texture File (.st)\0*.st\0All Files (*.*)\0*.*\0";
const char* projectFileFilters = "Editor Project FIles (.pmp)\0*.pmp\0All Files (*.*)\0*.*\0";

std::string filePicker(const char* filters) {
        char fileString[500] = {0};

        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = fileString;
        ofn.nMaxFile = 500;

        ofn.lpstrFilter = filters;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;

        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
        if(GetOpenFileNameA(&ofn)) {
                return std::string(fileString);
        }
        else {
                return "";
        }
}


bool mouse1;
bool mouse2;


void eventHandler(SDL_Event* eventList) {
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















enum state { START_MENU, VIEW_TEXTURE, EDIT_STAGE };
int currentState = state::START_MENU;
bool inEditor = false;








int nthp::pm::editor::editorRuntime() {

        if(nthp::core.init(nthp::RenderRuleSet(1080, 800, nthp::intToFixed(800), nthp::intToFixed(800), nthp::vectFixed(0,0)), "PM Editor", false, false)) {
                return 1;
        }


        // Start Menu Assets. ====================================================================================

        currentState = state::START_MENU;
        nthp::script::activePalette.importPaletteFromFile("resource/genericPalette.pal");
        nthp::texture::SoftwareTexture ui_startBar("editor_resource/ui_startbar.st", &nthp::script::activePalette, nthp::core.getRenderer());
        nthp::texture::Frame uiFrame;
        uiFrame.src = { 0, 0, 100, 301 };
        uiFrame.texture = ui_startBar.getTexture();

        nthp::RenderPacket renderUi = nthp::generateRenderPacket(ui_startBar.getTexture(), &uiFrame.src, {0,0,100,301}, 0, nthp::RenderPacket::C_OPERATE::ABSOLUTE);

        nthp::entity::cRect mouseRect;
        mouseRect.w = nthp::intToFixed(1);
        mouseRect.h = nthp::intToFixed(1);


        nthp::entity::cRect newStageButton(0, 0, nthp::intToFixed(100), nthp::intToFixed(100));
        nthp::entity::cRect openStageButton(0, nthp::intToFixed(100), nthp::intToFixed(100), nthp::intToFixed(100));
        nthp::entity::cRect viewTextureButton(0, nthp::intToFixed(200), nthp::intToFixed(100), nthp::intToFixed(100));

        // ==========================================================================================================

        // Editor Assets ==============================================================



        


        
        // ==========================================================================================================







        int x,y;

        std::chrono::steady_clock tickTimer;
        std::chrono::microseconds frameTime;
        auto frameStart = tickTimer.now();

        
        
        // Anyone would agree an infinite loop here is acceptable.
        while(true) {
                

                
                while(nthp::core.isRunning()) {
                        frameStart = tickTimer.now();

                        nthp::core.handleEvents(eventHandler);
                        SDL_GetMouseState(&x, &y);
                        mouseRect.x = nthp::intToFixed(x);
                        mouseRect.y = nthp::intToFixed(y);

                        switch(currentState)
                                case state::START_MENU:
                                {
                                        if(mouse1) {
                                        do {
                                                if(nthp::entity::checkRectCollision(mouseRect, newStageButton)) { currentState = state::EDIT_STAGE; break; }
                                                if(nthp::entity::checkRectCollision(mouseRect, openStageButton)) { printf("%s\n",filePicker(projectFileFilters).c_str()); break; }

                                                if(nthp::entity::checkRectCollision(mouseRect, viewTextureButton)) { printf("VIEWTEXTURE\n"); break; }
                                        }
                                        while(false);

                                        mouse1 = 0;
                                        nthp::core.clear();

                                        nthp::core.render(renderUi);

                                        nthp::core.display();
                                        break;
                                }

                                case state::EDIT_STAGE:
                                {











                                }







                                default:
                                        break;
                        }





                        


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




        return 0;
}