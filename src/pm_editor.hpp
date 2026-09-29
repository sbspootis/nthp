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
                                void* frameSet;                 // Pointer to either the frameset target or textureObject.
                                bool usingTextureFrame;         // Denotes use of texture or frameset. (false=frameset, true=texture)

                                nthp::vectf64 renderSize;
                                nthp::vectf64 hitboxSize;
                                nthp::vectf64 hitboxOffset;
                                nthp::vectf64 position;
                        };


                        class Scene {
                        public:
                                Scene(std::string newName) { name = newName; }
                                int importNewTexture(std::string filename, std::string nameInternal);
                                void deleteTexture(unsigned int target);

                                void regenAllTextures();
                                unsigned int createSchematic(std::string name, void* frameTarget, bool usingTextureFrame);

                                void addNewFrameset();
                                void deleteFrameset(unsigned int index);

                                int addEntity(unsigned int schematicID);
                                int deleteEntity(size_t index);




                                std::string name;

                                std::vector<entityObject> entityList;
                                std::vector<textureObject> textureList;
                                std::vector<frameSetObject> frameSetList;
                                std::vector<objectTypeSchematic> schematicList;
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