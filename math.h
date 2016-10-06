#ifndef _BEN_MATH_H

#define _BEN_MATH_H 1

#include <math.h>

#include "options.h"

#ifndef M_PHI
#define M_PHI ( (sqrt(5.0) + 1.0) / 2.0)
#endif /* M_PHI */

#ifndef M_PLASTIC
#define M_PLASTIC ( cbrt( 0.5 + ( sqrt(23.0/3.0) / 6.0 ) ) + \
                    cbrt( 0.5 - ( sqrt(23.0/3.0) / 6.0 ) ) )
#endif

#endif /* _BEN_MATH_H */

