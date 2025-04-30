#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <opencv2/core/core.hpp>

// Camera model headers
#include "Camera.h"
#include "PinholeCamera.h"
#include "CataCamera.h"
#include "EquidistantCamera.h"
#include "PinholeFullCamera.h"
#include "ScaramuzzaCamera.h"
#include "CameraFactory.h"

// --- Boost shared_ptr support ---
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);
// Define the holder type in this implementation file
namespace pybind11 { namespace detail {
    template <typename T>
    struct holder_helper<boost::shared_ptr<T>> { // specialization
        static const T *get(const boost::shared_ptr<T> &p) { return p.get(); }
    };
}}


namespace py = pybind11;

// --- Custom Type Casters ---

// cv::Size <-> tuple[int, int]
namespace pybind11 { namespace detail {
    template <> struct type_caster<cv::Size> {
    public:
        PYBIND11_TYPE_CASTER(cv::Size, _("tuple_int_int"));

        bool load(handle src, bool /* convert */) {
            if (!py::isinstance<py::tuple>(src)) return false;
            auto t = py::reinterpret_borrow<py::tuple>(src);
            if (t.size() != 2) return false;
            value = cv::Size(t[0].cast<int>(), t[1].cast<int>());
            return true;
        }

        static handle cast(const cv::Size& src, return_value_policy /* policy */, handle /* parent */) {
            return py::make_tuple(src.width, src.height).release();
        }
    };
}} // namespace pybind11::detail

// cv::Point2f <-> tuple[float, float]
namespace pybind11 { namespace detail {
    template <> struct type_caster<cv::Point2f> {
    public:
        PYBIND11_TYPE_CASTER(cv::Point2f, _("tuple_float_float"));

        bool load(handle src, bool /* convert */) {
            if (!py::isinstance<py::tuple>(src)) return false;
            auto t = py::reinterpret_borrow<py::tuple>(src);
            if (t.size() != 2) return false;
            value = cv::Point2f(t[0].cast<float>(), t[1].cast<float>());
            return true;
        }

        static handle cast(const cv::Point2f& src, return_value_policy /* policy */, handle /* parent */) {
            return py::make_tuple(src.x, src.y).release();
        }
    };
}} // namespace pybind11::detail

// cv::Point3f <-> tuple[float, float, float]
namespace pybind11 { namespace detail {
    template <> struct type_caster<cv::Point3f> {
    public:
        PYBIND11_TYPE_CASTER(cv::Point3f, _("tuple_float_float_float"));

        bool load(handle src, bool /* convert */) {
            if (!py::isinstance<py::tuple>(src)) return false;
            auto t = py::reinterpret_borrow<py::tuple>(src);
            if (t.size() != 3) return false;
            value = cv::Point3f(t[0].cast<float>(), t[1].cast<float>(), t[2].cast<float>());
            return true;
        }

        static handle cast(const cv::Point3f& src, return_value_policy /* policy */, handle /* parent */) {
            return py::make_tuple(src.x, src.y, src.z).release();
        }
    };
}} // namespace pybind11::detail

// Note: cv::Mat caster is needed for methods like initUndistortMap, etc.
// Using a library like pybind11-opencv is recommended for full cv::Mat support.


PYBIND11_MODULE(camera_models, m) {
    m.doc() = "Python bindings for camera_models library";

    py::enum_<Camera::ModelType>(m, "ModelType")
        .value("KANNALA_BRANDT", Camera::ModelType::KANNALA_BRANDT)
        .value("MEI", Camera::ModelType::MEI)
        .value("PINHOLE", Camera::ModelType::PINHOLE)
        .value("PINHOLE_FULL", Camera::ModelType::PINHOLE_FULL)
        .value("SCARAMUZZA", Camera::ModelType::SCARAMUZZA)
        .export_values();

    // --- Base Camera Parameters ---
    // Bind as holder to allow inheritance in Python if needed, but cannot be instantiated.
    py::class_<Camera::Parameters, boost::shared_ptr<Camera::Parameters>>(m, "CameraParametersBase")
         .def_property_readonly("model_type", py::overload_cast<>(&Camera::Parameters::modelType, py::const_))
         .def_property("camera_name", py::overload_cast<>(&Camera::Parameters::cameraName, py::const_), py::overload_cast<>(&Camera::Parameters::cameraName))
         .def_property("image_width", py::overload_cast<>(&Camera::Parameters::imageWidth, py::const_), py::overload_cast<>(&Camera::Parameters::imageWidth))
         .def_property("image_height", py::overload_cast<>(&Camera::Parameters::imageHeight, py::const_), py::overload_cast<>(&Camera::Parameters::imageHeight))
         .def_property_readonly("n_intrinsics", &Camera::Parameters::nIntrinsics);
         // read/write Yaml are pure virtual

    // --- Base Camera ---
    // Bind as holder to allow inheritance in Python if needed, but cannot be instantiated.
    py::class_<Camera, boost::shared_ptr<Camera>>(m, "CameraBase")
        .def_property_readonly("model_type", &Camera::modelType)
        .def_property_readonly("camera_name", &Camera::cameraName)
        .def_property_readonly("image_width", &Camera::imageWidth)
        .def_property_readonly("image_height", &Camera::imageHeight)
        .def("lift_sphere", [](const Camera& self, const Eigen::Vector2d& p) {
            Eigen::Vector3d P;
            self.liftSphere(p, P);
            return P;
        }, py::arg("p"), "Lifts 2D image point to the unit sphere")
        .def("lift_projective", [](const Camera& self, const Eigen::Vector2d& p) {
            Eigen::Vector3d P;
            self.liftProjective(p, P);
            return P;
        }, py::arg("p"), "Lifts 2D image point to 3D projective ray")
        .def("space_to_plane", [](const Camera& self, const Eigen::Vector3d& P) {
            Eigen::Vector2d p;
            self.spaceToPlane(P, p);
            return p;
        }, py::arg("P"), "Projects 3D point to 2D image plane")
        .def("undist_to_plane", [](const Camera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d p;
            self.undistToPlane(p_u, p);
            return p;
        }, py::arg("p_u"), "Projects an undistorted 2D point to the image plane")
        .def("get_K", &Camera::getK, "Get intrinsic parameters [fx, fy, cx, cy] or similar")
        .def("write_parameters_to_yaml_file", &Camera::writeParametersToYamlFile, py::arg("filename"))
        .def("parameters_to_string", &Camera::parametersToString)
        // .def("estimate_intrinsics", ...) // Requires cv::Mat caster
        // .def("estimate_extrinsics", ...) // Requires cv::Mat caster
        // .def("init_undistort_rectify_map", ...) // Requires cv::Mat caster
        .def("reprojection_dist", &Camera::reprojectionDist, py::arg("P1"), py::arg("P2"), "Calculate reprojection distance between two 3D points")
        .def("reprojection_error", py::overload_cast<const Eigen::Vector3d&, const Eigen::Quaterniond&, const Eigen::Vector3d&, const Eigen::Vector2d&>(&Camera::reprojectionError, py::const_),
             py::arg("P"), py::arg("camera_q"), py::arg("camera_t"), py::arg("observed_p"))
        // .def("project_points", ...) // Requires cv::Mat caster
        .def("__repr__", &Camera::parametersToString);


    // --- Pinhole ---
    py::class_<PinholeCamera::Parameters, boost::shared_ptr<PinholeCamera::Parameters>, Camera::Parameters>(m, "PinholeCameraParameters")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("w"), py::arg("h"), py::arg("k1"), py::arg("k2"), py::arg("p1"), py::arg("p2"),
             py::arg("fx"), py::arg("fy"), py::arg("cx"), py::arg("cy"))
        .def_property("k1", [](const PinholeCamera::Parameters &p){ return p.k1(); }, [](PinholeCamera::Parameters &p, double v){ p.k1() = v; })
        .def_property("k2", [](const PinholeCamera::Parameters &p){ return p.k2(); }, [](PinholeCamera::Parameters &p, double v){ p.k2() = v; })
        .def_property("p1", [](const PinholeCamera::Parameters &p){ return p.p1(); }, [](PinholeCamera::Parameters &p, double v){ p.p1() = v; })
        .def_property("p2", [](const PinholeCamera::Parameters &p){ return p.p2(); }, [](PinholeCamera::Parameters &p, double v){ p.p2() = v; })
        .def_property("fx", [](const PinholeCamera::Parameters &p){ return p.fx(); }, [](PinholeCamera::Parameters &p, double v){ p.fx() = v; })
        .def_property("fy", [](const PinholeCamera::Parameters &p){ return p.fy(); }, [](PinholeCamera::Parameters &p, double v){ p.fy() = v; })
        .def_property("cx", [](const PinholeCamera::Parameters &p){ return p.cx(); }, [](PinholeCamera::Parameters &p, double v){ p.cx() = v; })
        .def_property("cy", [](const PinholeCamera::Parameters &p){ return p.cy(); }, [](PinholeCamera::Parameters &p, double v){ p.cy() = v; })
        .def("read_from_yaml_file", &PinholeCamera::Parameters::readFromYamlFile, py::arg("filename"))
        .def("write_to_yaml_file", &PinholeCamera::Parameters::writeToYamlFile, py::arg("filename"))
        .def("__repr__", [](const PinholeCamera::Parameters& p) { std::ostringstream oss; oss << p; return oss.str(); });

    py::class_<PinholeCamera, boost::shared_ptr<PinholeCamera>, Camera>(m, "PinholeCamera")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("image_width"), py::arg("image_height"), py::arg("k1"), py::arg("k2"), py::arg("p1"), py::arg("p2"), py::arg("fx"), py::arg("fy"), py::arg("cx"), py::arg("cy"))
        .def(py::init<const PinholeCamera::Parameters&>(), py::arg("params"))
        .def("get_parameters", [](const PinholeCamera& self) { return PinholeCamera::Parameters(self.getParameters()); }, py::return_value_policy::copy, "Get a copy of the camera parameters")
        .def("set_parameters", &PinholeCamera::setParameters, py::arg("parameters"))
        .def("distortion", [](const PinholeCamera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d d_u;
            self.distortion(p_u, d_u);
            return d_u;
        }, py::arg("p_u"), "Calculate distortion vector for a normalized point")
        .def("distortion_with_jacobian", [](const PinholeCamera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d d_u;
            Eigen::Matrix2d J;
            self.distortion(p_u, d_u, J);
            return std::make_tuple(d_u, J);
        }, py::arg("p_u"), "Calculate distortion vector and its Jacobian")
        .def("set_no_distortion", &PinholeCamera::setNoDistortion, py::arg("b_noDistortion"))
        .def("read_parameters", &PinholeCamera::readParameters, py::arg("parameter_vec"))
        .def("write_parameters", [](const PinholeCamera& self) {
            std::vector<double> params;
            self.writeParameters(params);
            return params;
        });
        // Methods requiring cv::Mat (initUndistortMap, etc.) are inherited but need casters


    // --- PinholeFull ---
    py::class_<PinholeFullCamera::Parameters, boost::shared_ptr<PinholeFullCamera::Parameters>, Camera::Parameters>(m, "PinholeFullCameraParameters")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("w"), py::arg("h"), py::arg("k1"), py::arg("k2"), py::arg("k3"), py::arg("k4"), py::arg("k5"), py::arg("k6"), py::arg("p1"), py::arg("p2"),
             py::arg("fx"), py::arg("fy"), py::arg("cx"), py::arg("cy"))
        .def_property("k1", [](const PinholeFullCamera::Parameters &p){ return p.k1(); }, [](PinholeFullCamera::Parameters &p, double v){ p.k1() = v; })
        .def_property("k2", [](const PinholeFullCamera::Parameters &p){ return p.k2(); }, [](PinholeFullCamera::Parameters &p, double v){ p.k2() = v; })
        .def_property("k3", [](const PinholeFullCamera::Parameters &p){ return p.k3(); }, [](PinholeFullCamera::Parameters &p, double v){ p.k3() = v; })
        .def_property("k4", [](const PinholeFullCamera::Parameters &p){ return p.k4(); }, [](PinholeFullCamera::Parameters &p, double v){ p.k4() = v; })
        .def_property("k5", [](const PinholeFullCamera::Parameters &p){ return p.k5(); }, [](PinholeFullCamera::Parameters &p, double v){ p.k5() = v; })
        .def_property("k6", [](const PinholeFullCamera::Parameters &p){ return p.k6(); }, [](PinholeFullCamera::Parameters &p, double v){ p.k6() = v; })
        .def_property("p1", [](const PinholeFullCamera::Parameters &p){ return p.p1(); }, [](PinholeFullCamera::Parameters &p, double v){ p.p1() = v; })
        .def_property("p2", [](const PinholeFullCamera::Parameters &p){ return p.p2(); }, [](PinholeFullCamera::Parameters &p, double v){ p.p2() = v; })
        .def_property("fx", [](const PinholeFullCamera::Parameters &p){ return p.fx(); }, [](PinholeFullCamera::Parameters &p, double v){ p.fx() = v; })
        .def_property("fy", [](const PinholeFullCamera::Parameters &p){ return p.fy(); }, [](PinholeFullCamera::Parameters &p, double v){ p.fy() = v; })
        .def_property("cx", [](const PinholeFullCamera::Parameters &p){ return p.cx(); }, [](PinholeFullCamera::Parameters &p, double v){ p.cx() = v; })
        .def_property("cy", [](const PinholeFullCamera::Parameters &p){ return p.cy(); }, [](PinholeFullCamera::Parameters &p, double v){ p.cy() = v; })
        .def("read_from_yaml_file", &PinholeFullCamera::Parameters::readFromYamlFile, py::arg("filename"))
        .def("write_to_yaml_file", &PinholeFullCamera::Parameters::writeToYamlFile, py::arg("filename"))
        .def("__repr__", [](const PinholeFullCamera::Parameters& p) { std::ostringstream oss; oss << p; return oss.str(); });

    py::class_<PinholeFullCamera, boost::shared_ptr<PinholeFullCamera>, Camera>(m, "PinholeFullCamera")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("image_width"), py::arg("image_height"), py::arg("k1"), py::arg("k2"), py::arg("k3"), py::arg("k4"), py::arg("k5"), py::arg("k6"), py::arg("p1"), py::arg("p2"), py::arg("fx"), py::arg("fy"), py::arg("cx"), py::arg("cy"))
        .def(py::init<const PinholeFullCamera::Parameters&>(), py::arg("params"))
        .def("get_parameters", [](const PinholeFullCamera& self) { return PinholeFullCamera::Parameters(self.getParameters()); }, py::return_value_policy::copy, "Get a copy of the camera parameters")
        .def("set_parameters", &PinholeFullCamera::setParameters, py::arg("parameters"))
        .def("distortion", [](const PinholeFullCamera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d d_u;
            self.distortion(p_u, d_u);
            return d_u;
        }, py::arg("p_u"), "Calculate distortion vector for a normalized point")
        .def("distortion_with_jacobian", [](const PinholeFullCamera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d d_u;
            Eigen::Matrix2d J;
            self.distortion(p_u, d_u, J);
            return std::make_tuple(d_u, J);
        }, py::arg("p_u"), "Calculate distortion vector and its Jacobian")
        .def("lift_projective_scaled", [](const PinholeFullCamera& self, const Eigen::Vector2d& p, float image_scale) {
            Eigen::Vector3d P;
            self.liftProjective(p, P, image_scale);
            return P;
        }, py::arg("p"), py::arg("image_scale"))
        .def("space_to_plane_scaled", [](const PinholeFullCamera& self, const Eigen::Vector3d& P, float image_scale) {
            Eigen::Vector2d p;
            self.spaceToPlane(P, p, image_scale);
            return p;
        }, py::arg("P"), py::arg("image_scale"))
        .def("read_parameters", &PinholeFullCamera::readParameters, py::arg("parameter_vec"))
        .def("write_parameters", [](const PinholeFullCamera& self) {
            std::vector<double> params;
            self.writeParameters(params);
            return params;
        })
        .def("get_principle", &PinholeFullCamera::getPrinciple, "Get principle point (cx, cy)")
        .def("space_to_plane_with_jacobian", [](const PinholeFullCamera& self, const Eigen::Vector3d& P) {
            Eigen::Vector2d p;
            Eigen::Matrix<double, 2, 3> J;
            self.spaceToPlane(P, p, J);
            return std::make_tuple(p, J);
        }, py::arg("P"), "Project 3D point to 2D plane and return Jacobian");


    // --- CataCamera (MEI) ---
    py::class_<CataCamera::Parameters, boost::shared_ptr<CataCamera::Parameters>, Camera::Parameters>(m, "CataCameraParameters")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("w"), py::arg("h"), py::arg("xi"), py::arg("k1"), py::arg("k2"), py::arg("p1"), py::arg("p2"),
             py::arg("gamma1"), py::arg("gamma2"), py::arg("u0"), py::arg("v0"))
        .def_property("xi", [](const CataCamera::Parameters &p){ return p.xi(); }, [](CataCamera::Parameters &p, double v){ p.xi() = v; })
        .def_property("k1", [](const CataCamera::Parameters &p){ return p.k1(); }, [](CataCamera::Parameters &p, double v){ p.k1() = v; })
        .def_property("k2", [](const CataCamera::Parameters &p){ return p.k2(); }, [](CataCamera::Parameters &p, double v){ p.k2() = v; })
        .def_property("p1", [](const CataCamera::Parameters &p){ return p.p1(); }, [](CataCamera::Parameters &p, double v){ p.p1() = v; })
        .def_property("p2", [](const CataCamera::Parameters &p){ return p.p2(); }, [](CataCamera::Parameters &p, double v){ p.p2() = v; })
        .def_property("gamma1", [](const CataCamera::Parameters &p){ return p.gamma1(); }, [](CataCamera::Parameters &p, double v){ p.gamma1() = v; })
        .def_property("gamma2", [](const CataCamera::Parameters &p){ return p.gamma2(); }, [](CataCamera::Parameters &p, double v){ p.gamma2() = v; })
        .def_property("u0", [](const CataCamera::Parameters &p){ return p.u0(); }, [](CataCamera::Parameters &p, double v){ p.u0() = v; })
        .def_property("v0", [](const CataCamera::Parameters &p){ return p.v0(); }, [](CataCamera::Parameters &p, double v){ p.v0() = v; })
        .def("read_from_yaml_file", &CataCamera::Parameters::readFromYamlFile, py::arg("filename"))
        .def("write_to_yaml_file", &CataCamera::Parameters::writeToYamlFile, py::arg("filename"))
        .def("__repr__", [](const CataCamera::Parameters& p) { std::ostringstream oss; oss << p; return oss.str(); });

    py::class_<CataCamera, boost::shared_ptr<CataCamera>, Camera>(m, "CataCamera")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("image_width"), py::arg("image_height"), py::arg("xi"), py::arg("k1"), py::arg("k2"), py::arg("p1"), py::arg("p2"),
             py::arg("gamma1"), py::arg("gamma2"), py::arg("u0"), py::arg("v0"))
        .def(py::init<const CataCamera::Parameters&>(), py::arg("params"))
        .def("get_parameters", [](const CataCamera& self) { return CataCamera::Parameters(self.getParameters()); }, py::return_value_policy::copy, "Get a copy of the camera parameters")
        .def("set_parameters", &CataCamera::setParameters, py::arg("parameters"))
        .def("distortion", [](const CataCamera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d d_u;
            self.distortion(p_u, d_u);
            return d_u;
        }, py::arg("p_u"), "Calculate distortion vector for a normalized point")
        .def("distortion_with_jacobian", [](const CataCamera& self, const Eigen::Vector2d& p_u) {
            Eigen::Vector2d d_u;
            Eigen::Matrix2d J;
            self.distortion(p_u, d_u, J);
            return std::make_tuple(d_u, J);
        }, py::arg("p_u"), "Calculate distortion vector and its Jacobian")
        .def("space_to_plane_with_jacobian", [](const CataCamera& self, const Eigen::Vector3d& P) {
            Eigen::Vector2d p;
            Eigen::Matrix<double, 2, 3> J;
            self.spaceToPlane(P, p, J);
            return std::make_tuple(p, J);
        }, py::arg("P"), "Project 3D point to 2D plane and return Jacobian")
        .def("read_parameters", &CataCamera::readParameters, py::arg("parameter_vec"))
        .def("write_parameters", [](const CataCamera& self) {
            std::vector<double> params;
            self.writeParameters(params);
            return params;
        });


    // --- Equidistant (Kannala-Brandt) ---
    py::class_<EquidistantCamera::Parameters, boost::shared_ptr<EquidistantCamera::Parameters>, Camera::Parameters>(m, "EquidistantCameraParameters")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("w"), py::arg("h"), py::arg("k2"), py::arg("k3"), py::arg("k4"), py::arg("k5"),
             py::arg("mu"), py::arg("mv"), py::arg("u0"), py::arg("v0"))
        .def_property("k2", [](const EquidistantCamera::Parameters &p){ return p.k2(); }, [](EquidistantCamera::Parameters &p, double v){ p.k2() = v; })
        .def_property("k3", [](const EquidistantCamera::Parameters &p){ return p.k3(); }, [](EquidistantCamera::Parameters &p, double v){ p.k3() = v; })
        .def_property("k4", [](const EquidistantCamera::Parameters &p){ return p.k4(); }, [](EquidistantCamera::Parameters &p, double v){ p.k4() = v; })
        .def_property("k5", [](const EquidistantCamera::Parameters &p){ return p.k5(); }, [](EquidistantCamera::Parameters &p, double v){ p.k5() = v; })
        .def_property("mu", [](const EquidistantCamera::Parameters &p){ return p.mu(); }, [](EquidistantCamera::Parameters &p, double v){ p.mu() = v; })
        .def_property("mv", [](const EquidistantCamera::Parameters &p){ return p.mv(); }, [](EquidistantCamera::Parameters &p, double v){ p.mv() = v; })
        .def_property("u0", [](const EquidistantCamera::Parameters &p){ return p.u0(); }, [](EquidistantCamera::Parameters &p, double v){ p.u0() = v; })
        .def_property("v0", [](const EquidistantCamera::Parameters &p){ return p.v0(); }, [](EquidistantCamera::Parameters &p, double v){ p.v0() = v; })
        .def("read_from_yaml_file", &EquidistantCamera::Parameters::readFromYamlFile, py::arg("filename"))
        .def("write_to_yaml_file", &EquidistantCamera::Parameters::writeToYamlFile, py::arg("filename"))
        .def("__repr__", [](const EquidistantCamera::Parameters& p) { std::ostringstream oss; oss << p; return oss.str(); });

    py::class_<EquidistantCamera, boost::shared_ptr<EquidistantCamera>, Camera>(m, "EquidistantCamera")
        .def(py::init<>())
        .def(py::init<const std::string&, int, int, double, double, double, double, double, double, double, double>(),
             py::arg("camera_name"), py::arg("image_width"), py::arg("image_height"), py::arg("k2"), py::arg("k3"), py::arg("k4"), py::arg("k5"), py::arg("mu"), py::arg("mv"), py::arg("u0"), py::arg("v0"))
        .def(py::init<const EquidistantCamera::Parameters&>(), py::arg("params"))
        .def("get_parameters", [](const EquidistantCamera& self) { return EquidistantCamera::Parameters(self.getParameters()); }, py::return_value_policy::copy, "Get a copy of the camera parameters")
        .def("set_parameters", &EquidistantCamera::setParameters, py::arg("parameters"))
        .def("space_to_plane_with_jacobian", [](const EquidistantCamera& self, const Eigen::Vector3d& P) {
            Eigen::Vector2d p;
            Eigen::Matrix<double, 2, 3> J;
            self.spaceToPlane(P, p, J);
            return std::make_tuple(p, J);
        }, py::arg("P"), "Project 3D point to 2D plane and return Jacobian")
        .def("read_parameters", &EquidistantCamera::readParameters, py::arg("parameter_vec"))
        .def("write_parameters", [](const EquidistantCamera& self) {
            std::vector<double> params;
            self.writeParameters(params);
            return params;
        });


    // --- Scaramuzza (OCAM) ---
     py::class_<OCAMCamera::Parameters, boost::shared_ptr<OCAMCamera::Parameters>, Camera::Parameters>(m, "OCAMCameraParameters")
        .def(py::init<>())
        // No constructor with all params?
        .def_property("C", [](const OCAMCamera::Parameters &p){ return p.C(); }, [](OCAMCamera::Parameters &p, double v){ p.C() = v; })
        .def_property("D", [](const OCAMCamera::Parameters &p){ return p.D(); }, [](OCAMCamera::Parameters &p, double v){ p.D() = v; })
        .def_property("E", [](const OCAMCamera::Parameters &p){ return p.E(); }, [](OCAMCamera::Parameters &p, double v){ p.E() = v; })
        .def_property("center_x", [](const OCAMCamera::Parameters &p){ return p.center_x(); }, [](OCAMCamera::Parameters &p, double v){ p.center_x() = v; })
        .def_property("center_y", [](const OCAMCamera::Parameters &p){ return p.center_y(); }, [](OCAMCamera::Parameters &p, double v){ p.center_y() = v; })
        // Expose poly and inv_poly as properties returning lists/tuples?
        .def_property_readonly("poly", [](const OCAMCamera::Parameters& p) {
            std::vector<double> poly_vec(SCARAMUZZA_POLY_SIZE);
            for(int i=0; i<SCARAMUZZA_POLY_SIZE; ++i) poly_vec[i] = p.poly(i);
            return poly_vec;
         })
        .def("set_poly", [](OCAMCamera::Parameters& p, int idx, double val) {
            if (idx >= 0 && idx < SCARAMUZZA_POLY_SIZE) p.poly(idx) = val;
            else throw py::index_error("poly index out of bounds");
         }, py::arg("idx"), py::arg("val"))
         .def_property_readonly("inv_poly", [](const OCAMCamera::Parameters& p) {
            std::vector<double> inv_poly_vec(SCARAMUZZA_INV_POLY_SIZE);
            for(int i=0; i<SCARAMUZZA_INV_POLY_SIZE; ++i) inv_poly_vec[i] = p.inv_poly(i);
            return inv_poly_vec;
         })
        .def("set_inv_poly", [](OCAMCamera::Parameters& p, int idx, double val) {
            if (idx >= 0 && idx < SCARAMUZZA_INV_POLY_SIZE) p.inv_poly(idx) = val;
            else throw py::index_error("inv_poly index out of bounds");
         }, py::arg("idx"), py::arg("val"))
        .def("read_from_yaml_file", &OCAMCamera::Parameters::readFromYamlFile, py::arg("filename"))
        .def("write_to_yaml_file", &OCAMCamera::Parameters::writeToYamlFile, py::arg("filename"))
        .def("__repr__", [](const OCAMCamera::Parameters& p) { std::ostringstream oss; oss << p; return oss.str(); });

    py::class_<OCAMCamera, boost::shared_ptr<OCAMCamera>, Camera>(m, "OCAMCamera")
        .def(py::init<>())
        .def(py::init<const OCAMCamera::Parameters&>(), py::arg("params"))
        .def("get_parameters", [](const OCAMCamera& self) { return OCAMCamera::Parameters(self.getParameters()); }, py::return_value_policy::copy, "Get a copy of the camera parameters")
        .def("set_parameters", &OCAMCamera::setParameters, py::arg("parameters"))
        .def_static("lift_to_sphere_static", [](const OCAMCamera::Parameters& params, const Eigen::Vector2d& p) {
            Eigen::Matrix<double, 3, 1> P;
            // Need to get the parameter vector for the static method
            std::vector<double> paramVec;
            OCAMCamera tempCam(params); // Create temporary instance to get param vector
            tempCam.writeParameters(paramVec);
            // The static method LiftToSphere expects a raw pointer
            OCAMCamera::LiftToSphere<double>(paramVec.data(), p.cast<double>(), P);
            return P;
        }, py::arg("params"), py::arg("p"), "Static: Lift image point to sphere using parameters")
        .def_static("sphere_to_plane_static", [](const OCAMCamera::Parameters& params, const Eigen::Vector3d& P) {
            Eigen::Matrix<double, 2, 1> p;
             std::vector<double> paramVec;
            OCAMCamera tempCam(params); // Create temporary instance to get param vector
            tempCam.writeParameters(paramVec);
            OCAMCamera::SphereToPlane<double>(paramVec.data(), P.cast<double>(), p); // Cast Eigen types explicitly if needed
            return p;
        }, py::arg("params"), py::arg("P"), "Static: Project sphere point to plane using parameters")
        .def("parameter_count", &OCAMCamera::parameterCount)
        .def("read_parameters", &OCAMCamera::readParameters, py::arg("parameter_vec"))
        .def("write_parameters", [](const OCAMCamera& self) {
            std::vector<double> params;
            self.writeParameters(params);
            return params;
        });


    // --- Camera Factory ---
    // Use reference_internal for instance() if it returns a raw pointer managed elsewhere
    // but boost::shared_ptr holder should handle shared ownership correctly.
    py::class_<CameraFactory, boost::shared_ptr<CameraFactory> >(m, "CameraFactory")
        .def_static("instance", &CameraFactory::instance, py::return_value_policy::reference, "Get the singleton instance of CameraFactory")
        .def("generate_camera_from_yaml_file", &CameraFactory::generateCameraFromYamlFile,
             py::arg("filename"), "Generate camera model from YAML file")
        .def("generate_camera", &CameraFactory::generateCamera,
             py::arg("model_type"), py::arg("camera_name"), py::arg("image_size"),
             "Generate camera model by type, name, and image size");

} // PYBIND11_MODULE