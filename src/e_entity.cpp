#include "e_entity.hpp"


nthp::entity::gEntity::gEntity() {
        init();
}

inline void nthp::entity::gEntity::init() {
        frameData = nullptr;
        frameSize = 0;
        frameDataNative = false;

        currentFrame = 0;
        renderSize = nthp::vectFixed(0,0);
        wPosition = nthp::worldPosition(0,0);
        hbOffset = nthp::vectFixed(0,0);
}


const nthp::RenderPacket nthp::entity::gEntity::getUpdateRenderPacket(nthp::RenderRuleSet* context) {
        auto pxlPos = nthp::generatePixelPosition(wPosition, context);
        nthp::RenderPacket::C_OPERATE state = nthp::RenderPacket::C_OPERATE::VALID;

        if(frameData[currentFrame].texture == NULL)
                state = nthp::RenderPacket::C_OPERATE::INVALID;


        return nthp::generateRenderPacket(frameData[currentFrame].texture, &frameData[currentFrame].src, 
                {
                        (int)pxlPos.x, 
                        (int)pxlPos.y, 
                        (int)nthp::fixedToInt(nthp::f_fixedProduct(renderSize.x, context->scaleFactor.x)),
                        (int)nthp::fixedToInt(nthp::f_fixedProduct(renderSize.y, context->scaleFactor.y)),
                }, state);
}


// Gets the entity RenderPacket
const nthp::RenderPacket nthp::entity::gEntity::abs_getRenderPacket(nthp::RenderRuleSet* context) {
        const auto pxlPos = nthp::generatePixelPosition(wPosition, context);
        nthp::RenderPacket::C_OPERATE state = nthp::RenderPacket::C_OPERATE::ABSOLUTE;

        if(frameData[currentFrame].texture == NULL)
                state = nthp::RenderPacket::C_OPERATE::INVALID;

         return nthp::generateRenderPacket(frameData[currentFrame].texture, &frameData[currentFrame].src, 
                {
                        (int)pxlPos.x, 
                        (int)pxlPos.y,
                        (int)nthp::fixedToInt(renderSize.x),
                        (int)nthp::fixedToInt(renderSize.y)
                }, state);
}



void nthp::entity::gEntity::setRenderSize(nthp::vectFixed size) {
        renderSize = size;
}


void nthp::entity::gEntity::importFrameData(nthp::texture::Frame* data, size_t size, bool native) {
        frameData = data;
        frameSize = size;

        frameDataNative = native;
}


void nthp::entity::gEntity::setPosition(nthp::vectFixed newPos) {
        wPosition = newPos;
	hitbox.x = wPosition.x + hbOffset.x;
	hitbox.y = wPosition.y + hbOffset.y;
}

// Moves the entity by given offset (NO DELTATIME FACTOR)
void nthp::entity::gEntity::move(nthp::vectFixed offset) {
        wPosition += offset;
	hitbox.x = wPosition.x + hbOffset.x;
	hitbox.y = wPosition.y + hbOffset.y;
}

void nthp::entity::gEntity::setCurrentFrame(size_t cf) {
        if(cf > frameSize) cf -= frameSize;
        currentFrame = cf;
}


void nthp::entity::gEntity::setHtiboxSize(nthp::vectFixed newSize) {
        hitbox.x = wPosition.x + hbOffset.x;
	hitbox.y = wPosition.y + hbOffset.y;
	hitbox.w = newSize.x;
	hitbox.h = newSize.y;
}


void nthp::entity::gEntity::setHitboxOffset(nthp::vectFixed offset) {
	hbOffset = offset;
}



inline void nthp::entity::gEntity::clean() {
        if(frameDataNative) {
                free(frameData);
        }
}


nthp::entity::gEntity::~gEntity() {
        clean();
}
