
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
        textureList.back().filePath = filename;

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
        if(schematic.framesetIndex < 0 || schematic.framesetIndex >= frameSetList.size()) { return; }

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

        if(schema.framesetIndex >= 0 && schema.framesetIndex < frameSetList.size())
                entityList.back().entity.importFrameData(schema.constructedFrameset.data(), schema.constructedFrameset.size(), false);
        else
                entityList.back().entity.importFrameData(nullptr, 0, false);

        entityList.back().entity.setRenderSize(nthp::vectFixed(nthp::doubleToFixed(schema.renderSize.x), nthp::doubleToFixed(schema.renderSize.y)));
        entityList.back().entity.setHtiboxSize(nthp::vectFixed(nthp::doubleToFixed(schema.hitboxSize.x), nthp::doubleToFixed(schema.hitboxSize.y)));
        entityList.back().entity.setHitboxOffset(nthp::vectFixed(nthp::doubleToFixed(schema.hitboxOffset.x), nthp::doubleToFixed(schema.hitboxOffset.y)));
        entityList.back().entity.setCurrentFrame(0);
        entityList.back().entity.setPosition(position);
}

void Scene::deleteEntity(unsigned int ID) {
        if(ID < entityList.size()) {
                entityList.erase(entityList.begin()+ID);

                for(size_t i = 0; i < entityList.size(); ++i) {
                        entityList[i].identifier.value = std::to_string(i);
                }
        }
}


void Scene::updateEntitySchematicData(unsigned int entityID) {
        entityObject& entityTarget = entityList[entityID];
        objectTypeSchematic& schema = schematicList[entityTarget.targetSchema];
        
        if(schema.framesetIndex >= 0 && schema.framesetIndex < frameSetList.size()) {
                if(frameSetList[schema.framesetIndex].textureID < 0 || (frameSetList[schema.framesetIndex].textureID >= textureList.size()))
                        entityTarget.entity.importFrameData(NULL, 0, false);
                else
                        entityTarget.entity.importFrameData(schema.constructedFrameset.data(), schema.constructedFrameset.size(), false);
        }
        else {
                entityTarget.entity.importFrameData(NULL, 0, false);
        }


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




// Not gonna overcomplicate with binary or super structured output. Everything here is developer side
// anyway, so plain text is nice in case you want to edit something without having to open the entire project.
void Project::writeProjectFile(const char* filename) {
        std::fstream file(filename, std::ios::out);

        if(file.fail()) {
                PRINT_DEBUG_ERROR("Failed to save project file.\n");
                return;
        }

        // Project pararmeters / defaults.

        file << projectName << std::endl;
        file << sourceDirectory << std::endl;
        file << defaultVirtualRes.x << std::endl;
        file << defaultVirtualRes.y << std::endl;

        // Scene-specific Values.
        for(size_t scene_i = 0; scene_i < sceneList.size(); ++scene_i) {
                // Output (in plain text) the data structures in hierarchical order.
                // textures->framesets->schematics->entities.
                // The project import function will read everything in order.
                file << sceneList[scene_i].name << std::endl;
                file << sceneList[scene_i].xVirtualRes << std::endl;
                file << sceneList[scene_i].yVirtualRes << std::endl;

                file << "EDITOR_TEXTURES" << std::endl;
                for(size_t texture_i = 0; texture_i < sceneList[scene_i].textureList.size(); ++texture_i) {
                        const auto& t_list = sceneList[scene_i].textureList[texture_i];
                        file << t_list.identifier.constName << std::endl;
                        file << t_list.filePath << std::endl;
                }

                file << "EDITOR_FRAMESETS" << std::endl;
                for(size_t frameset_i = 0; frameset_i < sceneList[scene_i].frameSetList.size(); ++frameset_i) {
                        const auto& f_list = sceneList[scene_i].frameSetList[frameset_i];

                        file << f_list.identifier.constName << std::endl;
                        file << f_list.textureID << std::endl;
                        file << f_list.frameSet.size() << std::endl;

                        for(size_t i = 0; i < f_list.frameSet.size(); ++i) {
                                file << f_list.frameSet[i].x << ' ' << f_list.frameSet[i].y << ' ' << f_list.frameSet[i].w << ' ' << f_list.frameSet[i].h << std::endl;
                        }
                }

                file << "EDITOR_SCHEMATICS" << std::endl;
                for(size_t schematic_i = 0; schematic_i < sceneList[scene_i].schematicList.size(); ++schematic_i) {
                        const auto& s_list = sceneList[scene_i].schematicList[schematic_i];

                        file << s_list.name << std::endl;
                        file << s_list.framesetIndex << std::endl;

                        file << s_list.renderSize.x << ' ' << s_list.renderSize.y << std::endl;
                        file << s_list.hitboxSize.x << ' ' << s_list.hitboxSize.y << std::endl;
                        file << s_list.hitboxOffset.x << ' ' << s_list.hitboxOffset.y << std::endl;
                        
                }

                file << "EDITOR_ENTITIES" << std::endl;
                for(size_t entity_i = 0; entity_i < sceneList[scene_i].entityList.size(); ++entity_i) {
                        auto& e_list = sceneList[scene_i].entityList[entity_i];

                        file << e_list.identifier.constName << std::endl;
                        file << e_list.targetSchema << std::endl;
                        file << e_list.layer << std::endl;
                        file << nthp::fixedToDouble(e_list.entity.getPosition().x) << ' ' << nthp::fixedToDouble(e_list.entity.getPosition().y) << std::endl;
                }
        }

}