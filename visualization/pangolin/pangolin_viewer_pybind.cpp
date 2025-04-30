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
        
        // 新的点云可视化API
        .def("clear_all_points", &PangolinViewer::clear_all_points)
        
        // 添加点云 - 基本版本（单一颜色）
        .def("add_points", [](PangolinViewer& self, py::array_t<float>& points, 
                             py::array_t<float>& color, const std::string& label="", float point_size=4.0f) {
            // 检查输入点数组
            if (points.ndim() != 2 || points.shape(1) != 3) {
                throw std::runtime_error("Points array must be of shape (N, 3)");
            }
            
            // 检查颜色数组
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }
            
            // 转换点数组
            std::vector<Eigen::Vector3f> points_vec;
            auto r = points.unchecked<2>();
            for (ssize_t i = 0; i < r.shape(0); ++i) {
                points_vec.emplace_back(r(i, 0), r(i, 1), r(i, 2));
            }
            
            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));
            
            // 调用C++方法
            self.add_points(points_vec, color_vec, label, point_size);
        }, py::arg("points"), py::arg("color"), py::arg("label") = "", py::arg("point_size") = 4.0f)
        
        // 添加点云 - 颜色名称版本
        .def("add_points_with_color_name", [](PangolinViewer& self, py::array_t<float>& points, 
                                            const std::string& color_name="red", const std::string& label="", 
                                            float point_size=4.0f) {
            // 检查输入点数组
            if (points.ndim() != 2 || points.shape(1) != 3) {
                throw std::runtime_error("Points array must be of shape (N, 3)");
            }
            
            // 转换点数组
            std::vector<Eigen::Vector3f> points_vec;
            auto r = points.unchecked<2>();
            for (ssize_t i = 0; i < r.shape(0); ++i) {
                points_vec.emplace_back(r(i, 0), r(i, 1), r(i, 2));
            }
            
            // 调用C++方法
            self.add_points_with_color_name(points_vec, color_name, label, point_size);
        }, py::arg("points"), py::arg("color_name") = "red", py::arg("label") = "", py::arg("point_size") = 4.0f)
        
        // 添加点云 - 多颜色版本（改名为add_points_with_colors避免Python无法区分重载）
        .def("add_points_with_colors", [](PangolinViewer& self, py::array_t<float>& points, 
                             py::array_t<float>& colors, const std::string& label="", float point_size=4.0f) {
            // 检查输入点数组
            if (points.ndim() != 2 || points.shape(1) != 3) {
                throw std::runtime_error("Points array must be of shape (N, 3)");
            }
            
            // 检查颜色数组
            if (colors.ndim() != 2 || colors.shape(1) != 3) {
                throw std::runtime_error("Colors array must be of shape (N, 3)");
            }
            
            // 转换点数组
            std::vector<Eigen::Vector3f> points_vec;
            auto r = points.unchecked<2>();
            for (ssize_t i = 0; i < r.shape(0); ++i) {
                points_vec.emplace_back(r(i, 0), r(i, 1), r(i, 2));
            }
            
            // 转换颜色数组
            std::vector<Eigen::Vector3f> colors_vec;
            auto c = colors.unchecked<2>();
            for (ssize_t i = 0; i < c.shape(0); ++i) {
                colors_vec.emplace_back(c(i, 0), c(i, 1), c(i, 2));
            }
            
            // 调用C++方法
            self.add_points(points_vec, colors_vec, label, point_size);
        }, py::arg("points"), py::arg("colors"), py::arg("label") = "", py::arg("point_size") = 4.0f)
        
        // 原有API (保持兼容性)
        .def("publish_traj", [](PangolinViewer& self, py::array_t<float> t_wc, py::array_t<float> q_wc) {
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