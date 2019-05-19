//eigen shits up the compiler outside of optimized builds
//all this stuff is compatible with eigen's shit

#define EIGEN_VECTORIZE_SSE4_2

#include <complex>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <memory>
#include <utility>
#include <tuple>
#include <array>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/LU>
#include <Eigen/Geometry>
#include <Eigen/SVD>
#include <Eigen/SparseCore>
#include <Eigen/SparseLU>

using namespace std;
using namespace Eigen;

#define todo
#include "linear_algebra_header.h"

using namespace std;
using namespace Eigen;

template<int num, class type> Matrix<type, num, num> eigen_read_matrix(const type* d) {
    Matrix<type, num, num> res;
    for (int y=0;y<num;++y) {
        for (int x=0;x<num;++x) {
            res(y, x)=d[y*num+x];
        }
    }
    return res;
}

template<int num, class type, class matrix> void eigen_assign_matrix(type* out, const matrix& d) {
    for (int y=0;y<num;++y) {
        for (int x=0;x<num;++x) {
            out[y*num+x]=d(y, x);
        }
    }
}

template<class type> Quaternion<type> eigen_read_quaternion(const type* d) {
    return Quaternion<type>(d[0], d[1], d[2], d[3]);
}

template<class type, class quaternion> void eigen_assign_quaternion(type* out, const quaternion& d) {
    out[0]=d.w();
    out[1]=d.x();
    out[2]=d.y();
    out[3]=d.z();
}

template<class type1, class type2, unsigned long size> array<type2, size> solve_least_squares_impl(
    const vector<array<type1, size>>& A,
    const vector<type2>& b
) {
    assert(A.size()==b.size());

    //Matrix<type2, Dynamic, Dynamic> A_matrix(b.size(), size); todo //slow
    Matrix<type1, Dynamic, size> A_matrix(b.size(), size);
    Matrix<type2, Dynamic, 1> b_matrix(b.size());

    for (int i=0;i<b.size();++i) {
        for (int j=0;j<size;++j) {
            A_matrix(i, j)=A[i][j];
        }
        b_matrix[i]=b[i];
    }

    Matrix<type2, size, 1> x_matrix=
        (A_matrix.transpose() * A_matrix).ldlt().solve(A_matrix.transpose() * b_matrix)
    ;

    /*todo //slow
    Matrix<type2, size, 1> x_matrix=
        A_matrix.bdcSvd(ComputeThinU | ComputeThinV).solve(b_matrix)
    ;*/

    /*Matrix<type2, Dynamic, 1> error=A_matrix*x_matrix-b_matrix;
    cerr << "Error: ";
    for (int i=0;i<size;++i) {
        cerr << error[i] << ", ";
    }
    cerr << "\n";*/

    array<type2, size> res;
    for (int i=0;i<size;++i) {
        res[i]=x_matrix[i];
    }
    return res;
}

template<class type> void solve_equations_impl(
    const vector<tuple<int, int, type>>& A,
    const vector<type>& b,
    vector<type>& x
) {
    const type error_scale=1e-5f;

    vector<Triplet<type>> triplets;
    for (tuple<int, int, type> c : A) {
        triplets.emplace_back(get<0>(c), get<1>(c), get<2>(c));
    }

    SparseMatrix<type> A_matrix(b.size(), b.size());
    Matrix<type, Dynamic, 1> b_matrix(b.size());
    Matrix<type, Dynamic, 1> x_matrix(b.size());

    for (int i=0;i<b.size();++i) {
        b_matrix[i]=b[i];
    }

    A_matrix.setFromTriplets(triplets.begin(), triplets.end());

    SparseLU<SparseMatrix<type>> solver;

    auto check_error=[&](int n) {
        if (solver.info()!=0) {
            cerr << "solve_equations_impl error: " <<  n << ", " << solver.info() << ", " << solver.lastErrorMessage() << "\n";
        }
    };

    solver.analyzePattern(A_matrix);
    solver.factorize(A_matrix); check_error(0);
    x_matrix=solver.solve(b_matrix);

    x.clear();
    for (int i=0;i<b.size();++i) {
        x.push_back(x_matrix[i]);
    }

    //
    //

    auto rand_11=[&]() -> type {
        return type(std::rand())*type(2.0/RAND_MAX)-type(1);
    };

    Matrix<type, Dynamic, 1> x_matrix_new=x_matrix;
    type error_amount=error_scale*x_matrix.norm()/sqrt(b.size());
    for (int i=0;i<b.size();++i) {
        x_matrix_new[i]+=rand_11()*error_amount;
    }
    type x_error=(x_matrix_new-x_matrix).norm()/x_matrix.norm();

    Matrix<type, Dynamic, 1> b_matrix_new=A_matrix*x_matrix_new;
    type b_error=(b_matrix_new-b_matrix).norm()/b_matrix.norm();

    cerr << "solve_equations " <<
        "x_error: " << x_error << ", b_error: " << b_error << " " <<
        "x/b: " << x_error/b_error << " " <<
    "\n";
}

namespace linear_algebra {
    void solve_equations(
        const vector<tuple<int, int, complex<double>>>& A,
        const vector<complex<double>>& b,
        vector<complex<double>>& x
    ) {
        solve_equations_impl(A, b, x);
    }

    array<complex<double>, 3> solve_least_squares(
        const vector<array<double, 3>>& A,
        const vector<complex<double>>& b
    ) {
        return solve_least_squares_impl(A, b);
    }

    array<complex<double>, 10> solve_least_squares(
        const vector<array<double, 10>>& A,
        const vector<complex<double>>& b
    ) {
        return solve_least_squares_impl(A, b);
    }

    void eigen_to_rotation_matrix(float* out, const float* q) {
        eigen_assign_matrix<3>(out, eigen_read_quaternion(q).toRotationMatrix());
    }

    void eigen_to_rotation_matrix(double* out, const double* q) {
        eigen_assign_matrix<3>(out, eigen_read_quaternion(q).toRotationMatrix());
    }

    void eigen_from_rotation_matrix(float* out, const float* m) {
        eigen_assign_quaternion(out, Quaternion<float>(eigen_read_matrix<3>(m)));
    }

    void eigen_from_rotation_matrix(double* out, const double* m) {
        eigen_assign_quaternion(out, Quaternion<double>(eigen_read_matrix<3>(m)));
    }

    void eigen_inverse_2(float* out, const float* m) {
        eigen_assign_matrix<2>(out, eigen_read_matrix<2>(m).inverse());
    }

    void eigen_inverse_2(double* out, const double* m) {
        eigen_assign_matrix<2>(out, eigen_read_matrix<2>(m).inverse());
    }

    void eigen_inverse_3(float* out, const float* m) {
        eigen_assign_matrix<3>(out, eigen_read_matrix<3>(m).inverse());
    }

    void eigen_inverse_3(double* out, const double* m) {
        eigen_assign_matrix<3>(out, eigen_read_matrix<3>(m).inverse());
    }

    void eigen_inverse_2(complex<float>* out, const complex<float>* m) {
        eigen_assign_matrix<2>(out, eigen_read_matrix<2>(m).inverse());
    }

    void eigen_inverse_2(complex<double>* out, const complex<double>* m) {
        eigen_assign_matrix<2>(out, eigen_read_matrix<2>(m).inverse());
    }

    void eigen_inverse_3(complex<float>* out, const complex<float>* m) {
        eigen_assign_matrix<3>(out, eigen_read_matrix<3>(m).inverse());
    }

    void eigen_inverse_3(complex<double>* out, const complex<double>* m) {
        eigen_assign_matrix<3>(out, eigen_read_matrix<3>(m).inverse());
    }
}
