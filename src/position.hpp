#pragma once
#include "global.hpp"

namespace nthp {

        typedef nthp::vectFixed worldPosition;
        
        extern inline nthp::vectGeneric generatePixelPosition(nthp::worldPosition pos, nthp::RenderRuleSet* ruleset);
        extern inline nthp::vectFixed generateWorldPosition(vectGeneric pos, nthp::RenderRuleSet* ruleset);
}
