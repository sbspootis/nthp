#pragma once
#include <stdint.h>

namespace nthp {


        // Definition of fixed-point decimal numbers.
        // It seemed kinda fun to try using them.

        // Fixed Point Scale represents the fixed-exponent of fixed point numbers
        // (fixed_t) as a power of 2 (SCALE_FACTOR = 1 / (2 ^ FIXED_POINT_SCALE)). Considering
        // THP does a lot of int conversions to decimal for positioning to render and vice-versa,
        // the overall speed will benefit, as the decimal precision is not as important, and casting
        // integers to fixed point is really fast.

        // God I love the preprocessor.

#ifndef FIXED_POINT_SCALE
                // The fixed-point scale factor as a whole-number power of 2
#define         FIXED_POINT_SCALE       10

#endif




#ifndef FIXED_POINT_WIDTH


        // Size of a fixed point number in bits; Must be a standard width (8,16,32,64).
#define         FIXED_POINT_WIDTH       32

#endif






// Ugly, but there's no better way to do it. All you have to do now is change the
// two macros above, and the whole fixed-point system should scale accordingly. Assumes
// 32-bits if none, or an invalid width, is defined.
#if FIXED_POINT_WIDTH == 64
                                #define FIXED_TYPE               int64_t
                                #define UPCAST_TYPE              ____NOTYPE
                                namespace fixedTypeConstants { constexpr uint64_t FIXED_MAX = UINT64_MAX; }
 #else
        #if FIXED_POINT_WIDTH == 32
                                #define FIXED_TYPE               int32_t
                                #define UPCAST_TYPE              int64_t
                                namespace fixedTypeConstants { constexpr uint32_t FIXED_MAX = UINT32_MAX; }
        #else   
                #if FIXED_POINT_WIDTH == 16
                                #define FIXED_TYPE               int16_t
                                #define UPCAST_TYPE              int32_t
                                namespace fixedTypeConstants { constexpr uint16_t FIXED_MAX = UINT16_MAX; }
                #else
                        #if FIXED_POINT_WIDTH == 8
                                #define FIXED_TYPE               int8_t
                                #define UPCAST_TYPE              int16_t
                                namespace fixedTypeConstants { constexpr uint8_t FIXED_MAX = UINT8_MAX; }
                        #else
                                #define FIXED_TYPE               int32_t
                                #define UPCAST_TYPE              int64_t
                                namespace fixedTypeConstants { constexpr uint32_t FIXED_MAX = UINT32_MAX; }
                        #endif

                #endif
        #endif
#endif




#if FIXED_POINT_SCALE > FIXED_POINT_WIDTH
        #undef FIXED_POINT_SCALE
        #define FIXED_POINT_SCALE       (FIXED_POINT_WIDTH / 2)
#endif

  

        typedef FIXED_TYPE fixed_t;


        constexpr FIXED_TYPE fixedToInt(fixed_t value) { return (value >> FIXED_POINT_SCALE); }
        constexpr fixed_t intToFixed(FIXED_TYPE value) { return (value << FIXED_POINT_SCALE); }


        constexpr double fixedToDouble(fixed_t value) { return ((double)value / (double)((FIXED_TYPE)1 << FIXED_POINT_SCALE)); }
        constexpr fixed_t doubleToFixed(double value) { return (value * ((FIXED_TYPE)1 << FIXED_POINT_SCALE)); }

        namespace fixedTypeConstants { 
                constexpr decltype(FIXED_MAX) FIXED_DECIMAL_MASK = (FIXED_MAX >> (FIXED_POINT_WIDTH - FIXED_POINT_SCALE));
                constexpr decltype(FIXED_MAX) FIXED_INTEGER_MASK = (~FIXED_DECIMAL_MASK);



                // The Epsilon of a fixed-point number (in this case the whole system) represents the smallest
                // possible unit that can be accurately represented by the number. Incrementing a fixed_t will result in an
                // increase of the fixed representation by EPSILON.
                // If EPSILON = 0.000000533, then
                // fixed_t++ = (fixed_t + 0.000000533).
                constexpr double FIXED_EPSILON = nthp::fixedToDouble(1);

                // Fast, non-iterative approximation of the scale factor's square root.
                // Is 100% accurate with even-exponent scale factors. Required for the f_sqrt() function.
                constexpr FIXED_TYPE sqrt_eval = (1 << (FIXED_POINT_SCALE / 2));
        }


        // Returns the decimal value of the fixed-point number as a fixed-point, discarding the integer (3.14 = 0.14).
        // Extremely fast. 
        constexpr fixed_t getFixedDecimal(fixed_t value) { return (value & nthp::fixedTypeConstants::FIXED_DECIMAL_MASK); }

        // Returns the whole-number value (integer) of the fixed_point number as a fixed_point, discarding the decimal (3.14 = 3). 
        // Extremely fast
        constexpr fixed_t getFixedInteger(fixed_t value) { return (value & nthp::fixedTypeConstants::FIXED_INTEGER_MASK); }


#ifndef FAST_PRODUCT_RANGE
        // Defines the fast fixed-point product operation range. It cannot have a value
        // over (FIXED_POINT_SCALE / 2). The lower the number, the less range the output can have,
        // but the accuracy increases. The accuracy is automatically adjusted by the preprocessor
        // on compile time. An invalid range (over SCALE / 2) will be overwritten for maximum range
        // (and by extension minimal accuracy).
        #define FAST_PRODUCT_RANGE              (1)
#endif


#ifndef FAST_QUOTIEN_RANGE
        // Defines the fast fixed-point quotient operation range. It cannot have a value
        // over (FIXED_POINT_SCALE). The lower the number, the less range the output can have,
        // but the accuracy increases. The accuracy is automatically adjusted by the preprocessor
        // on compile time. An invalid range (over FIXED_POINT_SCALE) will be overwritten for maximum range
        // (and by extension minimal accuracy).
        #define FAST_QUOTIENT_RANGE             (1)

#endif


#if (FAST_PRODUCT_RANGE < 0)
        #undef FAST_PRODUCT_RANGE
        #define FAST_PRODUCT_RANGE              (FIXED_POINT_SCALE / 2)
#endif

#if ((FAST_PRODUCT_RANGE * 2) < FIXED_POINT_SCALE)
        #define FAST_PRODUCT_ACCURACY           ((FIXED_POINT_SCALE) - (FAST_PRODUCT_RANGE * 2))
#else
        // Overwrites invalid FAST_PRODUCT_RANGE with highest possible range.
        #if (FAST_PRODUCT_RANGE * 2) > FIXED_POINT_SCALE
                #undef FAST_PRODUCT_RANGE
                #define FAST_PRODUCT_RANGE      (FIXED_POINT_SCALE / 2)
                #define FAST_PRODUCT_ACCURACY   (0)
        #else
                #define FAST_PRODUCT_ACCURACY   (0)
        #endif

#endif


#if (FAST_QUOTIEN_RANGE < 0)
        #undef FAST_QUOTIENT_RANGE
        #define FAST_QUOTIENT_RANGE             (FIXED_POINT_SCALE / 2)
        #define FAST_QUOTIENT_ACCURACY          (FIXED_POINT_SCALE / 2)
#endif

#if (FAST_QUOTIENT_RANGE > FIXED_POINT_SCALE)
        #undef FAST_QUOTIENT_RANGE
        #define FAST_QUOTIENT_RANGE             (FIXED_POINT_SCALE / 2)
        #define FAST_QUOTIENT_ACCURACY          (FIXED_POINT_SCALE / 2)
#else
        #define FAST_QUOTIENT_ACCURACY          (FIXED_POINT_SCALE - FAST_QUOTIENT_RANGE)
#endif




        // Fast fixed point operations. Accuracy and range can be adjusted by setting FAST_(PRODUCT/QUOTIENT)_ACCURACY.
        // These operations work with any width of fixed-point, but a tradeoff between accuracy and range must be made.
        // Although fast, these operations may not be the most reliable in all situations. If using a width under 64-bits,
        // the "cast" operations become available (see c_fixedProduct() and c_fixedQuotient for more).
        constexpr fixed_t f_fixedProduct(fixed_t a, fixed_t b)          { return ((((a) >> FAST_PRODUCT_RANGE) * ((b) >> FAST_PRODUCT_RANGE)) >> FAST_PRODUCT_ACCURACY); }

        constexpr fixed_t unsafe_fixedProduct(fixed_t a, fixed_t b)     { return ((a * b) >> FIXED_POINT_SCALE); }

        // Fast fixed point operations. Accuracy and range can be adjusted by setting FAST_(PRODUCT/QUOTIENT)_ACCURACY.
        // These operations work with any width of fixed-point, but a tradeoff between accuracy and range must be made.
        // Although fast, these operations may not be the most reliable in all situations. If using a width under 64-bits,
        // the "cast" operations become available (see c_fixedProduct() and c_fixedQuotient for more).
        constexpr fixed_t f_fixedQuotient(fixed_t a, fixed_t b)         { return ((((a) << FAST_QUOTIENT_ACCURACY) / (b)) << FAST_QUOTIENT_RANGE); }


        constexpr fixed_t unsafe_fixedQuotient(fixed_t a, fixed_t b)    { return (((a) << FIXED_POINT_SCALE) / b); }


        // Iterative square root algorithm for fixed point numbers. A little slower than floating point square roots.
        // Requires a conversion int->double to perform the square root. Use sparingly.
        static inline const fixed_t f_sqrt(fixed_t a) { return (((fixed_t)sqrtf(a)) * nthp::fixedTypeConstants::sqrt_eval); }



#if FIXED_POINT_WIDTH < 64

        // If speed is not a concern, nor endlessly tweaking the range and scale, we humbly offer upcast product and quotient
        // operations. (Provided the fixed point width is less than 64bit). Upcasting provides no loss in the possible range or accuracy,
        // but is painfully slow to perform, negating much of what little speed benefit fixed-point provides. Use the fast operations
        // whenever possible, as they work with any fixed point width and are faster.
        constexpr UPCAST_TYPE c_fixedProduct(fixed_t a, fixed_t b) { return ((((UPCAST_TYPE)a) * ((UPCAST_TYPE)b)) >> FIXED_POINT_SCALE); }




        // If speed is not a concern, nor endlessly tweaking the range and scale, we humbly offer upcast product and quotient
        // operations. (Provided the fixed point width is less than 64bit). Upcasting provides no loss in the possible range or accuracy,
        // but is painfully slow to perform, negating much of what little speed benefit fixed-point provides. Use the fast operations
        // whenever possible, as they work with any fixed point width and are faster.
        constexpr UPCAST_TYPE c_fixedQuotient(fixed_t a, fixed_t b) { return (((UPCAST_TYPE)a) << FIXED_POINT_SCALE) / (b); }


#endif




}     
