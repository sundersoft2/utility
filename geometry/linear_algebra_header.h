namespace linear_algebra {
    void solve_equations(
        const vector<tuple<int, int, complex<double>>>& A,
        const vector<complex<double>>& b,
        vector<complex<double>>& x
    );

    array<complex<double>, 3> solve_least_squares(
        const vector<array<double, 3>>& A,
        const vector<complex<double>>& b
    );

    array<complex<double>, 10> solve_least_squares(
        const vector<array<double, 10>>& A,
        const vector<complex<double>>& b
    );

    //matricies are row major
    void eigen_to_rotation_matrix(float* out, const float* q);
    void eigen_to_rotation_matrix(double* out, const double* q);
    void eigen_from_rotation_matrix(float* out, const float* m);
    void eigen_from_rotation_matrix(double* out, const double* m);
    void eigen_inverse_2(float* out, const float* m);
    void eigen_inverse_2(double* out, const double* m);
    void eigen_inverse_2(complex<float>* out, const complex<float>* m);
    void eigen_inverse_2(complex<double>* out, const complex<double>* m);
    void eigen_inverse_3(complex<float>* out, const complex<float>* m);
    void eigen_inverse_3(complex<double>* out, const complex<double>* m);
}