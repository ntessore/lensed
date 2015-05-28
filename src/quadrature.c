#include "opencl.h"
#include "quadrature.h"

/*
 * Classic (G3,K7) Gauß-Kronrod integration rule.
 * See Kronrod (1965) for the original extension of a Gauß integration rule,
 * and Patterson (1968) for the general case.
 */

// number of points in quadrature rule
#define QUAD_N 7

// abscissae of rule for [-0.5, 0.5] integration domain
static const double QUAD_ABSC[QUAD_N] = {
    -0.48024563435401014171175354631453998133489111818265, \
    -0.38729833462074168851792653997823996108329217052916, \
    -0.21712187467340127900103575142231390864144927847751,
     0.00000000000000000000000000000000000000000000000000, \
     0.21712187467340127900103575142231390864144927847751, \
     0.38729833462074168851792653997823996108329217052916, \
     0.48024563435401014171175354631453998133489111818265
};

// weights of rule
static const double QUAD_WEIG[QUAD_N] = {
    0.052328113013233632596911928596036519121101080312552, \
    0.13424404493416672036428464033335481238052280000859, \
    0.20069870738798111145252590930921593936371150114327, \
    0.2254582693292370711725550435227854582693292370712, \
    0.20069870738798111145252590930921593936371150114327, \
    0.13424404493416672036428464033335481238052280000859, \
    0.052328113013233632596911928596036519121101080312552
};

// error estimation weights of rule
static const double QUAD_ERRW[QUAD_N] = {
     0.05232811301323363259691192859603651912110108031255, \
    -0.14353373284361105741349313744442296539725497776919, \
     0.20069870738798111145252590930921593936371150114327, \
    -0.21898617511520737327188940092165898617511520737327, \
     0.20069870738798111145252590930921593936371150114327, \
    -0.14353373284361105741349313744442296539725497776919, \
     0.05232811301323363259691192859603651912110108031255
};

size_t quad_points()
{
    return QUAD_N*QUAD_N;
}

void quad_rule(cl_float2 xx[], cl_float2 ww[], double sx, double sy)
{
    for(size_t i = 0, j = 0; j < QUAD_N; ++j)
    {
        for(size_t k = 0; k < QUAD_N; ++k, ++i)
        {
            xx[i].s[0] = sx*QUAD_ABSC[k];
            xx[i].s[1] = sy*QUAD_ABSC[j];
            ww[i].s[0] = QUAD_WEIG[j]*QUAD_WEIG[k];
            ww[i].s[1] = QUAD_ERRW[j]*QUAD_ERRW[k];
        }
    }
}