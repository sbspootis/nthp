#include "st_renderText.hpp"

nthp::texture::text::RenderText::RenderText() {
        init();
}

void nthp::texture::text::RenderText::init() {
        renderCursor.angle = 0;
        renderCursor.dstRect = {0,0,0,0};
        renderCursor.srcRect = &src;
        renderCursor.texture = nullptr;
        renderCursor.state = nthp::RenderPacket::C_OPERATE::INVALID;
}


void nthp::texture::text::RenderText::setFont(nthp::texture::text::Font* newFont) {
        font = newFont;
        renderCursor.texture = font->fontTextureData.getTextureData().getTexture();
}

void nthp::texture::text::RenderText::setStringTarget(char* newTarget) {
        stringTarget = newTarget;
}


void nthp::texture::text::RenderText::setPosition(nthp::worldPosition newPosition) {
        position = newPosition;
}

void nthp::texture::text::RenderText::setCharacterRenderSize(nthp::vectFixed size) {
        renderSize = size;
}


int nthp::texture::text::RenderText::renderText(nthp::EngineCore* coreTarget) {
        // currentRenderChar = [(iterator * renderSize.x) + position.x + kerning, position.y]
        auto _pos = position;
        renderCursor.state = nthp::RenderPacket::C_OPERATE::VALID;

        vectGeneric pxlPos;

        for(int i = 0; stringTarget[i] != '\0'; ++i) {

                src = font->frameList[font->map.map[stringTarget[i] & 127]];
                _pos = nthp::vectFixed(nthp::f_fixedProduct(nthp::intToFixed(i), renderSize.x) + position.x + (nthp::f_fixedProduct(nthp::intToFixed(i),kerning)), position.y);


                pxlPos = nthp::generatePixelPosition(_pos, &coreTarget->p_coreDisplay);

                renderCursor.dstRect = {
                        pxlPos.x, 
                        pxlPos.y, 
                        (int)nthp::fixedToInt(nthp::f_fixedProduct(renderSize.x, coreTarget->p_coreDisplay.scaleFactor.x)),
                        (int)nthp::fixedToInt(nthp::f_fixedProduct(renderSize.y, coreTarget->p_coreDisplay.scaleFactor.y))
                };

                nthp::core.render(renderCursor);
        }
        

        return 0;
}

int nthp::texture::text::RenderText::abs_renderText(nthp::EngineCore* coreTarget) {
        // currentRenderChar = [(iterator * renderSize.x) + position.x + kerning, position.y]
        auto _pos = position;
        renderCursor.state = nthp::RenderPacket::C_OPERATE::ABSOLUTE;

        vectGeneric pxlPos;

        for(int i = 0; stringTarget[i] != '\0'; ++i) {

                src = font->frameList[font->map.map[stringTarget[i]]];
                _pos = nthp::vectFixed(nthp::f_fixedProduct(nthp::intToFixed(i), renderSize.x) + position.x + (nthp::f_fixedProduct(nthp::intToFixed(i),kerning)), position.y);


                pxlPos = nthp::generatePixelPosition(_pos, &coreTarget->p_coreDisplay);

                renderCursor.dstRect = {
                        pxlPos.x, 
                        pxlPos.y, 
                        (int)nthp::fixedToInt(nthp::f_fixedProduct(renderSize.x, coreTarget->p_coreDisplay.scaleFactor.x)),
                        (int)nthp::fixedToInt(nthp::f_fixedProduct(renderSize.y, coreTarget->p_coreDisplay.scaleFactor.y))
                };

                nthp::core.render(renderCursor);
        }
        

        return 0;
}
