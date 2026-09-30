#pragma once
#include "pm.hpp"

namespace nthp {
        namespace pm {

                namespace editor {
                        
                        extern int editorRuntime();
                                                
                        struct entityObject {
                                nthp::entity::gEntity entity;
                                nthp::script::CompilerInstance::CONST_DEF identifier;           // the identifier is the internal name.
                                int layer;
                                unsigned int targetSchema;
                        };

                        struct textureObject {
                                textureObject() { texture.init(); }
                                nthp::texture::gTexture texture;
                                nthp::texture::Frame singleTextureFrame;
                                nthp::script::CompilerInstance::CONST_DEF identifier;           // the identifier is the internal name.
                        };

                        struct frameSetObject {
                                std::vector<SDL_Rect> frameSet;
                                int textureID;
                                nthp::script::CompilerInstance::CONST_DEF identifier;           // the identifier is the internal name.
                                std::string searchTextureName;
                        };

                        struct objectTypeSchematic {
                                std::string name;
                                int framesetIndex;
                                std::string searchFramesetName;
                                std::vector<nthp::texture::Frame> constructedFrameset;

                                nthp::vectf64 renderSize;
                                nthp::vectf64 hitboxSize;
                                nthp::vectf64 hitboxOffset;
                        };


                        class Scene {
                        public:
                                Scene(std::string newName) { name = newName; }
                                int importNewTexture(std::string filename, std::string nameInternal);
                                void deleteTexture(unsigned int target);

                                void regenAllTextures();
                                unsigned int createSchematic(std::string name);
                                void deleteSchematic(unsigned int ID);

                                void addNewFrameset();
                                void deleteFrameset(unsigned int index);

                                void addEntity(unsigned int schematicID, nthp::worldPosition position);
                                void updateEntitySchematicData(unsigned int entityID);
                                void updateAllEntitySchematicData();
                                int deleteEntity(size_t index);

                                void constructSchematicFrameset(unsigned int schematicID);




                                std::string name;

                                std::vector<entityObject> entityList;
                                std::vector<textureObject> textureList;
                                std::vector<frameSetObject> frameSetList;
                                std::vector<objectTypeSchematic> schematicList;
                                nthp::entity::gEntity* selectedEntity;
                        };


                        class Project {
                        public:
                                
                                
                                int newEmptyProject(const char* name, const char* src);
                                int addNewScene();

                                Scene& activeScene() { return sceneList[currentScene]; }
                                

                                
                                
                                

                                std::string projectName;
                                std::string sourceDirectory;
                                bool allowModification = false;


                                std::vector<Scene> sceneList;
                                int currentScene = 0;
                                
                        };

                }

        }
}