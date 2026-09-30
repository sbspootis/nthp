
#include "pm_editor.hpp"
using namespace nthp::pm::editor;



int Scene::importNewTexture(std::string filename, std::string nameInternal) {
        textureList.push_back(textureObject());
        if(textureList.back().texture.autoLoadTextureFile(filename.c_str(), &nthp::script::activePalette, nthp::core.getRenderer())) {
                textureList.pop_back();
                return 1;
        }

        textureList.back().identifier.constName = "#" + nameInternal;
        textureList.back().identifier.value = std::to_string(textureList.size() - 1);
        textureList.back().singleTextureFrame.texture = textureList.back().texture.getTextureData().texture;
        textureList.back().singleTextureFrame.src = { 0, 0, (int)textureList.back().texture.getTextureData().getMetaData().x, (int)textureList.back().texture.getTextureData().getMetaData().y };

        return 0;
}

void Scene::deleteTexture(unsigned int target) {
        textureList[target].texture.clean();
        textureList.erase(textureList.begin()+target);

        for(size_t i = 0; i < textureList.size(); ++i) {
                textureList[i].identifier.value = std::to_string(i);
        }

}

void Scene::regenAllTextures() {
        for(size_t i = 0; i < textureList.size(); ++i) {
                textureList[i].texture.getTextureData().regenerateTexture(&nthp::script::activePalette, nthp::core.getRenderer());
        }
}


void Scene::addNewFrameset() {
        frameSetList.push_back(frameSetObject());
        frameSetList.back().frameSet.clear();

        frameSetList.back().identifier.constName = std::string("newFrameset") + std::to_string(frameSetList.size() - 1);
        frameSetList.back().identifier.value = std::to_string(frameSetList.size() - 1);

        frameSetList.back().textureID = -1;
        frameSetList.back().frameSet.push_back({0, 0, 0, 0});
}

void Scene::deleteFrameset(unsigned int index) {
        if(index >= frameSetList.size()) { return; }

        frameSetList.erase(frameSetList.begin()+index);
        for(size_t i = 0; i < frameSetList.size(); ++i) {
                frameSetList[i].identifier.value = std::to_string(i);
        }
}


unsigned int Scene::createSchematic(std::string name) {
        schematicList.push_back(objectTypeSchematic());

        schematicList.back().name = name;
        schematicList.back().renderSize = nthp::vectf64(0,0);
        schematicList.back().hitboxSize = nthp::vectf64(0,0);
        schematicList.back().hitboxOffset = nthp::vectf64(0,0);
        schematicList.back().framesetIndex = -1;

        return schematicList.size() - 1;
}


void Scene::constructSchematicFrameset(unsigned int schematicID) {
        auto& schematic = schematicList[schematicID];
        auto& frameset = frameSetList[schematic.framesetIndex];
        if(frameset.textureID < 0) { return; }

        schematic.constructedFrameset.clear();

        for(size_t i = 0; i < frameset.frameSet.size(); ++i) {
                schematic.constructedFrameset.push_back(nthp::texture::Frame(textureList[frameset.textureID].texture.getTextureData().getTexture(), frameset.frameSet[i]));
        }

}


void Scene::deleteSchematic(unsigned int ID) {
        if(ID >= schematicList.size()) { return; }

        schematicList.erase(schematicList.begin()+ID);

        // Schematics have no nessesary order for script purposes.
}




void Scene::addEntity(unsigned int schematicID, nthp::worldPosition position) {
        entityList.push_back(entityObject());
        entityList.back().entity.init();

        auto& schema = schematicList[schematicID];
        
        entityList.back().targetSchema = schematicID;
        entityList.back().identifier.constName = std::string("new") + schema.name + std::to_string(entityList.size() - 1);
        entityList.back().identifier.value = std::to_string(entityList.size() - 1);
        entityList.back().layer = 0;

        entityList.back().entity.importFrameData(schema.constructedFrameset.data(), schema.constructedFrameset.size(), false);
        entityList.back().entity.setRenderSize(nthp::vectFixed(nthp::doubleToFixed(schema.renderSize.x), nthp::doubleToFixed(schema.renderSize.y)));
        entityList.back().entity.setHtiboxSize(nthp::vectFixed(nthp::doubleToFixed(schema.hitboxSize.x), nthp::doubleToFixed(schema.hitboxSize.y)));
        entityList.back().entity.setHitboxOffset(nthp::vectFixed(nthp::doubleToFixed(schema.hitboxOffset.x), nthp::doubleToFixed(schema.hitboxOffset.y)));
        entityList.back().entity.setCurrentFrame(0);
        entityList.back().entity.setPosition(position);
}


void Scene::updateEntitySchematicData(unsigned int entityID) {
        entityObject& entityTarget = entityList[entityID];
        objectTypeSchematic& schema = schematicList[entityTarget.targetSchema];
        

        if(frameSetList[schema.framesetIndex].textureID >= 0 && (schema.constructedFrameset.size()))
                entityTarget.entity.importFrameData(schema.constructedFrameset.data(), schema.constructedFrameset.size(), false);
        else
                entityTarget.entity.importFrameData(NULL, 0, false);


        entityTarget.entity.setRenderSize(nthp::vectFixed(nthp::doubleToFixed(schema.renderSize.x), nthp::doubleToFixed(schema.renderSize.y)));
        entityTarget.entity.setHtiboxSize(nthp::vectFixed(nthp::doubleToFixed(schema.hitboxSize.x), nthp::doubleToFixed(schema.hitboxSize.y)));
        entityTarget.entity.setHitboxOffset(nthp::vectFixed(nthp::doubleToFixed(schema.hitboxOffset.x), nthp::doubleToFixed(schema.hitboxOffset.y)));
        entityTarget.entity.setCurrentFrame(0);
}

void Scene::updateAllEntitySchematicData() {
        
        for(size_t i = 0; i < entityList.size(); ++i) {
                updateEntitySchematicData(i);
        }
}











int Project::newEmptyProject(const char* name, const char* src) {
        projectName = name;
        sourceDirectory = src;
        sceneList.clear();

        sceneList.push_back(Scene("scene0"));
        currentScene = 0;
        allowModification = true;
        
        return 0;
}

int Project::addNewScene() {
        std::string name = "scene" + std::to_string(sceneList.size());

        sceneList.push_back(Scene(name.c_str()));
        currentScene = sceneList.size() - 1;

        return 0;
}



