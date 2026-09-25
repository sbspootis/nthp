#ifdef WINDOWS



#include "pm_editor.hpp"
using namespace nthp::pm::editor;


int Project::newEmptyProject() {
        projectName = "new_project1";
        sceneList.clear();

        sceneList.push_back(Scene("scene1"));
        currentScene = 0;
        
}


int Project::importNewTexture(std::string filename, std::string nameInternal) {
        sceneList[currentScene].textureList.push_back(textureObject());
        if(sceneList[currentScene].textureList.back().texture.autoLoadTextureFile(filename.c_str(), &nthp::script::activePalette, nthp::core.getRenderer())) {
                return 1;
        }

        sceneList[currentScene].textureList.back().identifier.constName = "#" + nameInternal;
        sceneList[currentScene].textureList.back().identifier.value = std::to_string(sceneList[currentScene].textureList.size() - 1);
        sceneList[currentScene].textureList.back().singleTextureFrame.texture = sceneList[currentScene].textureList.back().texture.getTextureData().texture;
        sceneList[currentScene].textureList.back().singleTextureFrame.src = { 0, 0, (int)sceneList[currentScene].textureList.back().texture.getTextureData().getMetaData().x, (int)sceneList[currentScene].textureList.back().texture.getTextureData().getMetaData().y };

        return 0;
}









#endif