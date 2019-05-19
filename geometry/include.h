#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <deque>
#include <cmath>
#include <list>
#include <ctime>
#include <complex>
#include <memory>
#include <tuple>
#include <functional>
#include <map>
#include <set>
#include <algorithm>
#include <limits>
#include <climits>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <new>
#include <xmmintrin.h>
#include <pmmintrin.h>

//#include <boost/container_hash/hash.hpp>

//#include <shared/math.h>
#include <shared/graphics.h>
//#include <shared/win.h>
#include <shared/generic_macros.h>

//#include "cuda.h"

//using namespace Eigen;
using namespace std;

//this function is here to speed up compilation time
/*void precompiled_functions(int n, vector<Triplet<float>> v) {
    {
        SparseMatrix<float> A(n, n);
        SparseMatrix<float> b(n, n);
        SparseMatrix<float> x(n, n);
        A.setFromTriplets(v.begin(), v.end());
        b.setFromTriplets(v.begin(), v.end());
        SparseLU<SparseMatrix<float>> solver;
        solver.analyzePattern(A);
        solver.factorize(A);
        x=solver.solve(b);
    }
    {
        MatrixXf A(n, n);
        MatrixXf b(n, n);
        MatrixXf x(n, n);
        PartialPivLU<MatrixXf> solver(A);
        x=solver.solve(b);
    }
} */
