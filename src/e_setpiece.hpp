#pragma once
#include "s_script.hpp"





namespace nthp {
        namespace entity {

                // A self-contained structure for fixed renderables. Render's a texture to the screen
                // (like an entity) but uses a static renderpacket instead of generating it when the QRENDER
                // is issued.
                struct staticSetpiece {
                        void init() {
                                renderSize = nthp::vectFixed(0,0);
                                position = nthp::vectFixed(0,0);
                                currentFrame = 0;
                                frames = nullptr;
                                compiledPacket = INVALID_RENDERPACKET;
                                frameSize = 0;
                        }

                        nthp::vectFixed renderSize;
                        nthp::worldPosition position;

                        size_t currentFrame;
                        nthp::texture::Frame* frames;
                        size_t frameSize;
                        nthp::RenderPacket compiledPacket;
                };

                inline int compileSetpiece(staticSetpiece* setpiece, nthp::RenderRuleSet* context) {
                        const auto pxlPos = nthp::generatePixelPosition(setpiece->position, context);
                        if(setpiece->frames == NULL) {
                                PRINT_DEBUG_ERROR("No frameset assigned to setpiece [%p]; unable to compile.\n", setpiece);
                                setpiece->compiledPacket = INVALID_RENDERPACKET;
                                return 1;
                        }

                        setpiece->compiledPacket.texture = setpiece->frames[setpiece->currentFrame].texture;
                        setpiece->compiledPacket.dstRect = {
                                (int)pxlPos.x, 
                                (int)pxlPos.y, 
                                (int)nthp::fixedToInt(nthp::f_fixedProduct(setpiece->renderSize.x, context->scaleFactor.x)),
                                (int)nthp::fixedToInt(nthp::f_fixedProduct(setpiece->renderSize.y, context->scaleFactor.y))
                        };
                        setpiece->compiledPacket.srcRect = &(setpiece->frames[setpiece->currentFrame].src);
                        setpiece->compiledPacket.state = nthp::RenderPacket::C_OPERATE::VALID;


                        return 0;
                }

                inline int compileSetpiece_abs(staticSetpiece* setpiece, nthp::RenderRuleSet* context) {
                        const auto pxlPos = nthp::generatePixelPosition(setpiece->position, context);

                        if(setpiece->frames == NULL) {
                                PRINT_DEBUG_ERROR("No frameset assigned to setpiece [%p]; unable to compile.\n", setpiece);
                                setpiece->compiledPacket = INVALID_RENDERPACKET;
                                return 1;
                        }

                        setpiece->compiledPacket.texture = setpiece->frames[setpiece->currentFrame].texture;
                        setpiece->compiledPacket.dstRect = {
                                (int)pxlPos.x, 
                                (int)pxlPos.y, 
                                (int)nthp::fixedToInt(setpiece->renderSize.x),
                                (int)nthp::fixedToInt(setpiece->renderSize.y)
                        };
                        setpiece->compiledPacket.srcRect = &(setpiece->frames[setpiece->currentFrame].src);
                        setpiece->compiledPacket.state = nthp::RenderPacket::C_OPERATE::ABSOLUTE;


                        return 0;
                }

        }
}