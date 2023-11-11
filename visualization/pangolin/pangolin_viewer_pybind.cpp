#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>  // 包含这个头文件
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include "PangolinViewer.h"  // 确保此路径正确

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<Eigen::Vector3f>);
PYBIND11_MAKE_OPAQUE(std::map<size_t, Eigen::Vector3f>);
PYBIND11_MAKE_OPAQUE(std::map<size_t, std::vector<Eigen::Vector3f>>);


PYBIND11_MODULE(pangolin_viewer, m) {
    py::bind_vector<std::vector<Eigen::Vector3f>>(m, "VectorVector3f");
    py::bind_map<std::map<size_t, Eigen::Vector3f>>(m, "MapSizeTVector3f");
    py::bind_map<std::map<size_t, std::vector<Eigen::Vector3f>>>(m, "MapSizeTVectorVector3f");

    py::class_<PangolinViewer>(m, "PangolinViewer")
        .def(py::init<int, int, bool>())
        .def("run", &PangolinViewer::run)
        .def("close", &PangolinViewer::close)
        .def("join", &PangolinViewer::join)
        .def("reset", &PangolinViewer::reset)
		.def("view_init", &PangolinViewer::extern_init)
		.def("should_not_quit", &PangolinViewer::extern_should_not_quit)
		.def("show", &PangolinViewer::extern_run_single_step, py::arg("delay_time_in_s"))
        .def("set_img_resolution", &PangolinViewer::set_img_resolution, py::arg("width"), py::arg("height"))
        // .def("publish_traj", &PangolinViewer::publish_traj, py::arg("q_wc"), py::arg("t_wc"))
        .def("publish_traj", [](PangolinViewer& self, py::array_t<float>& t_wc, py::array_t<float>& q_wc) {
			assert(t_wc.ndim() == 1 && t_wc.shape(0) == 3);
			assert(q_wc.ndim() == 1 && q_wc.shape(0) == 4);
			py::buffer_info t_wc_info = t_wc.request();
			Eigen::Vector3f t_wc_eigen(static_cast<float*>(t_wc_info.ptr));
			py::buffer_info q_wc_info = q_wc.request();
			Eigen::Quaternionf q_wc_eigen(static_cast<float*>(q_wc_info.ptr));
			self.publish_traj(q_wc_eigen, t_wc_eigen);
		}, py::arg("t_wc"), py::arg("q_wc"))
        .def("publish_3D_points", static_cast<void (PangolinViewer::*)(std::vector<Eigen::Vector3f>&, std::vector<Eigen::Vector3f>&)>(&PangolinViewer::publish_3D_points), py::arg("slam_pts"), py::arg("msckf_pts"))
        .def("publish_3D_points", static_cast<void (PangolinViewer::*)(std::map<size_t, Eigen::Vector3f>&, std::vector<Eigen::Vector3f>&)>(&PangolinViewer::publish_3D_points), py::arg("slam_pts"), py::arg("msckf_pts"))
		.def("publish_3D_points", [](PangolinViewer& self, py::array_t<float>& slam_pts, py::array_t<float>& msckf_pts) {
			// Convert the first NumPy array to std::vector<Eigen::Vector3f>
			std::vector<Eigen::Vector3f> vec_slam_pts;
			auto r1 = slam_pts.unchecked<2>(); // Use unchecked for raw access
			for (ssize_t i = 0; i < r1.shape(0); ++i) {
				vec_slam_pts.emplace_back(r1(i, 0), r1(i, 1), r1(i, 2));
			}
			
			// Convert the second NumPy array to std::vector<Eigen::Vector3f>
			std::vector<Eigen::Vector3f> vec_msckf_pts;
			auto r2 = msckf_pts.unchecked<2>(); // Use unchecked for raw access
			for (ssize_t i = 0; i < r2.shape(0); ++i) {
				vec_msckf_pts.emplace_back(r2(i, 0), r2(i, 1), r2(i, 2));
			}
			
			// Call the method with the converted vectors
			self.publish_3D_points(vec_slam_pts, vec_msckf_pts);
		})
        .def("publish_track_img", [](PangolinViewer &self, py::array_t<unsigned char> &img) {
            py::buffer_info buf = img.request();
            cv::Mat mat(static_cast<int>(buf.shape[0]), static_cast<int>(buf.shape[1]), CV_8UC3, buf.ptr);
            self.publish_track_img(mat);
        })
        .def("publish_vio_opt_data", &PangolinViewer::publish_vio_opt_data, py::arg("vals"))
        .def("publish_plane_detection_img", [](PangolinViewer &self, py::array_t<unsigned char> &img) {
            py::buffer_info buf = img.request();
            cv::Mat mat(static_cast<int>(buf.shape[0]), static_cast<int>(buf.shape[1]), CV_8UC3, buf.ptr);
            self.publish_plane_detection_img(mat);
        })
		.def("publish_plane_triangulate_pts", [](PangolinViewer &self, const std::map<size_t, py::EigenDRef<Eigen::Vector3f>>& pts) {
			std::map<size_t, Eigen::Vector3f> plane_tri_pts;
			for (const auto& item : pts) {
				plane_tri_pts[item.first] = item.second;
			}
			self.publish_plane_triangulate_pts(plane_tri_pts);
		}, py::arg("plane_tri_pts"))
		.def("publish_plane_vio_stable_pts", [](PangolinViewer &self, const std::map<size_t, py::EigenDRef<Eigen::Vector3f>>& pts) {
			std::map<size_t, Eigen::Vector3f> plane_vio_stable_pts;
			for (const auto& item : pts) {
				plane_vio_stable_pts[item.first] = item.second;
			}
			self.publish_plane_vio_stable_pts(plane_vio_stable_pts);
		}, py::arg("plane_vio_stable_pts"))
		.def("publish_planes_horizontal", [](PangolinViewer &self, const std::map<size_t, std::vector<py::EigenDRef<Eigen::Vector3f>>>& planes) {
			std::map<size_t, std::vector<Eigen::Vector3f>> his_planes_horizontal;
			for (const auto& item : planes) {
				for (const auto& vec : item.second) {
					his_planes_horizontal[item.first].push_back(vec);
				}
			}
			self.publish_planes_horizontal(his_planes_horizontal);
		}, py::arg("planes"))
		.def("publish_planes_vertical", [](PangolinViewer &self, const std::map<size_t, std::vector<py::EigenDRef<Eigen::Vector3f>>>& planes) {
			std::map<size_t, std::vector<Eigen::Vector3f>> his_planes_vertical;
			for (const auto& item : planes) {
				for (const auto& vec : item.second) {
					his_planes_vertical[item.first].push_back(vec);
				}
			}
			self.publish_planes_vertical(his_planes_vertical);
		}, py::arg("planes"))
		.def("publish_traj_gt", &PangolinViewer::publish_traj_gt, py::arg("q_wc_gt"), py::arg("t_wc_gt"))
		.def("get_algorithm_wait_flag", &PangolinViewer::get_algorithm_wait_flag)
		.def("set_visualize_opencv_mat", &PangolinViewer::set_visualize_opencv_mat)
		.def("algorithm_wait", &PangolinViewer::algorithm_wait)
		.def("notify_algorithm", &PangolinViewer::notify_algorithm)
		;
}