#include "butterworth_filter.h"
#include <algorithm>
#include <cmath>
#include <complex>
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

// ---------------- factory methods ----------------

ButterworthFilter ButterworthFilter::from_ba(const std::vector<double>& b,
                                             const std::vector<double>& a,
                                             bool cache_zi) {
    ButterworthFilter filter;
    filter.mode_ = Mode::BA;
    filter.precompute_ = cache_zi;
    
    filter.ba_kernel_.b = b;
    filter.ba_kernel_.a = a;
    normalize_ba(filter.ba_kernel_.b, filter.ba_kernel_.a);
    filter.ba_kernel_.ntaps = (int)std::max(filter.ba_kernel_.b.size(), filter.ba_kernel_.a.size());
    
    if (cache_zi) {
        filter.ba_kernel_.zi = lfilter_zi(filter.ba_kernel_.b, filter.ba_kernel_.a);
    }
    
    return filter;
}

ButterworthFilter ButterworthFilter::from_sos(const std::vector<SOSSection>& sos,
                                              bool cache_zi) {
    ButterworthFilter filter;
    filter.mode_ = Mode::SOS;
    filter.precompute_ = cache_zi;
    
    filter.sos_kernel_.sos = sos;
    normalize_sos(filter.sos_kernel_.sos);
    filter.sos_kernel_.n_sections = (int)filter.sos_kernel_.sos.size();
    filter.sos_kernel_.ntaps = sos_ntaps(filter.sos_kernel_.sos);
    
    if (cache_zi) {
        filter.sos_kernel_.zi = sosfilt_zi(filter.sos_kernel_.sos);
    }
    
    return filter;
}

ButterworthFilter ButterworthFilter::from_params(int order,
                                                 double fs,
                                                 const std::string& btype,
                                                 const std::vector<double>& cutoff,
                                                 bool cache_zi) {
    auto [b, a] = butter_ba(order, fs, btype, cutoff);
    return from_ba(b, a, cache_zi);
}

// ---------------- public APIs ----------------

std::vector<double> ButterworthFilter::filtfilt(const std::vector<double>& x,
                                                PadType padtype,
                                                int padlen) const {
    return filtfilt(x.data(), x.size(), padtype, padlen);
}

std::vector<double> ButterworthFilter::filtfilt(const double* x, size_t n,
                                                PadType padtype, int padlen) const {
    if (mode_ == Mode::SOS) return filtfilt_impl(sos_kernel_, x, n, padtype, padlen);
    return filtfilt_impl(ba_kernel_, x, n, padtype, padlen);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter(const std::vector<double>& x,
                           const std::vector<double>* zi) const {
    return lfilter(x.data(), x.size(), zi);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter(const double* x, size_t n,
                           const std::vector<double>* zi) const {
    if (mode_ == Mode::SOS) return lfilter_impl(sos_kernel_, x, n, zi);
    return lfilter_impl(ba_kernel_, x, n, zi);
}

std::vector<double> ButterworthFilter::detrend(const std::vector<double>& x) const {
    return detrend(x.data(), x.size());
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

// ---------------- SOS helpers ----------------

void ButterworthFilter::normalize_sos(std::vector<SOSSection>& sos) {
    for (auto& s : sos) {
        const double a0 = s[3];
        if (a0 == 0.0) throw std::invalid_argument("normalize_sos: a0 must be nonzero");
        if (a0 == 1.0) {
            // still ensure canonical a0==1 exactly
            s[3] = 1.0;
            continue;
        }
        const double inv = 1.0 / a0;
        s[0] *= inv; s[1] *= inv; s[2] *= inv;
        s[4] *= inv; s[5] *= inv;
        s[3] = 1.0;
    }
}

int ButterworthFilter::sos_ntaps(const std::vector<SOSSection>& sos) {
    // Match SciPy sosfiltfilt's heuristic ntaps computation.
    // For an SOS cascade, the equivalent TF order is 2*n_sections.
    // SciPy computes:
    //   ntaps = 2*n_sections + 1 - min(n_zeros_at_end(b), n_zeros_at_end(a))
    // where b/a are per-section and trailing zeros are counted across sections.
    const int nsec = (int)sos.size();
    if (nsec == 0) return 0;

    int tz_b = 0;
    int tz_a = 0;
    for (int i = 0; i < nsec; ++i) {
        const auto& s = sos[i];
        if (s[2] == 0.0) ++tz_b;
        if (s[5] == 0.0) ++tz_a;
    }
    const int tz = std::min(tz_b, tz_a);
    return 2 * nsec + 1 - tz;
}

std::vector<double> ButterworthFilter::sosfilt_zi(const std::vector<SOSSection>& sos) {
    const int nsec = (int)sos.size();
    std::vector<double> zi((size_t)2 * (size_t)nsec, 0.0);
    if (nsec == 0) return zi;

    // Compute per-section DF2T zi for step response, scaled by cumulative DC gain.
    double cum_gain = 1.0;
    for (int i = 0; i < nsec; ++i) {
        const auto& s = sos[i];
        const double b0 = s[0], b1 = s[1], b2 = s[2];
        const double a1 = s[4], a2 = s[5];

        const double sum_b = b0 + b1 + b2;
        const double sum_a = 1.0 + a1 + a2;
        const double g = (sum_a != 0.0) ? (sum_b / sum_a) : 0.0;

        // DF2T steady-state states for unit-step input to *this section*.
        const double y_ss = g;
        const double z1 = (y_ss - b0);
        const double z2 = (b2 - a2 * y_ss);

        zi[(size_t)2 * (size_t)i + 0] = z1 * cum_gain;
        zi[(size_t)2 * (size_t)i + 1] = z2 * cum_gain;

        cum_gain *= g;
    }
    return zi;
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
ButterworthFilter::lfilter_zi(const std::vector<double>& b_in,
                              const std::vector<double>& a_in) {
    std::vector<double> b = b_in;
    std::vector<double> a = a_in;
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

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::sosfilt_df2t(const std::vector<SOSSection>& sos,
                                const double* x, size_t n,
                                const std::vector<double>* zi) {
    const int nsec = (int)sos.size();
    if (nsec == 0) {
        return {std::vector<double>(x, x + n), {}};
    }

    std::vector<double> z((size_t)2 * (size_t)nsec, 0.0);
    if (zi) {
        if (zi->size() != z.size()) throw std::invalid_argument("sosfilt: zi size mismatch");
        z = *zi;
    }

    std::vector<double> y(n, 0.0);
    for (size_t k = 0; k < n; ++k) {
        double xi = x[k];
        for (int si = 0; si < nsec; ++si) {
            const auto& s = sos[si];
            const double b0 = s[0], b1 = s[1], b2 = s[2];
            // a0 is normalized to 1
            const double a1 = s[4], a2 = s[5];

            const size_t o = (size_t)2 * (size_t)si;
            const double z1 = z[o + 0];
            const double z2 = z[o + 1];

            const double yi = b0 * xi + z1;
            const double new_z1 = b1 * xi - a1 * yi + z2;
            const double new_z2 = b2 * xi - a2 * yi;
            z[o + 0] = new_z1;
            z[o + 1] = new_z2;
            xi = yi;
        }
        y[k] = xi;
    }
    return {y, z};
}

// ---------------- 核心滤波实现 ----------------

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter_impl(const BAKernel& k, const double* x, size_t n,
                               const std::vector<double>* zi) {
    return lfilter_df2t(k.b, k.a, x, n, zi);
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::lfilter_impl(const SOSKernel& k, const double* x, size_t n,
                               const std::vector<double>* zi) {
    return sosfilt_df2t(k.sos, x, n, zi);
}

std::vector<double>
ButterworthFilter::filtfilt_impl(const BAKernel& k, const double* x, size_t n,
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

std::vector<double>
ButterworthFilter::filtfilt_impl(const SOSKernel& k, const double* x, size_t n,
                                PadType padtype, int padlen) {
    if (k.sos.empty()) throw std::invalid_argument("filtfilt: sos empty");
    if (n == 0) return {};

    const int edge = compute_edge((int)n, k.ntaps, padtype, padlen);
    const std::vector<double> ext = (edge > 0)
        ? pad_extend_1d_ptr(x, n, edge, padtype)
        : std::vector<double>(x, x + n);

    // zi base: use cached if present, else compute
    std::vector<double> zi_base = k.zi;
    if (zi_base.empty()) zi_base = sosfilt_zi(k.sos);

    // forward with zi*x0
    std::vector<double> zi_f;
    const std::vector<double>* pzi_f = nullptr;
    if (!zi_base.empty()) {
        zi_f.resize(zi_base.size());
        const double x0 = ext.front();
        for (size_t i = 0; i < zi_base.size(); ++i) zi_f[i] = zi_base[i] * x0;
        pzi_f = &zi_f;
    }

    auto y_pair = sosfilt_df2t(k.sos, ext.data(), ext.size(), pzi_f);
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

    auto y2_pair = sosfilt_df2t(k.sos, y.data(), y.size(), pzi_b);
    std::vector<double> y2 = std::move(y2_pair.first);
    std::reverse(y2.begin(), y2.end());

    if (edge == 0) return y2;
    return std::vector<double>(y2.begin() + edge, y2.end() - edge);
}

// ============================================================
//              Butterworth design implementation
// ============================================================

std::vector<double> ButterworthFilter::poly(const std::vector<std::complex<double>>& roots) {
    // Compute polynomial coefficients from roots
    const int n = (int)roots.size();
    std::vector<std::complex<double>> coeffs(n + 1, 0.0);
    coeffs[0] = 1.0;
    
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j > 0; --j) {
            coeffs[j] = coeffs[j] - roots[i] * coeffs[j - 1];
        }
    }
    
    // Convert to real (imaginary parts should be negligible)
    std::vector<double> result(n + 1);
    for (int i = 0; i <= n; ++i) {
        result[i] = std::real(coeffs[i]);
    }
    return result;
}

ButterworthFilter::ComplexPair ButterworthFilter::buttap_zp(int n) {
    // Butterworth analog lowpass prototype: no finite zeros
    ComplexPair zp;
    zp.p.resize(n);
    
    const double pi = M_PI;
    for (int k = 0; k < n; ++k) {
        double angle = pi * (2.0 * k + n + 1.0) / (2.0 * n);
        zp.p[k] = std::exp(std::complex<double>(0.0, angle));
    }
    
    return zp;
}

ButterworthFilter::ComplexPair ButterworthFilter::lp2lp_zp(const ComplexPair& zp, double wo) {
    ComplexPair result;
    result.z.resize(zp.z.size());
    result.p.resize(zp.p.size());
    
    for (size_t i = 0; i < zp.z.size(); ++i) {
        result.z[i] = zp.z[i] * wo;
    }
    for (size_t i = 0; i < zp.p.size(); ++i) {
        result.p[i] = zp.p[i] * wo;
    }
    
    return result;
}

ButterworthFilter::ComplexPair ButterworthFilter::lp2hp_zp(const ComplexPair& zp, double wo) {
    ComplexPair result;
    const int degree = (int)zp.p.size() - (int)zp.z.size();
    
    // Transform existing zeros
    for (size_t i = 0; i < zp.z.size(); ++i) {
        result.z.push_back(wo / zp.z[i]);
    }
    
    // Transform poles
    for (size_t i = 0; i < zp.p.size(); ++i) {
        result.p.push_back(wo / zp.p[i]);
    }
    
    // Add zeros at origin for degree > 0
    for (int i = 0; i < degree; ++i) {
        result.z.push_back(0.0);
    }
    
    return result;
}

ButterworthFilter::ComplexPair ButterworthFilter::lp2bp_zp(const ComplexPair& zp, double wo, double bw) {
    ComplexPair result;
    const int degree = (int)zp.p.size() - (int)zp.z.size();
    
    // Transform zeros
    for (size_t i = 0; i < zp.z.size(); ++i) {
        std::complex<double> t = 0.5 * bw * zp.z[i];
        std::complex<double> r = std::sqrt(t * t - wo * wo);
        result.z.push_back(t + r);
        result.z.push_back(t - r);
    }
    
    // Transform poles
    for (size_t i = 0; i < zp.p.size(); ++i) {
        std::complex<double> t = 0.5 * bw * zp.p[i];
        std::complex<double> r = std::sqrt(t * t - wo * wo);
        result.p.push_back(t + r);
        result.p.push_back(t - r);
    }
    
    // Add zeros at origin for degree > 0
    for (int i = 0; i < degree; ++i) {
        result.z.push_back(0.0);
    }
    
    return result;
}

ButterworthFilter::ComplexPair ButterworthFilter::lp2bs_zp(const ComplexPair& zp, double wo, double bw) {
    ComplexPair result;
    const int degree = (int)zp.p.size() - (int)zp.z.size();
    
    // Transform zeros
    for (size_t i = 0; i < zp.z.size(); ++i) {
        std::complex<double> t = 0.5 * bw / zp.z[i];
        std::complex<double> r = std::sqrt(t * t - wo * wo);
        result.z.push_back(t + r);
        result.z.push_back(t - r);
    }
    
    // Transform poles
    for (size_t i = 0; i < zp.p.size(); ++i) {
        std::complex<double> t = 0.5 * bw / zp.p[i];
        std::complex<double> r = std::sqrt(t * t - wo * wo);
        result.p.push_back(t + r);
        result.p.push_back(t - r);
    }
    
    // Add zeros at +/- j*wo for degree > 0
    for (int i = 0; i < degree; ++i) {
        result.z.push_back(std::complex<double>(0.0, wo));
        result.z.push_back(std::complex<double>(0.0, -wo));
    }
    
    return result;
}

ButterworthFilter::ComplexPair ButterworthFilter::bilinear_zp(const ComplexPair& zp, double fs) {
    ComplexPair result;
    const double fs2 = 2.0 * fs;
    const int degree = (int)zp.p.size() - (int)zp.z.size();
    
    // Transform zeros
    for (size_t i = 0; i < zp.z.size(); ++i) {
        result.z.push_back((fs2 + zp.z[i]) / (fs2 - zp.z[i]));
    }
    
    // Transform poles
    for (size_t i = 0; i < zp.p.size(); ++i) {
        result.p.push_back((fs2 + zp.p[i]) / (fs2 - zp.p[i]));
    }
    
    // Zeros at infinity -> z = -1
    for (int i = 0; i < degree; ++i) {
        result.z.push_back(-1.0);
    }
    
    return result;
}

std::vector<double> ButterworthFilter::normalize_passband_gain(const std::vector<double>& b,
                                                               const std::vector<double>& a,
                                                               double w) {
    // Compute H(e^{jw}) = sum(b * e^{-jwk}) / sum(a * e^{-jwk})
    std::complex<double> num(0.0, 0.0);
    std::complex<double> den(0.0, 0.0);
    
    for (size_t k = 0; k < b.size(); ++k) {
        std::complex<double> ej = std::exp(std::complex<double>(0.0, -w * (double)k));
        num += b[k] * ej;
    }
    
    for (size_t k = 0; k < a.size(); ++k) {
        std::complex<double> ej = std::exp(std::complex<double>(0.0, -w * (double)k));
        den += a[k] * ej;
    }
    
    std::complex<double> H = num / den;
    double gain = 1.0 / (std::abs(H) + 1e-30);
    
    std::vector<double> b_norm(b.size());
    for (size_t i = 0; i < b.size(); ++i) {
        b_norm[i] = b[i] * gain;
    }
    
    return b_norm;
}

std::pair<std::vector<double>, std::vector<double>>
ButterworthFilter::butter_ba(int order, double fs, const std::string& btype, const std::vector<double>& cutoff) {
    if (order <= 0) {
        throw std::invalid_argument("order must be positive");
    }
    if (fs <= 0.0) {
        throw std::invalid_argument("fs must be > 0");
    }
    
    const double fs2 = 2.0 * fs;
    const double pi = M_PI;
    
    // Prewarp function
    auto prewarp = [&](double f_hz) {
        if (f_hz <= 0.0 || f_hz >= 0.5 * fs) {
            throw std::invalid_argument("cutoff must satisfy 0 < f < fs/2");
        }
        return fs2 * std::tan(pi * f_hz / fs);
    };
    
    // 1) Analog prototype (wc=1 rad/s)
    ComplexPair zp = buttap_zp(order);
    
    // 2) Analog frequency transform
    double w_norm = 0.0;
    
    if (btype == "lowpass" || btype == "highpass") {
        if (cutoff.size() != 1) {
            throw std::invalid_argument("lowpass/highpass requires single cutoff frequency");
        }
        double wc = prewarp(cutoff[0]);
        
        if (btype == "lowpass") {
            zp = lp2lp_zp(zp, wc);
            w_norm = 0.0;
        } else {
            zp = lp2hp_zp(zp, wc);
            w_norm = pi;
        }
    } else if (btype == "bandpass" || btype == "bandstop") {
        if (cutoff.size() != 2) {
            throw std::invalid_argument("bandpass/bandstop requires two cutoff frequencies");
        }
        double w1 = prewarp(cutoff[0]);
        double w2 = prewarp(cutoff[1]);
        
        if (w2 <= w1) {
            throw std::invalid_argument("band cutoff must satisfy low < high");
        }
        
        double w0 = std::sqrt(w1 * w2);
        double bw = w2 - w1;
        
        if (btype == "bandpass") {
            zp = lp2bp_zp(zp, w0, bw);
            w_norm = 2.0 * std::atan(w0 / fs2);
        } else {
            zp = lp2bs_zp(zp, w0, bw);
            w_norm = 0.0;
        }
    } else {
        throw std::invalid_argument("btype must be one of: lowpass/highpass/bandpass/bandstop");
    }
    
    // 3) Bilinear transform (analog -> digital)
    zp = bilinear_zp(zp, fs);
    
    // 4) zpk -> ba
    std::vector<double> b = poly(zp.z);
    std::vector<double> a = poly(zp.p);
    
    // Normalize a[0] = 1
    if (a.empty() || a[0] == 0.0) {
        throw std::runtime_error("butter_ba: invalid denominator");
    }
    
    double a0 = a[0];
    for (size_t i = 0; i < b.size(); ++i) b[i] /= a0;
    for (size_t i = 0; i < a.size(); ++i) a[i] /= a0;
    
    // 5) Normalize passband gain
    b = normalize_passband_gain(b, a, w_norm);
    
    return {b, a};
}
