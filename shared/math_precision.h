#ifndef ILYA_HEADER_MATH_PRECISION
#define ILYA_HEADER_MATH_PRECISION

#include "math_linear_vector.h"
#include "math_quaternion.h"
#include "math_linear_transform.h"
#include "math_geometry_objects.h"

namespace math {

namespace math_double {
    using namespace math::vec_double;
    using namespace math::matrix_double;
    using namespace math::quaternion_double;
    using namespace math::geometry_double;
}

namespace math_float {
    using namespace math::vec_float;
    using namespace math::matrix_float;
    using namespace math::quaternion_float;    
    using namespace math::geometry_float;
}

}

#endif
