#if  __STDC__!=1 || __STDC_VERSION__<199901L
# error "Requires C99"
#endif

// https://stackoverflow.com/questions/6430448/why-doesnt-gcc-optimize-aaaaaa-to-aaaaaa/
//
// $ cc test_aaaaaa.c -lmpfr -lm
// ./a.out

#include <float.h>
#include <math.h>
#include <stdio.h>

#include <mpfr.h>

// Pretend that there may be other floating point formats
#if FLT_RADIX != 2 || DBL_MANT_DIG<FLT_MANT_DIG || DBL_MANT_EXP<FLT_MANT_EXP
# error "Unknown floating point format"
#endif

#define A_N 6
#define FUNCTIONS 6

// Carefully calculate the correctly rounded relative error into a double
static double relative_error(mpfr_srcptr op1,float op2,mpfr_rnd_t rnd)
{
    // Subtract into something that is guaranteed to fit
    MPFR_DECL_INIT(t,FLT_MANT_DIG*A_N);
    mpfr_sub_d(t,op1,op2,MPFR_RNDZ);
    mpfr_copysign(t,t,op1,MPFR_RNDZ);

    // The signs are equal so the result will be positive
    MPFR_DECL_INIT(d,DBL_MANT_DIG);
    mpfr_div(d,t,op1,rnd);
    return mpfr_get_d(d,MPFR_RNDZ);
}

int main(void)
{
    MPFR_DECL_INIT(exact,FLT_MANT_DIG*A_N);
    MPFR_DECL_INIT(   am,FLT_MANT_DIG);

    double rel[FUNCTIONS][2] = {0};

    const float start = 1.0F;
    const float stop = start * FLT_RADIX;

    for( float a=start; a<stop; a=nextafterf(a,FLT_MAX) ) {

        // a^n exactly
        mpfr_set_flt(am,a,MPFR_RNDZ);
        mpfr_pow_ui(exact,am,A_N,MPFR_RNDZ);

        // a^n exactly to nearest float
        mpfr_set(am,exact,MPFR_RNDN);

        const float b[FUNCTIONS] = {
            mpfr_get_flt(exact,MPFR_RNDN),
            powf(a,(float)A_N),
#if (__GNUC__>9||__clang__) && __has_builtin(__builtin_powif)
            __builtin_powif(a,A_N),
#else
            0,
#endif
            a*a*a*a*a*a,
            (a*a)*(a*a)*(a*a),
            (a*a*a)*(a*a*a)
        };

        // Round errors away from zero since we want worst case
        for( int i=0; i<FUNCTIONS; ++i ) {
            rel[i][0] = fmax(rel[i][0],relative_error(   am,b[i],MPFR_RNDA));
            rel[i][1] = fmax(rel[i][1],relative_error(exact,b[i],MPFR_RNDA));
        }
    }

    printf( "# %-*s %-*s\n", DBL_DIG+6, "Nearest", DBL_DIG+6, "Exact" );
    for( int i=0; i<FUNCTIONS; ++i )
        printf( "%d %.*E %.*E\n", i, DBL_DIG, rel[i][0], DBL_DIG, rel[i][1] );
}
