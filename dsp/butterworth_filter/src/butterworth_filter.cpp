#include "butterworth_filter.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

static std::vector<double> pad_to_len(const std::vector<double>& v, int n) {
    if ((int)v.size() >= n) return v;
    std::vector<double> out = v;
    out.resize(n, 0.0);
    return out;
}

// Gaussian elimination with partial pivoting, row-major A (n*n)
static std::vector<double> solve_linear(const std::vector<double>& A,
                                        const std::vector<double>& b,
                                        int n) {
    if ((int)A.size() != n * n) throw std::invalid_argument("solve_linear: A size mismatch");
    if ((int)b.size() != n)     throw std::invalid_argument("solve_linear: b size mismatch");

    std::vector<double> M(n * (n + 1), 0.0); // augmented [A|b]
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) M[r * (n + 1) + c] = A[r * n + c];
        M[r * (n + 1) + n] = b[r];
    }

    for (int col = 0; col < n; ++col) {
        int pivot = col;
        double best = std::fabs(M[col * (n + 1) + col]);
        for (int r = col + 1; r < n; ++r) {
            double v = std::fabs(M[r * (n + 1) + col]);
            if (v > best) { best = v; pivot = r; }
        }
        if (best == 0.0) throw std::runtime_error("solve_linear: singular matrix");

        if (pivot != col) {
            for (int c = col; c <= n; ++c)
                std::swap(M[col * (n + 1) + c], M[pivot * (n + 1) + c]);
        }

        const double inv_piv = 1.0 / M[col * (n + 1) + col];
        for (int c = col; c <= n; ++c) M[col * (n + 1) + c] *= inv_piv;

        for (int r = col + 1; r < n; ++r) {
            const double f = M[r * (n + 1) + col];
            if (f == 0.0) continue;
            for (int c = col; c <= n; ++c)
                M[r * (n + 1) + c] -= f * M[col * (n + 1) + c];
        }
    }

    std::vector<double> x(n, 0.0);
    for (int r = n - 1; r >= 0; --r) {
        double s = M[r * (n + 1) + n];
        for (int c = r + 1; c < n; ++c)
            s -= M[r * (n + 1) + c] * x[c];
        x[r] = s;
    }
    return x;
}

} // namespace

// ---------------- ctor ----------------

ButterworthFilter::ButterworthFilter(const std::vector<double>& b,
                                     const std::vector<double>& a,
                                     bool precompute)
: precompute_(precompute) {
    k_.b = b;
    k_.a = a;
    normalize_ba(k_.b, k_.a);
    k_.ntaps = (int)std::max(k_.b.size(), k_.a.size());

    if (precompute_) {
        k_.zi = lfilter_zi(k_.b, k_.a);
    }
}

// ---------------- public APIs (vector) ----------------

std::vector<double> ButterworthFilter::filter(const std::vector<double>& x,
                                              PadType padtype,
                                              int padlen) const {
    return filtfilt(x, padtype, padlen);
}

std::vector<double> ButterworthFilter::filtfilt(const std::vector<double>& x,
                                                PadType padtype,
                                                int padlen) const {
    return filtfilt(k_, x.data(), x.size(), padtype, padlen);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter(const std::vector<double>& x,
                           const std::vector<double>* zi) const {
    return lfilter(k_, x.data(), x.size(), zi);
}

std::vector<double> ButterworthFilter::detrend(const std::vector<double>& x) const {
    return detrend(x.data(), x.size());
}

// ---------------- public APIs (ptr) ----------------

std::vector<double> ButterworthFilter::filter(const double* x, size_t n,
                                              PadType padtype, int padlen) const {
    return filtfilt(x, n, padtype, padlen);
}

std::vector<double> ButterworthFilter::filtfilt(const double* x, size_t n,
                                                PadType padtype, int padlen) const {
    return filtfilt(k_, x, n, padtype, padlen);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter(const double* x, size_t n,
                           const std::vector<double>* zi) const {
    return lfilter(k_, x, n, zi);
}

std::vector<double> ButterworthFilter::detrend(const double* x, size_t n) const {
    if (n <= 1) return std::vector<double>(x, x + n);

    // y = x - (m*t + c), t = 0..n-1
    const double N = (double)n;
    const double sum_t  = (N - 1.0) * N * 0.5;
    const double sum_tt = (N - 1.0) * N * (2.0 * N - 1.0) / 6.0;

    double sum_y = 0.0, sum_ty = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum_y  += x[i];
        sum_ty += (double)i * x[i];
    }

    const double denom = N * sum_tt - sum_t * sum_t;
    double m = 0.0;
    if (denom != 0.0) m = (N * sum_ty - sum_t * sum_y) / denom;
    const double c = (sum_y / N) - m * (sum_t / N);

    std::vector<double> y(n);
    for (size_t i = 0; i < n; ++i) y[i] = x[i] - (m * (double)i + c);
    return y;
}

std::vector<double> ButterworthFilter::lfilterZi() const {
    return k_.zi;
}

// ---------------- static precompute ----------------

ButterworthFilter::Kernel
ButterworthFilter::precompute(const std::vector<double>& b,
                              const std::vector<double>& a) {
    Kernel k;
    k.b = b;
    k.a = a;
    normalize_ba(k.b, k.a);
    k.ntaps = (int)std::max(k.b.size(), k.a.size());
    k.zi = lfilter_zi(k.b, k.a);
    return k;
}

// ---------------- kernel wrappers ----------------

std::vector<double>
ButterworthFilter::filtfilt(const Kernel& k,
                            const std::vector<double>& x,
                            PadType padtype, int padlen) {
    return filtfilt(k, x.data(), x.size(), padtype, padlen);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter(const Kernel& k,
                           const std::vector<double>& x,
                           const std::vector<double>* zi) {
    return lfilter(k, x.data(), x.size(), zi);
}

// ---------------- core helpers ----------------

void ButterworthFilter::normalize_ba(std::vector<double>& b, std::vector<double>& a) {
    if (a.empty()) throw std::invalid_argument("a must not be empty");
    const double a0 = a[0];
    if (a0 == 0.0) throw std::invalid_argument("a[0] must be nonzero");
    if (a0 == 1.0) return;

    const double inv = 1.0 / a0;
    for (double& v : b) v *= inv;
    for (double& v : a) v *= inv;
}

int ButterworthFilter::compute_edge(int x_len, int ntaps, PadType padtype, int padlen) {
    // SciPy default: padlen(None) => edge = 3 * ntaps
    int edge = (padlen < 0) ? (3 * ntaps) : padlen;
    if (edge < 0) throw std::invalid_argument("padlen must be >= 0");

    if (padtype == PadType::None || edge == 0) return 0;

    // SciPy requires len(x) > edge
    if (x_len <= edge) throw std::invalid_argument("x.size() must be > padlen/edge");
    return edge;
}

std::vector<double>
ButterworthFilter::pad_extend_1d(const std::vector<double>& x, int edge, PadType padtype) {
    return pad_extend_1d_ptr(x.data(), x.size(), edge, padtype);
}

std::vector<double>
ButterworthFilter::pad_extend_1d_ptr(const double* x, size_t n, int edge, PadType padtype) {
    if (edge <= 0 || padtype == PadType::None) {
        return std::vector<double>(x, x + n);
    }
    if ((int)n <= edge) throw std::invalid_argument("pad_extend: n must be > edge");

    std::vector<double> ext;
    ext.reserve(n + 2ull * (size_t)edge);

    const double x0 = x[0];
    const double xN = x[n - 1];

    if (padtype == PadType::Odd) {
        for (int i = edge; i >= 1; --i) ext.push_back(2.0 * x0 - x[(size_t)i]);
        ext.insert(ext.end(), x, x + n);
        for (int i = 1; i <= edge; ++i) ext.push_back(2.0 * xN - x[n - 1ull - (size_t)i]);
        return ext;
    }
    if (padtype == PadType::Even) {
        for (int i = edge; i >= 1; --i) ext.push_back(x[(size_t)i]);
        ext.insert(ext.end(), x, x + n);
        for (int i = 1; i <= edge; ++i) ext.push_back(x[n - 1ull - (size_t)i]);
        return ext;
    }
    // Constant
    for (int i = 0; i < edge; ++i) ext.push_back(x0);
    ext.insert(ext.end(), x, x + n);
    for (int i = 0; i < edge; ++i) ext.push_back(xN);
    return ext;
}

std::vector<double>
ButterworthFilter::lfilter_zi(std::vector<double> b, std::vector<double> a) {
    normalize_ba(b, a);

    const int n = (int)std::max(b.size(), a.size()) - 1;
    if (n <= 0) return {};

    a = pad_to_len(a, n + 1);
    b = pad_to_len(b, n + 1);

    // (I - A) where A = companion(a).T
    // IA[i,i]=1, IA[i,0]+=a[i+1], IA[i,i+1]-=1
    std::vector<double> IA(n * n, 0.0);
    for (int i = 0; i < n; ++i) {
        IA[i * n + i] = 1.0;
        IA[i * n + 0] += a[i + 1];
        if (i < n - 1) IA[i * n + (i + 1)] -= 1.0;
    }

    const double b0 = b[0];
    std::vector<double> B(n, 0.0);
    for (int i = 0; i < n; ++i) B[i] = b[i + 1] - a[i + 1] * b0;

    return solve_linear(IA, B, n);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter_df2t(std::vector<double> b, std::vector<double> a,
                                const double* x, size_t n,
                                const std::vector<double>* zi) {
    normalize_ba(b, a);

    const int order = (int)std::max(b.size(), a.size()) - 1;
    if (order <= 0) {
        const double g = b.empty() ? 0.0 : b[0];
        std::vector<double> y(n);
        for (size_t k = 0; k < n; ++k) y[k] = g * x[k];
        return {y, {}};
    }

    a = pad_to_len(a, order + 1);
    b = pad_to_len(b, order + 1);

    std::vector<double> z(order, 0.0);
    if (zi) {
        if ((int)zi->size() != order) throw std::invalid_argument("lfilter: zi size mismatch");
        z = *zi;
    }

    std::vector<double> y(n, 0.0);
    for (size_t k = 0; k < n; ++k) {
        const double xi = x[k];
        const double yi = b[0] * xi + z[0];
        y[k] = yi;

        for (int i = 0; i < order - 1; ++i)
            z[i] = z[i + 1] + b[i + 1] * xi - a[i + 1] * yi;
        z[order - 1] = b[order] * xi - a[order] * yi;
    }
    return {y, z};
}

// ---------------- kernel ptr core ----------------

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter(const Kernel& k, const double* x, size_t n,
                           const std::vector<double>* zi) {
    return lfilter_df2t(k.b, k.a, x, n, zi);
}

std::vector<double>
ButterworthFilter::filtfilt(const Kernel& k, const double* x, size_t n,
                            PadType padtype, int padlen) {
    if (k.b.empty() || k.a.empty()) throw std::invalid_argument("filtfilt: b/a empty");
    if (n == 0) return {};

    const int edge = compute_edge((int)n, k.ntaps, padtype, padlen);
    const std::vector<double> ext = (edge > 0)
        ? pad_extend_1d_ptr(x, n, edge, padtype)
        : std::vector<double>(x, x + n);

    // zi base: use cached if present, else compute
    std::vector<double> zi_base = k.zi;
    if (zi_base.empty()) zi_base = lfilter_zi(k.b, k.a);

    // forward with zi*x0
    std::vector<double> zi_f;
    const std::vector<double>* pzi_f = nullptr;
    if (!zi_base.empty()) {
        zi_f.resize(zi_base.size());
        const double x0 = ext.front();
        for (size_t i = 0; i < zi_base.size(); ++i) zi_f[i] = zi_base[i] * x0;
        pzi_f = &zi_f;
    }

    auto y_pair = lfilter_df2t(k.b, k.a, ext.data(), ext.size(), pzi_f);
    std::vector<double> y = std::move(y_pair.first);

    // backward with zi*y0
    const double y0 = y.back();
    std::reverse(y.begin(), y.end());

    std::vector<double> zi_b;
    const std::vector<double>* pzi_b = nullptr;
    if (!zi_base.empty()) {
        zi_b.resize(zi_base.size());
        for (size_t i = 0; i < zi_base.size(); ++i) zi_b[i] = zi_base[i] * y0;
        pzi_b = &zi_b;
    }

    auto y2_pair = lfilter_df2t(k.b, k.a, y.data(), y.size(), pzi_b);
    std::vector<double> y2 = std::move(y2_pair.first);
    std::reverse(y2.begin(), y2.end());

    if (edge == 0) return y2;
    return std::vector<double>(y2.begin() + edge, y2.end() - edge);
}
