#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

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

static std::pair<const double*, size_t> as_ptr_len_1d(const py::array& a) {
    require_1d_c_f64(a);
    py::buffer_info info = a.request();
    return {static_cast<const double*>(info.ptr), (size_t)info.shape[0]};
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

    // ----- Kernel -----
    py::class_<ButterworthFilter::Kernel>(m, "Kernel")
        .def(py::init<>())
        .def_readwrite("b", &ButterworthFilter::Kernel::b)
        .def_readwrite("a", &ButterworthFilter::Kernel::a)
        .def_readwrite("zi", &ButterworthFilter::Kernel::zi)
        .def_readwrite("ntaps", &ButterworthFilter::Kernel::ntaps);

    // ----- ButterworthFilter -----
    py::class_<ButterworthFilter>(m, "ButterworthFilter")
        .def(py::init<const std::vector<double>&, const std::vector<double>&, bool>(),
             py::arg("b"), py::arg("a"), py::arg("precompute") = true)

        // ========== numpy zero-copy main APIs ==========
        .def("filter",
             [](const ButterworthFilter& self,
                const py::array& x,
                ButterworthFilter::PadType padtype,
                int padlen) {
                 auto [ptr, n] = as_ptr_len_1d(x);
                 auto y = self.filter(ptr, n, padtype, padlen);
                 return vec_to_ndarray(std::move(y));
             },
             py::arg("x"),
             py::arg("padtype") = ButterworthFilter::PadType::Odd,
             py::arg("padlen") = -1)

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

        .def("lfilterZi",
             [](const ButterworthFilter& self) {
                 return vec_to_ndarray(self.lfilterZi());
             })

        // ========== list compatibility (will copy) ==========
        .def("filter_list",
             [](const ButterworthFilter& self,
                const std::vector<double>& x,
                ButterworthFilter::PadType padtype,
                int padlen) {
                 return self.filter(x, padtype, padlen);
             },
             py::arg("x"),
             py::arg("padtype") = ButterworthFilter::PadType::Odd,
             py::arg("padlen") = -1)

        .def("filtfilt_list",
             [](const ButterworthFilter& self,
                const std::vector<double>& x,
                ButterworthFilter::PadType padtype,
                int padlen) {
                 return self.filtfilt(x, padtype, padlen);
             },
             py::arg("x"),
             py::arg("padtype") = ButterworthFilter::PadType::Odd,
             py::arg("padlen") = -1)

        .def("detrend_list",
             [](const ButterworthFilter& self, const std::vector<double>& x) {
                 return self.detrend(x);
             },
             py::arg("x"))

        // ========== kernel APIs ==========
        .def_static("precompute", &ButterworthFilter::precompute,
                    py::arg("b"), py::arg("a"))

        .def_static("filtfilt_kernel",
                    [](const ButterworthFilter::Kernel& k,
                       const py::array& x,
                       ButterworthFilter::PadType padtype,
                       int padlen) {
                        auto [ptr, n] = as_ptr_len_1d(x);
                        auto y = ButterworthFilter::filtfilt(k, ptr, n, padtype, padlen);
                        return vec_to_ndarray(std::move(y));
                    },
                    py::arg("kernel"),
                    py::arg("x"),
                    py::arg("padtype") = ButterworthFilter::PadType::Odd,
                    py::arg("padlen") = -1)

        .def_static("lfilter_kernel",
                    [](const ButterworthFilter::Kernel& k,
                       const py::array& x,
                       py::object zi_obj) {
                        auto [ptr, n] = as_ptr_len_1d(x);

                        if (zi_obj.is_none()) {
                            auto yz = ButterworthFilter::lfilter(k, ptr, n, nullptr);
                            return py::make_tuple(vec_to_ndarray(std::move(yz.first)),
                                                  vec_to_ndarray(std::move(yz.second)));
                        }

                        std::vector<double> zi = zi_obj.cast<std::vector<double>>();
                        auto yz = ButterworthFilter::lfilter(k, ptr, n, &zi);
                        return py::make_tuple(vec_to_ndarray(std::move(yz.first)),
                                              vec_to_ndarray(std::move(yz.second)));
                    },
                    py::arg("kernel"),
                    py::arg("x"),
                    py::arg("zi") = py::none());
}
