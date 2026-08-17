#pragma once
#include "global.hpp"



namespace nthp {

        struct ray {
                ray() { x1=0;y1=0;x2=0;y2=0;}
                ray(nthp::fixed_t _x1, nthp::fixed_t _y1, nthp::fixed_t _x2, nthp::fixed_t _y2) { x1=_x1;y1=_y1;x2=_x2;y2=_y2; }
                nthp::fixed_t x1;
                nthp::fixed_t y1;
                nthp::fixed_t x2;
                nthp::fixed_t y2;
        };
        struct rayCollision {
                bool isColliding;
                vectFixed position;
        };


        inline nthp::rayCollision rayColliding(const nthp::ray a, const nthp::ray b) {
                nthp::rayCollision col;

		nthp::fixed_t uA = nthp::f_fixedQuotient((nthp::f_fixedProduct((b.x2 - b.x1) + 1, (a.y1 - b.y1) + 1) - nthp::f_fixedProduct((b.y2 - b.y1) + 1, (a.x1 - b.x1) + 1)), (nthp::f_fixedProduct((b.y2 - b.y1) + 1, (a.x2 - a.x1) + 1) - nthp::f_fixedProduct((b.x2 - b.x1) + 1, (a.y2 - a.y1) + 1)));
		nthp::fixed_t uB = nthp::f_fixedQuotient((nthp::f_fixedProduct((a.x2 - a.x1) + 1, (a.y1 - b.y1) + 1) - nthp::f_fixedProduct((a.y2 - a.y1) + 1, (a.x1 - b.x1) + 1)), (nthp::f_fixedProduct((b.y2 - b.y1) + 1, (a.x2 - a.x1) + 1) - nthp::f_fixedProduct((b.x2 - b.x1) + 1, (a.y2 - a.y1) + 1)));
		
                if (uA >= nthp::intToFixed(0) && uA <= nthp::intToFixed(1) && uB >= nthp::intToFixed(0) && uB <= nthp::intToFixed(1)) {
                        col.isColliding = true;
                        col.position.x = a.x1 + nthp::f_fixedProduct(uA, (a.x2 - a.x1));
                        col.position.y = a.y1 + nthp::f_fixedProduct(uA, (a.y2 - a.y1));
                        return col;
		}

                col.isColliding = false;
                return col;
        }


}