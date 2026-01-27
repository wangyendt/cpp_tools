#ifndef BUTTERWORTH_FILTER_HPP
#define BUTTERWORTH_FILTER_HPP

#include <vector>
#include <utility>
#include <cstddef>
#include <string>

class ButterworthFilter {
public:
    enum class PadType { None, Odd, Even, Constant };

    struct Kernel {
        std::vector<double> b;   // normalized
        std::vector<double> a;   // normalized
        std::vector<double> zi;  // lfilter_zi(b,a)
        int ntaps = 0;           // max(len(a),len(b))
    };

public:
    ButterworthFilter(const std::vector<double>& b,
                      const std::vector<double>& a,
                      bool cache_zi = true);

    // ---- factory methods ----
    static ButterworthFilter from_ba(const std::vector<double>& b,
                                     const std::vector<double>& a,
                                     bool cache_zi = true);

    static ButterworthFilter from_params(int order,
                                        double fs,
                                        const std::string& btype,
                                        const std::vector<double>& cutoff,
                                        bool cache_zi = true);

    // ---- main APIs (vector) ----
    std::vector<double> filtfilt(const std::vector<double>& x,
                                 PadType padtype = PadType::Odd,
                                 int padlen = -1) const;

    std::pair<std::vector<double>, std::vector<double>>
    lfilter(const std::vector<double>& x,
            const std::vector<double>* zi = nullptr) const;

    std::vector<double> detrend(const std::vector<double>& x) const;

    std::vector<double> lfilterZi() const;

    // ---- zero-copy friendly APIs (ptr) ----
    std::vector<double> filtfilt(const double* x, size_t n,
                                 PadType padtype = PadType::Odd,
                                 int padlen = -1) const;

    std::pair<std::vector<double>, std::vector<double>>
    lfilter(const double* x, size_t n,
            const std::vector<double>* zi = nullptr) const;

    std::vector<double> detrend(const double* x, size_t n) const;

public:
    // ---- precompute kernel (recommended for fixed b/a) ----
    static Kernel precompute(const std::vector<double>& b,
                             const std::vector<double>& a);

    // kernel + vector
    static std::vector<double> filtfilt(const Kernel& k,
                                        const std::vector<double>& x,
                                        PadType padtype = PadType::Odd,
                                        int padlen = -1);

    static std::pair<std::vector<double>, std::vector<double>>
    lfilter(const Kernel& k,
            const std::vector<double>& x,
            const std::vector<double>* zi = nullptr);

    // kernel + ptr
    static std::vector<double> filtfilt(const Kernel& k,
                                        const double* x, size_t n,
                                        PadType padtype = PadType::Odd,
                                        int padlen = -1);

    static std::pair<std::vector<double>, std::vector<double>>
    lfilter(const Kernel& k,
            const double* x, size_t n,
            const std::vector<double>* zi = nullptr);

private:
    Kernel k_;
    bool precompute_ = true;

private:
    static void normalize_ba(std::vector<double>& b, std::vector<double>& a);

    static int compute_edge(int x_len, int ntaps, PadType padtype, int padlen);

    static std::vector<double> pad_extend_1d(const std::vector<double>& x, int edge, PadType padtype);
    static std::vector<double> pad_extend_1d_ptr(const double* x, size_t n, int edge, PadType padtype);

    static std::vector<double> lfilter_zi(std::vector<double> b, std::vector<double> a);

    // DF2T
    static std::pair<std::vector<double>, std::vector<double>>
    lfilter_df2t(std::vector<double> b, std::vector<double> a,
                 const double* x, size_t n,
                 const std::vector<double>* zi);

    // Butterworth design helpers
    struct ComplexPair {
        std::vector<std::complex<double>> z;
        std::vector<std::complex<double>> p;
    };

    static std::pair<std::vector<double>, std::vector<double>>
    butter_ba(int order, double fs, const std::string& btype, const std::vector<double>& cutoff);

    static std::vector<double> normalize_passband_gain(const std::vector<double>& b,
                                                       const std::vector<double>& a,
                                                       double w);

    static ComplexPair buttap_zp(int n);
    static ComplexPair lp2lp_zp(const ComplexPair& zp, double wo);
    static ComplexPair lp2hp_zp(const ComplexPair& zp, double wo);
    static ComplexPair lp2bp_zp(const ComplexPair& zp, double wo, double bw);
    static ComplexPair lp2bs_zp(const ComplexPair& zp, double wo, double bw);
    static ComplexPair bilinear_zp(const ComplexPair& zp, double fs);

    static std::vector<double> poly(const std::vector<std::complex<double>>& roots);
};

#endif
