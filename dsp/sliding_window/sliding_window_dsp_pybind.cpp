#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>  // 包含这个头文件
#include "MKAverage.h"
#include "MonoQueue.h"
#include "WelfordStd.h"

namespace py = pybind11;

PYBIND11_MODULE(sliding_window_dsp, m) {
    py::class_<MKAverage>(m, "MKAverage")
        .def(py::init<int, int>())
        .def("addElement", &MKAverage::addElement)
        .def("calculateMKAverage", &MKAverage::calculateMKAverage);

    py::class_<MonoQueue>(m, "MonoQueue")
        .def(py::init<int>())
        .def("push", &MonoQueue::push)
        .def("max", &MonoQueue::max);

    py::class_<WelfordStd>(m, "WelfordStd")
        .def(py::init<int>())
        .def("calcSlidingStd", &WelfordStd::calcSlidingStd)
        .def("getStd", &WelfordStd::getStd)
        .def("getCnt", &WelfordStd::getCnt);
}
