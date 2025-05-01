#ifndef PANGOLIN_VIEWER_H
#define PANGOLIN_VIEWER_H

#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <optional>
#include <map>
#include <vector>
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <pangolin/pangolin.h>

class PangolinViewer {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  PangolinViewer(int w, int h, bool start_run_thread = true);
  virtual ~PangolinViewer();

  void run();
  void close();
  void join();
  void reset();
  void extern_init();
  void extern_run_single_step(float delay_time_in_s = 0.0);
  bool extern_should_not_quit();

  // set the resolution for track img
  void set_img_resolution(int width, int height) {
    track_img_width = width;
    track_img_height = height;
  }

  // 通用点云结构体
  struct Point3D {
    Eigen::Vector3f position;
    Eigen::Vector3f color;  // RGB颜色，范围0.0-1.0
    std::string label;

    Point3D(const Eigen::Vector3f& pos, const Eigen::Vector3f& col = Eigen::Vector3f(1.0f, 0.0f, 0.0f), 
           const std::string& lbl = "") : position(pos), color(col), label(lbl) {}
  };

  // 点云集合
  struct PointCloud {
    std::string name;
    std::vector<Point3D> points;
    float point_size = 4.0f;  // 点大小
    
    PointCloud(const std::string& n = "default") : name(n) {}
  };

  // ===== 新增轨迹结构体 =====
  struct TrajectoryPose {
      Eigen::Vector3f position;
      Eigen::Quaternionf orientation;

      TrajectoryPose(const Eigen::Vector3f& p, const Eigen::Quaternionf& q) 
          : position(p), orientation(q) {}
  };

  struct Trajectory {
      std::string name;
      std::vector<TrajectoryPose> poses;
      Eigen::Vector3f color = Eigen::Vector3f(0.0f, 1.0f, 0.0f); // 默认绿色
      float line_width = 1.0f;
      float camera_size = 0.05f; // 0表示不绘制相机模型
      bool show_cameras = false; // 是否绘制相机模型

      Trajectory(const std::string& n = "default_traj") : name(n) {}
  };

  // ===== 新增独立相机结构体 =====
  struct CameraInstance {
      size_t id;
      Eigen::Vector3f position;
      Eigen::Quaternionf orientation;
      Eigen::Vector3f color = Eigen::Vector3f(1.0f, 1.0f, 0.0f); // 默认黄色
      float scale = 0.1f; // 相机模型大小
      float line_width = 1.0f;
      std::string label;

      // 构造函数等可以按需添加
  };

  // ===== 新增轨迹API =====
  // 在每次渲染循环开始时清除所有额外添加的轨迹
  void clear_all_trajectories();

  // 添加轨迹段（位姿用四元数表示）
  void add_trajectory_quat(const std::vector<Eigen::Vector3f>& positions,
                           const std::vector<Eigen::Quaternionf>& orientations,
                           const Eigen::Vector3f& color = Eigen::Vector3f(0.0f, 1.0f, 0.0f),
                           const std::string& label = "",
                           float line_width = 1.0f,
                           bool show_cameras = false,
                           float camera_size = 0.05f);

  // 添加轨迹段（位姿用SE3矩阵表示）
  void add_trajectory_se3(const std::vector<Eigen::Matrix4f>& poses_se3,
                         const Eigen::Vector3f& color = Eigen::Vector3f(0.0f, 1.0f, 0.0f),
                         const std::string& label = "",
                         float line_width = 1.0f,
                         bool show_cameras = false,
                         float camera_size = 0.05f);

  // ===== 新增独立相机API =====
  // 清除所有独立相机
  void clear_all_cameras();

  // 添加独立相机（位姿用四元数表示），返回相机ID
  size_t add_camera_quat(const Eigen::Vector3f& position,
                         const Eigen::Quaternionf& orientation,
                         const Eigen::Vector3f& color = Eigen::Vector3f(1.0f, 1.0f, 0.0f),
                         const std::string& label = "",
                         float scale = 0.1f,
                         float line_width = 1.0f);
                         
  // 添加独立相机（位姿用SE3矩阵表示），返回相机ID
  size_t add_camera_se3(const Eigen::Matrix4f& pose_se3,
                       const Eigen::Vector3f& color = Eigen::Vector3f(1.0f, 1.0f, 0.0f),
                       const std::string& label = "",
                       float scale = 0.1f,
                       float line_width = 1.0f);
                       
  // 设置主相机，用于视图跟随
  void set_main_camera(size_t camera_id);

  // 在每次渲染循环开始时清除所有点云
  void clear_all_points();
  
  // 添加单帧要显示的点云
  void add_points(const std::vector<Eigen::Vector3f>& points, 
                const Eigen::Vector3f& color = Eigen::Vector3f(1.0f, 0.0f, 0.0f),
                const std::string& label = "",
                float point_size = 4.0f);
  
  // 添加单帧要显示的点云，带单独点颜色
  void add_points(const std::vector<Eigen::Vector3f>& points, 
                const std::vector<Eigen::Vector3f>& colors,
                const std::string& label = "",
                float point_size = 4.0f);
  
  // 添加单帧要显示的点云，使用颜色名称
  void add_points_with_color_name(const std::vector<Eigen::Vector3f>& points, 
                                const std::string& color_name = "red",
                                const std::string& label = "",
                                float point_size = 4.0f,
                                float line_width = 1.0f);
                         
  // ===== 修改后的图像API =====
  // 添加图像到第一个视图 (可传入cv::Mat)
  void add_image_1(const cv::Mat &img);
  // 添加图像到第一个视图 (可传入文件路径)
  void add_image_1(const std::string& image_path);
  
  // 添加图像到第二个视图 (可传入cv::Mat)
  void add_image_2(const cv::Mat &img);
  // 添加图像到第二个视图 (可传入文件路径)
  void add_image_2(const std::string& image_path);

  bool get_algorithm_wait_flag() const { return algorithm_wait_flag; }
  void set_visualize_opencv_mat() { visualize_opencv_mat = true; }

  void algorithm_wait() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk);
  }

  void notify_algorithm() { cv.notify_one(); }

private:
  bool need_reset;
  void reset_internal();
  void set_algorithm_wait_flag(bool flag) { algorithm_wait_flag = flag; }

  // 从颜色名称解析为RGB值
  Eigen::Vector3f parse_color_name(const std::string& color_name);
  // SE3矩阵转四元数+位移
  void se3_to_quat_trans(const Eigen::Matrix4f& se3, Eigen::Quaternionf& q, Eigen::Vector3f& t);

  std::thread run_thread;
  bool running;
  int w, h;
  int track_img_width, track_img_height;

  bool algorithm_wait_flag;
  bool visualize_opencv_mat;

  std::mutex m;
  std::condition_variable cv;

private:
  // 单帧点云数据 - 每次渲染循环都会更新
  std::vector<PointCloud> frame_point_clouds;
  std::mutex mutex_point_clouds;
  
  // 单帧轨迹数据 - 每次渲染循环都会更新
  std::vector<Trajectory> frame_trajectories;
  std::mutex mutex_trajectories;
  
  // 单帧独立相机数据 - 每次渲染循环都会更新
  std::map<size_t, CameraInstance> frame_cameras;
  std::mutex mutex_cameras;
  size_t next_camera_id = 0;
  std::optional<size_t> main_camera_id = std::nullopt;
  
  // 预定义颜色映射
  std::unordered_map<std::string, Eigen::Vector3f> color_map;

  // data we need to draw
  Eigen::Vector3f cur_t_wc;
  Eigen::Quaternionf cur_r_wc;
  std::vector<Eigen::Vector3f> vio_traj;

  pangolin::DataLog vio_dt_data_log, vio_extrin_t_data_log, vio_vel_data_log,
      vio_bg_data_log, vio_ba_data_log;
  double start_t, cur_t;

  cv::Mat track_img;
  bool track_img_changed;
  std::mutex mutex_img;
  std::mutex mutex_3D_show;

  bool plane_detection_img_changed;
  cv::Mat plane_detection_img;
  std::mutex mutex_plane_img;

  // render settings
  bool b_show_trajectory = true;
  bool b_show_3D_points = true;
  bool b_show_cameras = true;
  bool b_follow_camera = true;
  bool b_camera_view = true;
  bool b_show_est_bg = true;
  bool b_show_est_ba = true;
  bool b_show_est_dt = false;
  bool b_show_est_vel = false;
  bool b_show_est_extrin_t = false;

  struct RuntimeInfo;
  std::shared_ptr<RuntimeInfo> mRuntimeInfo;

  // helper function
  void draw_current_camera(const Eigen::Vector3f &p, const Eigen::Quaternionf &q,
                           Eigen::Vector4f color = Eigen::Vector4f(0.0f, 1.0f, 1.0f, 1.0f),
                           float cam_size = 0.2f);

  void follow_camera(const Eigen::Vector3f &p, const Eigen::Quaternionf &q);

  // 旧的绘制方法
  void draw_3D_points(const std::vector<Eigen::Vector3f> &points,
                      Eigen::Vector3f color, float pt_size);
  
  // 新的绘制方法
  void draw_3D_points(const PointCloud &point_cloud, float pt_size);
  void draw_all_point_clouds(float pt_size = 4.0f);
  
  void draw_history_3D_points(Eigen::Vector3f color, float pt_size);
  void draw_trajectory(const std::vector<Eigen::Vector3f> &traj,
                       Eigen::Vector3f color = Eigen::Vector3f(1.0f, 0.0f,
                                                               0.0f));
  
  // ===== 新增轨迹绘制函数 =====
  void draw_trajectory_line(const Trajectory& trajectory);
  void draw_trajectory_cameras(const Trajectory& trajectory);
  void draw_all_trajectories(); // 在渲染循环中调用

  void draw_trajectory_gt(const std::vector<Eigen::Vector3f> &traj,
                          Eigen::Vector3f color = Eigen::Vector3f(1.0f, 0.0f, 0.0f));

  // ===== 新增独立相机绘制函数 =====
  void draw_all_cameras(); // 在渲染循环中调用

private:
  // 内部辅助函数
  cv::Mat resize_and_pad_image(const cv::Mat& img_in, int view_w, int view_h);
};

#endif // PANGOLIN_VIEWER_H
