#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <cassert>
#include <iomanip>
#include <set>
#include <random>
#include <limits>
#include <cstdlib>
#include <map>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <deque>

#include <shared/generic.h>
#include "half.hpp_"

#include <elfio/elf_types.hpp>

#include <cuda.h>

using namespace std;
using namespace generic;
using namespace half_float;
using namespace ELFIO;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;