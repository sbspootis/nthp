#pragma once

#include "softwaretexture.hpp"
#include "position.hpp"
#include "e_collision.hpp"
#include "core.hpp"

namespace nthp {
        namespace entity {

        class gEntity {
        public:
                gEntity();
                void init();

                virtual const nthp::RenderPacket getUpdateRenderPacket(nthp::RenderRuleSet* context) final;
                virtual const nthp::RenderPacket abs_getRenderPacket(nthp::RenderRuleSet* context) final;

                void importFrameData(nthp::texture::Frame* data, size_t size, bool native);
                void setRenderSize(nthp::vectFixed size);

                void setPosition(nthp::vectFixed newPos);
                void move(nthp::vectFixed offset);

                inline nthp::vectFixed getRenderSize() { return renderSize; }
                inline nthp::worldPosition getPosition() { return wPosition; }

                void setCurrentFrame(size_t cf);
                inline size_t getCurrentFrameIndex() { return currentFrame; }
                inline nthp::texture::Frame getCurrentFrameTexture() { return frameData[currentFrame]; }

		inline nthp::entity::cRect getHitbox() { return hitbox; }
		void setHtiboxSize(nthp::vectFixed newSize);

		void setHitboxOffset(nthp::vectFixed offset);
		inline nthp::vectFixed getHitboxOffset() { return hbOffset; }

                void setRenderAngle(nthp::fixed_t newAngle);
                nthp::fixed_t getRenderAngle() { return nthp::doubleToFixed(angle); }

                void clean();
                ~gEntity();
        protected:
                
		nthp::texture::Frame* frameData;
                size_t frameSize;
                bool frameDataNative;
                
                size_t currentFrame;
                nthp::vectFixed renderSize;

                nthp::worldPosition wPosition;
                nthp::entity::cRect hitbox;
		nthp::vectFixed hbOffset;

                float angle;
        };

        }

}
