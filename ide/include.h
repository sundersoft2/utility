#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <algorithm>
#include <cctype>
#include <tuple>
#include <cassert>
#include <boost/uuid/sha1.hpp>
#include <boost/crc.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include <SDL2/SDL.h>

//#ifdef main
//    #undef main
//#endif

//#ifdef assert
//    #undef assert
//#endif

#include "shared/generic.h"
#include "shared/io.h"
#include "shared/graphics.h"
#include "shared/math.h"
#include "shared/fixed_size.h"

#include "lz4.h"
