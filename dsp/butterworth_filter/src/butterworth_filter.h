#ifndef BUTTERWORTH_FILTER_HPP
#define BUTTERWORTH_FILTER_HPP

#include <vector>
#include <utility>
#include <cstddef>

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
                      bool precompute = true);

    // ---- main APIs (vector) ----
    std::vector<double> filter(const std::vector<double>& x,
                               PadType padtype = PadType::Odd,
                               int padlen = -1) const;

    std::vector<double> filtfilt(const std::vector<double>& x,
                                 PadType padtype = PadType::Odd,
                                 int padlen = -1) const;

    std::pair<std::vector<double>, std::vector<double>>
    lfilter(const std::vector<double>& x,
            const std::vector<double>* zi = nullptr) const;

    std::vector<double> detrend(const std::vector<double>& x) const;

    std::vector<double> lfilterZi() const;

    // ---- zero-copy friendly APIs (ptr) ----
    std::vector<double> filter(const double* x, size_t n,
                               PadType padtype = PadType::Odd,
                               int padlen = -1) const;

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
};

#endif
