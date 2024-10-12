#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "apriltags/TagDetector.h"
#include "apriltags/TagFamily.h"
#include "apriltags/Tag16h5.h"
#include "apriltags/Tag25h7.h"
#include "apriltags/Tag25h9.h"
#include "apriltags/Tag36h9.h"
#include "apriltags/Tag36h11.h"

namespace py = pybind11;

PYBIND11_MODULE(apriltag_detection, m) {
    py::class_<AprilTags::TagDetection>(m, "TagDetection")
        .def(py::init<>())
        .def_readwrite("id", &AprilTags::TagDetection::id)
        .def_readwrite("hamming_distance", &AprilTags::TagDetection::hammingDistance)
        .def_property_readonly("corners", [](const AprilTags::TagDetection& self) {
            std::vector<std::pair<float, float>> corners;
            for (int i = 0; i < 4; ++i) {
                corners.push_back(self.p[i]);
            }
            return corners;
        })
        .def_readwrite("center", &AprilTags::TagDetection::cxy);

    py::class_<AprilTags::TagCodes>(m, "TagCodes")
        .def(py::init<int, int, const unsigned long long*, int>())
        .def_readwrite("bits", &AprilTags::TagCodes::bits)
        .def_readwrite("min_hamming_distance", &AprilTags::TagCodes::minHammingDistance)
        .def_readwrite("codes", &AprilTags::TagCodes::codes);

    py::class_<AprilTags::TagDetector, std::shared_ptr<AprilTags::TagDetector>>(m, "TagDetector")
        .def(py::init<const AprilTags::TagCodes&, const size_t>())
        .def("extract_tags", [](AprilTags::TagDetector& self, py::array_t<uint8_t> image) {
            cv::Mat cv_image(image.shape(0), image.shape(1), CV_8UC1, image.mutable_data());
            return self.extractTags(cv_image);
        });

    m.def("tag_codes_16h5", []() -> AprilTags::TagCodes { 
        return AprilTags::tagCodes16h5; 
    });
    m.def("tag_codes_25h7", []() -> AprilTags::TagCodes { 
        return AprilTags::tagCodes25h7; 
    });
    m.def("tag_codes_25h9", []() -> AprilTags::TagCodes { 
        return AprilTags::tagCodes25h9; 
    });
    m.def("tag_codes_36h9", []() -> AprilTags::TagCodes { 
        return AprilTags::tagCodes36h9; 
    });
    m.def("tag_codes_36h11", []() -> AprilTags::TagCodes { 
        return AprilTags::tagCodes36h11; 
    });
}
