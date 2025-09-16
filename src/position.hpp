#pragma once
#include "global.hpp"

namespace nthp {

        typedef nthp::vectFixed worldPosition;
        
        extern nthp::vectGeneric generatePixelPosition(nthp::worldPosition pos, nthp::RenderRuleSet* ruleset);
        extern nthp::vectFixed generateWorldPosition(vectGeneric pos, nthp::RenderRuleSet* ruleset);
}
