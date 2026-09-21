#pragma once
#include "global.hpp"

namespace nthp {
        namespace entity {
                struct cRect {
                        cRect() { x = 0; y = 0; w = 0; h = 0; }
                        cRect(nthp::fixed_t nx, nthp::fixed_t ny, nthp::fixed_t nw, nthp::fixed_t nh) { x = nx; y = ny; w = nw; h = nh; }

                        nthp::fixed_t x;
                        nthp::fixed_t y;
                        nthp::fixed_t w;
                        nthp::fixed_t h;
                };

                extern int checkRectCollision(cRect a,cRect b);
        }
}
