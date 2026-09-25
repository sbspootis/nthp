#pragma once
#include "pm.hpp"

namespace nthp {
        namespace pm {

                #ifdef WINDOWS

                namespace editor {
                        
                        extern int editorRuntime();
                                                
                        struct entityObject {
                                nthp::entity::gEntity entity;
                                nthp::script::CompilerInstance::CONST_DEF identifier;
                                int layer;
                        };

                        struct textureObject {
                                nthp::texture::gTexture texture;
                                nthp::texture::Frame singleTextureFrame;
                                nthp::script::CompilerInstance::CONST_DEF identifier;
                        };

                        struct frameSetObject {
                                std::vector<SDL_Rect> frameSet;
                                unsigned int textureID;
                                nthp::script::CompilerInstance::CONST_DEF identifier;
                        };

                        struct objectTypeSchematic {
                                std::string name;
                                unsigned int frameSet;

                                nthp::vectFixed renderSize;
                                nthp::vectFixed hitboxSize;
                                nthp::vectFixed hitboxOffset;
                                nthp::vectFixed position;
                        };


                        class Scene {
                        public:
                                Scene(std::string newName) { name = newName; }
                                typedef enum {
                                        TYPE_ENTITY,
                                        TYPE_TEXTURE,
                                        TYPE_FRAMESET,
                                        TYPE_SCHEMATIC
                                } typeID;
                                
                                



                                std::string name;

                                std::vector<entityObject> entityList;
                                std::vector<textureObject> textureList;
                                std::vector<frameSetObject> frameSetList;
                                std::vector<objectTypeSchematic> schematicList;

                                typeID selectedType;
                        };


                        class Project {
                        public:
                                
                                
                                int newEmptyProject();

                                int importNewTexture(std::string filename, std::string nameInternal);
                                int createSchematic(std::string name, size_t frameSet);
                                
                                int addEntity(unsigned int schematicID);
                                int deleteEntity(size_t index);

                                std::string projectName;

                                std::vector<Scene> sceneList;
                                size_t currentScene;

                                
                        };

                }

                #endif
        }
}