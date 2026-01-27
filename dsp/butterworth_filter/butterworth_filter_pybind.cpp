#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <array>
#include <vector>

#include "butterworth_filter.h"

namespace py = pybind11;

static void require_1d_c_f64(const py::array& a) {
    if (!a.dtype().is(py::dtype::of<double>()))
        throw std::invalid_argument("expected np.ndarray dtype=float64");
    if (a.ndim() != 1)
        throw std::invalid_argument("expected 1D array");
    if (!(a.flags() & py::array::c_style))
        throw std::invalid_argument("expected C-contiguous array (use np.ascontiguousarray(x, dtype=np.float64))");

    py::buffer_info info = a.request();
    if (info.strides[0] != (py::ssize_t)sizeof(double))
        throw std::invalid_argument("expected contiguous stride (use np.ascontiguousarray)");
}

static void require_2d_c_f64(const py::array& a, py::ssize_t cols) {
    if (!a.dtype().is(py::dtype::of<double>()))
        throw std::invalid_argument("expected np.ndarray dtype=float64");
    if (a.ndim() != 2)
        throw std::invalid_argument("expected 2D array");
    if (a.shape(1) != cols)
        throw std::invalid_argument("unexpected shape: sos must be (n_sections, 6)");
    if (!(a.flags() & py::array::c_style))
        throw std::invalid_argument("expected C-contiguous array (use np.ascontiguousarray(sos, dtype=np.float64))");

    py::buffer_info info = a.request();
    if (info.strides[1] != (py::ssize_t)sizeof(double))
        throw std::invalid_argument("expected contiguous inner stride");
}

static std::pair<const double*, size_t> as_ptr_len_1d(const py::array& a) {
    require_1d_c_f64(a);
    py::buffer_info info = a.request();
    return {static_cast<const double*>(info.ptr), (size_t)info.shape[0]};
}

static std::vector<ButterworthFilter::SOSSection> as_sos_sections(const py::object& sos_obj) {
    std::vector<ButterworthFilter::SOSSection> sos;

    if (py::isinstance<py::array>(sos_obj)) {
        py::array a = sos_obj.cast<py::array>();
        require_2d_c_f64(a, 6);

        py::buffer_info info = a.request();
        const auto nsec = (size_t)info.shape[0];
        const auto stride0 = (size_t)info.strides[0];
        const auto* base = static_cast<const unsigned char*>(info.ptr);

        sos.resize(nsec);
        for (size_t i = 0; i < nsec; ++i) {
            const double* row = reinterpret_cast<const double*>(base + i * stride0);
            sos[i] = {row[0], row[1], row[2], row[3], row[4], row[5]};
        }
        return sos;
    }

    // list[list[float]] or list[tuple[float,...]] etc.
    sos = sos_obj.cast<std::vector<ButterworthFilter::SOSSection>>();
    return sos;
}

// Wrap vector<double> into numpy array with zero-copy output using capsule lifetime.
static py::array vec_to_ndarray(std::vector<double>&& v) {
    auto* pv = new std::vector<double>(std::move(v));
    py::capsule cap(pv, [](void* p) { delete reinterpret_cast<std::vector<double>*>(p); });

    return py::array(
        py::buffer_info(
            pv->data(),
            (py::ssize_t)sizeof(double),
            py::format_descriptor<double>::format(),
            1,
            {(py::ssize_t)pv->size()},
            {(py::ssize_t)sizeof(double)}
        ),
        cap
    );
}

PYBIND11_MODULE(butterworth_filter, m) {
    m.doc() = "SciPy-matching filtfilt/lfilter with full-chain zero-copy numpy I/O";

    // ----- PadType -----
    py::enum_<ButterworthFilter::PadType>(m, "PadType")
        .value("NoPad", ButterworthFilter::PadType::None)
        .value("Odd", ButterworthFilter::PadType::Odd)
        .value("Even", ButterworthFilter::PadType::Even)
        .value("Constant", ButterworthFilter::PadType::Constant)
        .export_values();

    // ----- ButterworthFilter -----
    py::class_<ButterworthFilter>(m, "ButterworthFilter")
        // 构造函数 (转发到 from_ba)
        .def(py::init([](const std::vector<double>& b, const std::vector<double>& a, bool cache_zi) {
                 return ButterworthFilter::from_ba(b, a, cache_zi);
             }),
             py::arg("b"), py::arg("a"), py::arg("cache_zi") = true)

        // ========== factory methods ==========
        .def_static("from_ba",
                    &ButterworthFilter::from_ba,
                    py::arg("b"),
                    py::arg("a"),
                    py::arg("cache_zi") = true,
                    "Create filter from b and a coefficients")

        .def_static("from_sos",
                    [](py::object sos_obj, bool cache_zi) {
                        auto sos = as_sos_sections(sos_obj);
                        return ButterworthFilter::from_sos(sos, cache_zi);
                    },
                    py::arg("sos"),
                    py::arg("cache_zi") = true,
                    "Create filter from SOS array (n_sections,6): [b0,b1,b2,a0,a1,a2]")

        .def_static("from_params",
                    &ButterworthFilter::from_params,
                    py::arg("order"),
                    py::arg("fs"),
                    py::arg("btype"),
                    py::arg("cutoff"),
                    py::arg("cache_zi") = true,
                    "Create Butterworth filter from parameters")

        // ========== numpy zero-copy main APIs ==========
        .def("filtfilt",
             [](const ButterworthFilter& self,
                const py::array& x,
                ButterworthFilter::PadType padtype,
                int padlen) {
                 auto [ptr, n] = as_ptr_len_1d(x);
                 auto y = self.filtfilt(ptr, n, padtype, padlen);
                 return vec_to_ndarray(std::move(y));
             },
             py::arg("x"),
             py::arg("padtype") = ButterworthFilter::PadType::Odd,
             py::arg("padlen") = -1)

        .def("lfilter",
             [](const ButterworthFilter& self,
                const py::array& x,
                py::object zi_obj) {
                 auto [ptr, n] = as_ptr_len_1d(x);

                 if (zi_obj.is_none()) {
                     auto yz = self.lfilter(ptr, n, nullptr);
                     return py::make_tuple(vec_to_ndarray(std::move(yz.first)),
                                           vec_to_ndarray(std::move(yz.second)));
                 }

                 // zi 很短，拷贝成本可忽略；也允许传 ndarray/list
                 std::vector<double> zi = zi_obj.cast<std::vector<double>>();
                 auto yz = self.lfilter(ptr, n, &zi);
                 return py::make_tuple(vec_to_ndarray(std::move(yz.first)),
                                       vec_to_ndarray(std::move(yz.second)));
             },
             py::arg("x"),
             py::arg("zi") = py::none())

        .def("detrend",
             [](const ButterworthFilter& self, const py::array& x) {
                 auto [ptr, n] = as_ptr_len_1d(x);
                 auto y = self.detrend(ptr, n);
                 return vec_to_ndarray(std::move(y));
             },
             py::arg("x"))

        // ========== 初始状态计算 (与 SciPy API 一致) ==========
        .def_static("lfilter_zi",
                    [](const std::vector<double>& b, const std::vector<double>& a) {
                        return vec_to_ndarray(ButterworthFilter::lfilter_zi(b, a));
                    },
                    py::arg("b"), py::arg("a"),
                    "计算 lfilter 初始状态 (等价于 scipy.signal.lfilter_zi)")

        .def_static("sosfilt_zi",
                    [](py::object sos_obj) {
                        auto sos = as_sos_sections(sos_obj);
                        return vec_to_ndarray(ButterworthFilter::sosfilt_zi(sos));
                    },
                    py::arg("sos"),
                    "计算 sosfilt 初始状态 (等价于 scipy.signal.sosfilt_zi)");
}
