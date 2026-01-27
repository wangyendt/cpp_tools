#ifndef BUTTERWORTH_FILTER_HPP
#define BUTTERWORTH_FILTER_HPP

#include <array>
#include <complex>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class ButterworthFilter {
public:
    enum class PadType { None, Odd, Even, Constant };

    // SciPy SOS format: each row is [b0, b1, b2, a0, a1, a2]
    using SOSSection = std::array<double, 6>;

public:
    // ---- Factory methods ----
    // 从 b/a 系数创建滤波器 (适用于低阶滤波器)
    static ButterworthFilter from_ba(const std::vector<double>& b,
                                     const std::vector<double>& a,
                                     bool cache_zi = true);

    // 从 SOS (Second-Order Sections) 创建滤波器 (推荐用于高阶或接近 Nyquist 频率的设计)
    static ButterworthFilter from_sos(const std::vector<SOSSection>& sos,
                                      bool cache_zi = true);

    // 从参数直接设计 Butterworth 滤波器
    static ButterworthFilter from_params(int order,
                                        double fs,
                                        const std::string& btype,
                                        const std::vector<double>& cutoff,
                                        bool cache_zi = true);

    // ---- Core filtering APIs ----
    // 零相位滤波 (forward-backward filtering)
    std::vector<double> filtfilt(const std::vector<double>& x,
                                 PadType padtype = PadType::Odd,
                                 int padlen = -1) const;

    std::vector<double> filtfilt(const double* x, size_t n,
                                 PadType padtype = PadType::Odd,
                                 int padlen = -1) const;

    // 单向滤波,返回 (y, zf)
    std::pair<std::vector<double>, std::vector<double>>
    lfilter(const std::vector<double>& x,
            const std::vector<double>* zi = nullptr) const;

    std::pair<std::vector<double>, std::vector<double>>
    lfilter(const double* x, size_t n,
            const std::vector<double>* zi = nullptr) const;

    // 去趋势 (线性去趋势)
    std::vector<double> detrend(const std::vector<double>& x) const;
    std::vector<double> detrend(const double* x, size_t n) const;

    // ---- 初始状态计算 (与 SciPy API 一致) ----
    // 从 b/a 系数计算 lfilter 初始状态 (等价于 scipy.signal.lfilter_zi)
    static std::vector<double> lfilter_zi(const std::vector<double>& b,
                                          const std::vector<double>& a);

    // 从 SOS 计算 sosfilt 初始状态 (等价于 scipy.signal.sosfilt_zi)
    static std::vector<double> sosfilt_zi(const std::vector<SOSSection>& sos);

private:
    // 内部数据结构
    struct BAKernel {
        std::vector<double> b;   // normalized
        std::vector<double> a;   // normalized
        std::vector<double> zi;  // lfilter_zi(b,a)
        int ntaps = 0;           // max(len(a),len(b))
    };

    struct SOSKernel {
        std::vector<SOSSection> sos; // normalized per-section (a0==1)
        std::vector<double> zi;      // sosfilt_zi flattened: [z1_0,z2_0,z1_1,z2_1,...]
        int n_sections = 0;
        int ntaps = 0;               // SciPy's 'ntaps' notion for sosfiltfilt padlen heuristic
    };

    enum class Mode { BA, SOS };
    Mode mode_ = Mode::BA;

    BAKernel ba_kernel_;
    SOSKernel sos_kernel_;
    bool precompute_ = true;

private:
    // 私有构造函数
    ButterworthFilter() = default;

private:
    // BA 相关辅助函数
    static void normalize_ba(std::vector<double>& b, std::vector<double>& a);
    static std::pair<std::vector<double>, std::vector<double>>
    lfilter_df2t(std::vector<double> b, std::vector<double> a,
                 const double* x, size_t n,
                 const std::vector<double>* zi);

    // SOS 相关辅助函数
    static void normalize_sos(std::vector<SOSSection>& sos);
    static int sos_ntaps(const std::vector<SOSSection>& sos);
    static std::pair<std::vector<double>, std::vector<double>>
    sosfilt_df2t(const std::vector<SOSSection>& sos,
                 const double* x, size_t n,
                 const std::vector<double>* zi);

    // 通用辅助函数
    static int compute_edge(int x_len, int ntaps, PadType padtype, int padlen);
    static std::vector<double> pad_extend_1d_ptr(const double* x, size_t n, int edge, PadType padtype);

    // 核心滤波实现
    static std::pair<std::vector<double>, std::vector<double>>
    lfilter_impl(const BAKernel& k,
                 const double* x, size_t n,
                 const std::vector<double>* zi);

    static std::pair<std::vector<double>, std::vector<double>>
    lfilter_impl(const SOSKernel& k,
                 const double* x, size_t n,
                 const std::vector<double>* zi);

    static std::vector<double>
    filtfilt_impl(const BAKernel& k,
                  const double* x, size_t n,
                  PadType padtype, int padlen);

    static std::vector<double>
    filtfilt_impl(const SOSKernel& k,
                  const double* x, size_t n,
                  PadType padtype, int padlen);

    // Butterworth 设计辅助函数
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
