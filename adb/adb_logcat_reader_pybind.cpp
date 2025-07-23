#include <pybind11/pybind11.h>
#include "AdbLogcatReader.h"

namespace py = pybind11;

PYBIND11_MODULE(adb_logcat_reader, m) {
	py::class_<ADBLogcatReader>(m, "ADBLogcatReader")
		.def(py::init<>())
		.def("clearLogcat", &ADBLogcatReader::clearLogcat)
		.def("startLogcat", &ADBLogcatReader::startLogcat)
		.def("readLine", &ADBLogcatReader::readLine);
}
