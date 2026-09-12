#include "st_renderText.hpp"

nthp::texture::text::RenderText::RenderText() {
        renderCursor.importFrameData(&renderFrameChar, 1, false);
}


void nthp::texture::text::RenderText::setFont(nthp::texture::text::Font* newFont) {
        font = newFont;
        renderFrameChar.texture = font->fontTextureData.getTextureData().getTexture();
}



void nthp::texture::text::RenderText::setPosition(nthp::worldPosition newPosition) {
        position = newPosition;
}

void nthp::texture::text::RenderText::setCharacterRenderSize(nthp::vectFixed size) {
        renderCursor.setRenderSize(size);
}


int nthp::texture::text::RenderText::renderText(nthp::EngineCore* coreTarget) {
        // currentRenderChar = [(iterator * renderSize.x) + position.x + kerning, position.y]

        for(int i = 0; true; ++i) {
                if(stringTarget[i] == '\000') { break; }

                renderFrameChar.src = font->frameList[font->map.map[stringTarget[i]]];
                renderCursor.setPosition(nthp::vectFixed(nthp::f_fixedProduct(nthp::intToFixed(i), renderCursor.getRenderSize().x) + position.x + (nthp::f_fixedProduct(nthp::intToFixed(i),kerning)), position.y));

                coreTarget->render(renderCursor.getUpdateRenderPacket(&coreTarget->p_coreDisplay));
        }
        

        return 0;
}

int nthp::texture::text::RenderText::abs_renderText(nthp::EngineCore* coreTarget) {
        // currentRenderChar = [(iterator * renderSize.x) + position.x + kerning, position.y]

        for(int i = 0; true; ++i) {
                if(stringTarget[i] == '\000') { break; }
                
                renderFrameChar.src = font->frameList[font->map.map[stringTarget[i]]];
                renderCursor.setPosition(nthp::vectFixed(nthp::f_fixedProduct(nthp::intToFixed(i), renderCursor.getRenderSize().x) + position.x + ((!i) * kerning), position.y));

                coreTarget->render(renderCursor.abs_getRenderPacket(&coreTarget->p_coreDisplay));
        }


        return 0;
}