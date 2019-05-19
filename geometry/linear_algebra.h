#define USE_EIGEN 0

#include "linear_algebra_header.h"

//always real. can be floats or doubles
typedef double scalar;
//typedef float scalar;

template<class type> void sort_and_unique(type& t) {
    sort(t.begin(), t.end());
    decltype(t.begin()) new_end=unique(t.begin(), t.end());
    t.resize(new_end-t.begin());
}

template<class type, class func_lt, class func_eq> void sort_and_unique(type& t, func_lt lt, func_eq eq) {
    sort(t.begin(), t.end(), lt);
    decltype(t.begin()) new_end=unique(t.begin(), t.end(), eq);
    t.resize(new_end-t.begin());
}

namespace linear_algebra {
    template<class type, int num> struct vec {
        type _data[num];

        vec() {
            for (int x=0;x<num;++x) {
                _data[x]=NAN;
            }
        }

        vec(type t_x, type t_y) {
            static_assert(num==2, "");
            _data[0]=t_x;
            _data[1]=t_y;
        }

        vec(type t_x, type t_y, type t_z) {
            static_assert(num==3, "");
            _data[0]=t_x;
            _data[1]=t_y;
            _data[2]=t_z;
        }

        vec(type t_x, type t_y, type t_z, type t_w) {
            static_assert(num==4, "");
            _data[0]=t_x;
            _data[1]=t_y;
            _data[2]=t_z;
            _data[3]=t_w;
        }

        type& x() { static_assert(num>=1, ""); return _data[0]; }
        type& y() { static_assert(num>=2, ""); return _data[1]; }
        type& z() { static_assert(num>=3, ""); return _data[2]; }
        type& w() { static_assert(num>=4, ""); return _data[3]; }

        const type& x() const { static_assert(num>=1, ""); return _data[0]; }
        const type& y() const { static_assert(num>=2, ""); return _data[1]; }
        const type& z() const { static_assert(num>=3, ""); return _data[2]; }
        const type& w() const { static_assert(num>=4, ""); return _data[3]; }

        type& operator[](int pos) {
            assert(pos>=0 && pos<num);
            return _data[pos];
        }

        const type& operator[](int pos) const {
            assert(pos>=0 && pos<num);
            return _data[pos];
        }

        type dot(vec t) const {
            type res=0;
            for (int x=0;x<num;++x) {
                res+=_data[x]*t._data[x];
            }
            return res;
        }

        vec cross(vec t) const {
            static_assert(num==3);
            vec res;
            res.x()=y()*t.z() - z()*t.y();
            res.y()=z()*t.x() - x()*t.z();
            res.z()=x()*t.y() - y()*t.x();
            return res;
        }

        vec operator*(type v) const {
            vec res=*this;
            for (int x=0;x<num;++x) {
                res[x]*=v;
            }
            return res;
        }

        vec operator/(type v) const {
            return (*this)*(type(1)/v);
        }

        vec operator+(vec v) const {
            vec res;
            for (int x=0;x<num;++x) {
                res[x]=(*this)[x]+v[x];
            }
            return res;
        }

        vec operator-() const {
            vec res;
            for (int x=0;x<num;++x) {
                res[x]=-(*this)[x];
            }
            return res;
        }

        vec operator-(vec v) const {
            return (*this)+(-v);
        }

        type squaredNorm() const {
            type res=0;
            for (int x=0;x<num;++x) {
                res+=std::norm(_data[x]); //squared norm of complex number
            }
            return res;
        }

        type norm() const {
            return sqrt(squaredNorm());
        }

        vec normalized() const {
            vec res=(*this)/norm();
            for (int x=0;x<num;++x) {
                if (!isfinite(res[x])) {
                    return *this;
                }
            }
            return res;
        }

        static vec Zero() {
            vec res;
            for (int x=0;x<num;++x) {
                res._data[x]=0;
            }
            return res;
        }
    };

    template<class type, int num> vec<type, num> operator*(type v, vec<type, num> t) {
        return t*v;
    }

    template<class type, int num> struct matrix {
        type _data[num*num];

        matrix() {
            for (int x=0;x<num*num;++x) {
                _data[x]=NAN;
            }
        }

        type& operator()(int y, int x) {
            assert(x>=0 && x<num && y>=0 && y<num);
            return _data[y*num+x];
        }

        const type& operator()(int y, int x) const {
            assert(x>=0 && x<num && y>=0 && y<num);
            return _data[y*num+x];
        }

        void assign_col(int x, vec<type, num> v) {
            assert(x>=0 && x<num);
            for (int y=0;y<num;++y) {
                (*this)(y, x)=v[y];
            }
        }

        vec<type, num> col(int x) const {
            assert(x>=0 && x<num);
            vec<type, num> res;
            for (int y=0;y<num;++y) {
                res[y]=(*this)(y, x);
            }
            return res;
        }

        vec<type, num> row(int y) const {
            assert(y>=0 && y<num);
            vec<type, num> res;
            for (int x=0;x<num;++x) {
                res[x]=(*this)(y, x);
            }
            return res;
        }

        matrix<type, num> transpose() const {
            matrix<type, num> res;
            for (int y=0;y<num;++y) {
                for (int x=0;x<num;++x) {
                    res(x, y)=(*this)(y, x);
                }
            }
            return res;
        }

        void transposeInPlace() {
            *this=transpose();
        }

        vec<type, num> operator*(vec<type, num> v) const {
            vec<type, num> res;
            for (int y=0;y<num;++y) {
                res[y]=row(y).dot(v);
            }
            return res;
        }

        matrix operator*(matrix v) const {
            matrix res;
            for (int x=0;x<num;++x) {
                res.assign_col(x, (*this)*v.col(x));
            }
            return res;
        }

        matrix operator*(type v) const {
            matrix res;
            for (int x=0;x<num*num;++x) {
                res._data[x]=_data[x]*v;
            }
            return res;
        }

        matrix operator+(matrix v) const {
            matrix res;
            for (int x=0;x<num*num;++x) {
                res._data[x]=_data[x]+v._data[x];
            }
            return res;
        }

        static matrix Zero() {
            matrix res;
            for (int x=0;x<num*num;++x) {
                res._data[x]=0;
            }
            return res;
        }

        static matrix Identity() {
            matrix res=Zero();
            for (int x=0;x<num;++x) {
                res(x, x)=1;
            }
            return res;
        }

        matrix inverse() const {
            matrix res;
            if (num==2) {
                eigen_inverse_2(res._data, _data);
            } else
            if (num==3) {
                eigen_inverse_3(res._data, _data);
            } else {
                assert(false);
            }
            return res;
        }
    };

    template<class type, int num> matrix<type, num> operator*(type v, matrix<type, num> t) {
        return t*v;
    }

    template<class type> struct quaternion {
        type _data[4];

        quaternion() {
            for (int x=0;x<4;++x) {
                _data[x]=NAN;
            }
        }

        quaternion(matrix<type, 3> m) {
            eigen_from_rotation_matrix(_data, m._data);
        }

        quaternion(type t_w, type t_x, type t_y, type t_z) {
            _data[0]=t_w;
            _data[1]=t_x;
            _data[2]=t_y;
            _data[3]=t_z;
        }

        type& w() { return _data[0]; }
        type& x() { return _data[1]; }
        type& y() { return _data[2]; }
        type& z() { return _data[3]; }

        const type& w() const { return _data[0]; }
        const type& x() const { return _data[1]; }
        const type& y() const { return _data[2]; }
        const type& z() const { return _data[3]; }

        matrix<type, 3> toRotationMatrix() const {
            matrix<type, 3> res;
            eigen_to_rotation_matrix(res._data, _data);
            return res;
        }

        static quaternion Identity() {
            return quaternion(1, 0, 0, 0);
        }
    };
}

/*template<class type> struct triplet {
    int d_row;
    int d_col;
    type d_value;

    triplet(int t_row, int t_col, type t_value) : d_row(t_row), d_col(t_col), d_value(t_value) {}

    int row() const { return d_row; }
    int col() const { return d_col; }
    type value() const { return d_value; }
};*/

using linear_algebra::solve_equations;
using linear_algebra::solve_least_squares;

#if USE_EIGEN
    using namespace Eigen;

    //can be real (electrostatics/magnetostatics) or complex (quasistatics/full field)
    typedef complex<scalar> field_scalar;
    typedef Matrix<field_scalar, 2, 1> field_vector2;
    typedef Matrix<field_scalar, 3, 1> field_vector3;
    typedef Matrix<field_scalar, 4, 1> field_vector4;
    typedef Matrix<field_scalar, 2, 2> field_matrix2;
    typedef Matrix<field_scalar, 3, 3> field_matrix3;
    typedef Matrix<field_scalar, 4, 4> field_matrix4;

    typedef Matrix<scalar, 2, 1> vector2;
    typedef Matrix<scalar, 3, 1> vector3;
    typedef Matrix<scalar, 4, 1> vector4;
    typedef Matrix<scalar, 2, 2> matrix2;
    typedef Matrix<scalar, 3, 3> matrix3;
    typedef Matrix<scalar, 4, 4> matrix4;
    typedef Quaternion<scalar> quaternion;
    void assign_matrix(matrix2& m, vector2 x, vector2 y) { m << x, y; }
    void assign_matrix(matrix3& m, vector3 x, vector3 y, vector3 z) { m << x, y, z; }
    void assign_matrix(matrix4& m, vector4 x, vector4 y, vector4 z, vector4 w) { m << x, y, z, w; }
#else
    //can be real (electrostatics/magnetostatics) or complex (quasistatics/full field)
    typedef complex<scalar> field_scalar;
    typedef linear_algebra::vec<field_scalar, 2> field_vector2;
    typedef linear_algebra::vec<field_scalar, 3> field_vector3;
    typedef linear_algebra::vec<field_scalar, 4> field_vector4;
    typedef linear_algebra::matrix<field_scalar, 2> field_matrix2;
    typedef linear_algebra::matrix<field_scalar, 3> field_matrix3;
    typedef linear_algebra::matrix<field_scalar, 4> field_matrix4;

    typedef linear_algebra::vec<scalar, 2> vector2;
    typedef linear_algebra::vec<scalar, 3> vector3;
    typedef linear_algebra::vec<scalar, 4> vector4;
    typedef linear_algebra::matrix<scalar, 2> matrix2;
    typedef linear_algebra::matrix<scalar, 3> matrix3;
    typedef linear_algebra::matrix<scalar, 4> matrix4;
    typedef linear_algebra::quaternion<scalar> quaternion;

    template<class a, class v> void assign_matrix(a& m, v x, v y) {
        m.assign_col(0, x);
        m.assign_col(1, y);
    }

    template<class a, class v> void assign_matrix(a& m, v x, v y, v z) {
        m.assign_col(0, x);
        m.assign_col(1, y);
        m.assign_col(2, z);
    }

    template<class a, class v> void assign_matrix(a& m, v x, v y, v z, v w) {
        m.assign_col(0, x);
        m.assign_col(1, y);
        m.assign_col(2, z);
        m.assign_col(3, w);
    }

    template<int n, class type> ostream& operator<<(ostream& t, linear_algebra::vec<type, n> v) {
        t << "(";
        for (int x=0;x<n;++x) {
            if (x!=0) {
                t << ", ";
            }
            t << v[x];
        }
        t << ")";
        return t;
    }
#endif

int rand_int() {
    return std::rand();
}

scalar rand_01() {
    return scalar(std::rand())*scalar(1.0/RAND_MAX);
}

scalar rand_11() {
    return scalar(std::rand())*scalar(2.0/RAND_MAX)-scalar(1);
}

//axis_z must be normalized
//returns identity matrix if axis_z=<0,0,1>
matrix3 generate_basis(vector3 axis_z) {
    vector3 axis_x;
    {
        int i=1;
        if (abs(axis_z[0])<abs(axis_z[i])) {
            i=0;
        }
        if (abs(axis_z[2])<abs(axis_z[i])) {
            i=2;
        }

        vector3 v=vector3::Zero();
        v[i]=1;
        axis_x=v.cross(axis_z).normalized();
    }
    vector3 axis_y=axis_z.cross(axis_x);

    matrix3 res;
    assign_matrix(res, axis_x, axis_y, axis_z);
    return res;
}

field_vector3 to_field_vector3(vector3 v) {
    return field_vector3(v.x(), v.y(), v.z());
}