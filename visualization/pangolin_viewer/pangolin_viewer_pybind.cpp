#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>  // 包含这个头文件
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include "PangolinViewer.h"  // 确保此路径正确

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<Eigen::Vector3f>);
PYBIND11_MAKE_OPAQUE(std::vector<Eigen::Matrix4f>);
PYBIND11_MAKE_OPAQUE(std::map<size_t, Eigen::Vector3f>);
PYBIND11_MAKE_OPAQUE(std::map<size_t, std::vector<Eigen::Vector3f>>);


PYBIND11_MODULE(pangolin_viewer, m) {
    py::bind_vector<std::vector<Eigen::Vector3f>>(m, "VectorVector3f");
    py::bind_vector<std::vector<Eigen::Matrix4f>>(m, "VectorMatrix4f");
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
        
        // 新增：统一清除所有可视化元素的API绑定
        .def("clear_all_visual_elements", &PangolinViewer::clear_all_visual_elements)
        
        // 添加点云 - 基本版本（单一颜色）
        .def("add_points", [=](PangolinViewer& self, py::array_t<float>& points, 
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
        .def("add_points_with_color_name", [=](PangolinViewer& self, py::array_t<float>& points, 
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
        .def("add_points_with_colors", [=](PangolinViewer& self, py::array_t<float>& points, 
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
        
        // ===== 新增轨迹 API 绑定 =====
        .def("clear_all_trajectories", &PangolinViewer::clear_all_trajectories)

        .def("add_trajectory_se3", [=](PangolinViewer& self, 
                                   py::array_t<float, py::array::c_style | py::array::forcecast> poses_se3,
                                   py::array_t<float>& color,
                                   const std::string& label = "",
                                   float line_width = 1.0f,
                                   bool show_cameras = false,
                                   float camera_size = 0.05f) {
            // 检查输入 SE3 数组
            if (poses_se3.ndim() != 3 || poses_se3.shape(1) != 4 || poses_se3.shape(2) != 4) {
                throw std::runtime_error("Poses array must be of shape (N, 4, 4)");
            }
            // 检查颜色数组
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }

            // 转换 SE3 数组
            std::vector<Eigen::Matrix4f> poses_vec;
            auto r = poses_se3.unchecked<3>();
            for (ssize_t i = 0; i < r.shape(0); ++i) {
                Eigen::Matrix4f pose;
                for (ssize_t row = 0; row < 4; ++row) {
                    for (ssize_t col = 0; col < 4; ++col) {
                        pose(row, col) = r(i, row, col);
                    }
                }
                poses_vec.push_back(pose);
            }

            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));

            // 调用 C++ 方法
            self.add_trajectory_se3(poses_vec, color_vec, label, line_width, show_cameras, camera_size);
        }, py::arg("poses_se3"), py::arg("color"), py::arg("label") = "", py::arg("line_width") = 1.0f, 
           py::arg("show_cameras") = false, py::arg("camera_size") = 0.05f)

        .def("add_trajectory_quat", [=](PangolinViewer& self,
                                    py::array_t<float>& positions,
                                    py::array_t<float>& orientations,
                                    py::array_t<float>& color,
                                    const std::string& quat_format = "wxyz", // 'xyzw' or 'wxyz', 'JPL' or 'Hamilton'
                                    const std::string& label = "",
                                    float line_width = 1.0f,
                                    bool show_cameras = false,
                                    float camera_size = 0.05f) {
            // 检查输入数组
            if (positions.ndim() != 2 || positions.shape(1) != 3) {
                throw std::runtime_error("Positions array must be of shape (N, 3)");
            }
            if (orientations.ndim() != 2 || orientations.shape(1) != 4) {
                throw std::runtime_error("Orientations array must be of shape (N, 4)");
            }
            if (positions.shape(0) != orientations.shape(0)) {
                throw std::runtime_error("Number of positions and orientations must match");
            }
             // 检查颜色数组
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }

            // 转换位置数组
            std::vector<Eigen::Vector3f> positions_vec;
            auto pos_r = positions.unchecked<2>();
            for (ssize_t i = 0; i < pos_r.shape(0); ++i) {
                positions_vec.emplace_back(pos_r(i, 0), pos_r(i, 1), pos_r(i, 2));
            }

            // 转换方向数组 (处理格式)
            std::vector<Eigen::Quaternionf> orientations_vec;
            auto ori_r = orientations.unchecked<2>();
            bool is_wxyz = (quat_format.find("wxyz") != std::string::npos);
            bool is_jpl = (quat_format.find("JPL") != std::string::npos);

            for (ssize_t i = 0; i < ori_r.shape(0); ++i) {
                float qx = is_wxyz ? ori_r(i, 1) : ori_r(i, 0);
                float qy = is_wxyz ? ori_r(i, 2) : ori_r(i, 1);
                float qz = is_wxyz ? ori_r(i, 3) : ori_r(i, 2);
                float qw = is_wxyz ? ori_r(i, 0) : ori_r(i, 3);
                
                // Eigen::Quaternionf 构造函数是 (w, x, y, z)
                Eigen::Quaternionf q(qw, qx, qy, qz);
                
                // 如果是 JPL (ROS1/ROS2等), x,y,z需要取反以匹配Hamilton (Eigen/Pangolin)
                // 注意：这里假设输入已经是JPL或Hamilton之一，直接按存储顺序读
                // 如果还需要根据 'JPL' 标记来反转 x, y, z，则取消下面的注释
                // if (is_jpl) {
                //     q = Eigen::Quaternionf(qw, -qx, -qy, -qz);
                // }
                
                q.normalize();
                orientations_vec.push_back(q);
            }

            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));

            // 调用 C++ 方法
            self.add_trajectory_quat(positions_vec, orientations_vec, color_vec, label, line_width, show_cameras, camera_size);
        }, py::arg("positions"), py::arg("orientations"), py::arg("color"), 
           py::arg("quat_format") = "wxyz", py::arg("label") = "", py::arg("line_width") = 1.0f, 
           py::arg("show_cameras") = false, py::arg("camera_size") = 0.05f)
        
        // ===== 新增独立相机 API 绑定 =====
        .def("clear_all_cameras", &PangolinViewer::clear_all_cameras)
        .def("set_main_camera", &PangolinViewer::set_main_camera, py::arg("camera_id"))

        .def("add_camera_se3", [=](PangolinViewer& self,
                                 py::array_t<float, py::array::c_style | py::array::forcecast> pose_se3,
                                 py::array_t<float>& color,
                                 const std::string& label = "",
                                 float scale = 0.1f,
                                 float line_width = 1.0f) -> size_t {
            // 检查输入 SE3 数组
            if (pose_se3.ndim() != 2 || pose_se3.shape(0) != 4 || pose_se3.shape(1) != 4) {
                 throw std::runtime_error("SE3 pose must be a 4x4 matrix");
            }
             // 检查颜色数组
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }

            // 转换 SE3 数组
            Eigen::Matrix4f pose_mat;
            auto r = pose_se3.unchecked<2>();
            for (ssize_t row = 0; row < 4; ++row) {
                for (ssize_t col = 0; col < 4; ++col) {
                    pose_mat(row, col) = r(row, col);
                }
            }
            
            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));

            // 调用 C++ 方法
            return self.add_camera_se3(pose_mat, color_vec, label, scale, line_width);
        }, py::arg("pose_se3"), py::arg("color"), py::arg("label") = "", py::arg("scale") = 0.1f, py::arg("line_width") = 1.0f)

        .def("add_camera_quat", [=](PangolinViewer& self,
                                  py::array_t<float>& position,
                                  py::array_t<float>& orientation,
                                  py::array_t<float>& color,
                                  const std::string& quat_format = "wxyz",
                                  const std::string& label = "",
                                  float scale = 0.1f,
                                  float line_width = 1.0f) -> size_t {
            // 检查输入数组
            if (position.ndim() != 1 || position.shape(0) != 3) {
                throw std::runtime_error("Position array must be of shape (3,)");
            }
            if (orientation.ndim() != 1 || orientation.shape(0) != 4) {
                throw std::runtime_error("Orientation array must be of shape (4,)");
            }
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }

            // 转换位置数组
            auto pos_r = position.unchecked<1>();
            Eigen::Vector3f position_vec(pos_r(0), pos_r(1), pos_r(2));

            // 转换方向数组 (处理格式)
            auto ori_r = orientation.unchecked<1>();
            bool is_wxyz = (quat_format.find("wxyz") != std::string::npos);
            // bool is_jpl = (quat_format.find("JPL") != std::string::npos); // 如果需要区分JPL/Hamilton

            float qx = is_wxyz ? ori_r(1) : ori_r(0);
            float qy = is_wxyz ? ori_r(2) : ori_r(1);
            float qz = is_wxyz ? ori_r(3) : ori_r(2);
            float qw = is_wxyz ? ori_r(0) : ori_r(3);
            
            Eigen::Quaternionf orientation_q(qw, qx, qy, qz);
            // if (is_jpl) { orientation_q = Eigen::Quaternionf(qw, -qx, -qy, -qz); }
            orientation_q.normalize();

            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));

            // 调用 C++ 方法
            return self.add_camera_quat(position_vec, orientation_q, color_vec, label, scale, line_width);
        }, py::arg("position"), py::arg("orientation"), py::arg("color"), 
           py::arg("quat_format") = "wxyz", py::arg("label") = "", py::arg("scale") = 0.1f, py::arg("line_width") = 1.0f)

        // ===== 新增平面 API 绑定 =====
        .def("clear_all_planes", &PangolinViewer::clear_all_planes)
        .def("add_plane", [=](PangolinViewer& self, 
                            py::array_t<float>& vertices, 
                            py::array_t<float>& color, 
                            float alpha=0.5f, 
                            const std::string& label="") {
            // 检查顶点数组
            if (vertices.ndim() != 2 || vertices.shape(1) != 3) {
                throw std::runtime_error("Vertices array must be of shape (N, 3) with N >= 3");
            }
            if (vertices.shape(0) < 3) {
                 throw std::runtime_error("Plane needs at least 3 vertices");
            }
            // 检查颜色数组
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }

            // 转换顶点
            std::vector<Eigen::Vector3f> vertices_vec;
            auto r = vertices.unchecked<2>();
            for (ssize_t i = 0; i < r.shape(0); ++i) {
                vertices_vec.emplace_back(r(i, 0), r(i, 1), r(i, 2));
            }
            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));
            
            self.add_plane(vertices_vec, color_vec, alpha, label);
        }, py::arg("vertices"), py::arg("color"), py::arg("alpha") = 0.5f, py::arg("label") = "")
        
        // 新增：绑定基于法线、中心、尺寸的add_plane
        .def("add_plane_normal_center", [=](PangolinViewer& self,
                                        py::array_t<float>& normal,
                                        py::array_t<float>& center,
                                        float size,
                                        py::array_t<float>& color,
                                        float alpha = 0.5f,
                                        const std::string& label = "") {
            // 检查输入数组形状
            if (normal.ndim() != 1 || normal.shape(0) != 3) {
                throw std::runtime_error("Normal array must be of shape (3,)");
            }
            if (center.ndim() != 1 || center.shape(0) != 3) {
                throw std::runtime_error("Center array must be of shape (3,)");
            }
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }
            
            // 转换向量
            auto n = normal.unchecked<1>();
            Eigen::Vector3f normal_vec(n(0), n(1), n(2));
            auto cen = center.unchecked<1>();
            Eigen::Vector3f center_vec(cen(0), cen(1), cen(2));
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));
            
            // 调用C++函数（重载版本）
            self.add_plane(normal_vec, center_vec, size, color_vec, alpha, label);
        }, py::arg("normal"), py::arg("center"), py::arg("size"), py::arg("color"), py::arg("alpha")=0.5f, py::arg("label")="")
        
        // ===== 新增直线 API 绑定 =====
        .def("clear_all_lines", &PangolinViewer::clear_all_lines)
        .def("add_line", [=](PangolinViewer& self,
                           py::array_t<float>& start_point,
                           py::array_t<float>& end_point,
                           py::array_t<float>& color,
                           float line_width = 1.0f,
                           const std::string& label = "") {
            // 检查点数组
            if (start_point.ndim() != 1 || start_point.shape(0) != 3) {
                throw std::runtime_error("Start point array must be of shape (3,)");
            }
            if (end_point.ndim() != 1 || end_point.shape(0) != 3) {
                throw std::runtime_error("End point array must be of shape (3,)");
            }
            // 检查颜色数组
            if (color.ndim() != 1 || color.shape(0) != 3) {
                throw std::runtime_error("Color array must be of shape (3,)");
            }
            
            // 转换点
            auto s = start_point.unchecked<1>();
            Eigen::Vector3f start_vec(s(0), s(1), s(2));
            auto e = end_point.unchecked<1>();
            Eigen::Vector3f end_vec(e(0), e(1), e(2));
            // 转换颜色
            auto c = color.unchecked<1>();
            Eigen::Vector3f color_vec(c(0), c(1), c(2));
            
            self.add_line(start_vec, end_vec, color_vec, line_width, label);
        }, py::arg("start_point"), py::arg("end_point"), py::arg("color"), py::arg("line_width") = 1.0f, py::arg("label") = "")

        // ===== 修改后的图像 API 绑定 =====
        .def("add_image_1", [=](PangolinViewer &self, py::array_t<unsigned char> &img) {
            py::buffer_info buf = img.request();
            cv::Mat mat(static_cast<int>(buf.shape[0]), static_cast<int>(buf.shape[1]), CV_8UC3, buf.ptr);
            self.add_image_1(mat);
        }, py::arg("img")) // Bind the cv::Mat version
        .def("add_image_1", static_cast<void (PangolinViewer::*)(const std::string&)>(&PangolinViewer::add_image_1), 
             py::arg("image_path")) // Bind the string path version
             
        .def("add_image_2", [=](PangolinViewer &self, py::array_t<unsigned char> &img) {
            py::buffer_info buf = img.request();
            cv::Mat mat(static_cast<int>(buf.shape[0]), static_cast<int>(buf.shape[1]), CV_8UC3, buf.ptr);
            self.add_image_2(mat);
        }, py::arg("img")) // Bind the cv::Mat version
        .def("add_image_2", static_cast<void (PangolinViewer::*)(const std::string&)>(&PangolinViewer::add_image_2), 
             py::arg("image_path")) // Bind the string path version

		.def("is_step_mode_active", &PangolinViewer::is_step_mode_active)
		.def("set_visualize_opencv_mat", &PangolinViewer::set_visualize_opencv_mat)
		.def("wait_for_step", &PangolinViewer::wait_for_step)
		.def("notify_algorithm", &PangolinViewer::notify_algorithm)
		;
}