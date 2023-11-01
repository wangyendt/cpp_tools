#ifndef FFVINS_VIS_PANGOLIN_VIEWER_H
#define FFVINS_VIS_PANGOLIN_VIEWER_H

#include <memory>
#include <map>
#include <deque>
#include <condition_variable>
#include <thread>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <pangolin/pangolin.h>

class PangolinViewer {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    PangolinViewer(int w, int h, bool start_run_thread=true);
    virtual ~PangolinViewer();

    void run();
    void close();
    void join();
    void reset();

    // set the resolution for track img 
    void set_img_resolution(int width, int height) {
        track_img_width = width;
        track_img_height = height;
    }

    // send the visulize info we need 
    void publish_traj(Eigen::Quaternionf& q_wc, Eigen::Vector3f& t_wc);
    void publish_3D_points(std::vector<Eigen::Vector3f>& slam_pts, std::vector<Eigen::Vector3f>& msckf_pts);
    void publish_3D_points(std::map<size_t, Eigen::Vector3f>& slam_pts, std::vector<Eigen::Vector3f>& msckf_pts);
    void publish_track_img(cv::Mat& track_img);
    // the order is timestamp, timesoffset, extrin_trans, velocity, bg, ba 
    //              (0), (1), (2, 3, 4), (5, 6, 7), (8, 9, 10), (11, 12 ,13)
    void publish_vio_opt_data(std::vector<float> vals);
    void publish_plane_detection_img(cv::Mat& plane_img);
    void publish_plane_triangulate_pts(std::map<size_t, Eigen::Vector3f>& plane_tri_pts);
    void publish_plane_vio_stable_pts(std::map<size_t, Eigen::Vector3f>& plane_tri_pts);
    void publish_planes_horizontal(std::map<size_t, std::vector<Eigen::Vector3f>>& planes);
    void publish_planes_vertical(std::map<size_t, std::vector<Eigen::Vector3f>>& planes);
    void publish_traj_gt(Eigen::Quaternionf& q_wc, Eigen::Vector3f& t_wc);

    bool get_algorithm_wait_flag() const { return algorithm_wait_flag; }
    void set_visualize_opencv_mat() { visualize_opencv_mat = true; }

    void algorithm_wait() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk);    
    }

    void notify_algorithm() {
        cv.notify_one();
    }
private:
    bool need_reset;
    void reset_internal();
    void set_algorithm_wait_flag(bool flag) { algorithm_wait_flag = flag; }
    
    std::thread run_thread;
    bool running;
    int w,h;
    int track_img_width, track_img_height;

    bool algorithm_wait_flag;
    bool visualize_opencv_mat;

    std::mutex m;
    std::condition_variable cv;
private:
    // data we need to draw 
    Eigen::Vector3f cur_t_wc;
    Eigen::Quaternionf cur_r_wc;
    std::vector<Eigen::Vector3f> vio_traj;
    
    std::vector<Eigen::Vector3f> cur_slam_pts;
    std::vector<Eigen::Vector3f> cur_msckf_pts;
    std::map<size_t, Eigen::Vector3f> his_slam_pts;
    std::map<size_t, Eigen::Vector3f> his_plane_tri_pts;
    std::map<size_t, Eigen::Vector3f> his_plane_vio_stable_pts;
    std::map<size_t, std::vector<Eigen::Vector3f>> his_planes_horizontal;
    std::map<size_t, std::vector<std::vector<Eigen::Vector3f>>> his_planes_vertical;
    Eigen::Vector3f cur_t_wc_gt;
    Eigen::Quaternionf cur_r_wc_gt;
    std::vector<Eigen::Vector3f> traj_gt;

    pangolin::DataLog vio_dt_data_log, vio_extrin_t_data_log, vio_vel_data_log, vio_bg_data_log, vio_ba_data_log;
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
    bool b_show_history_points = true;
    bool b_follow_camera = true;
    bool b_camera_view = true;
    bool b_show_plane_tri_points = true;
    bool b_show_plane_vio_stable_points = true;
    bool b_show_plane = true;

    bool b_show_est_bg = true;
    bool b_show_est_ba = true;
    bool b_show_est_dt = false;
    bool b_show_est_vel = false;
    bool b_show_est_extrin_t = false;

    // helper function 
    void draw_current_camera(
                        const Eigen::Vector3f &p,
                        const Eigen::Quaternionf &q,
                        Eigen::Vector3f color = Eigen::Vector3f(0.0f, 0.0f, 1.0f),
                        float cam_size = 0.2f); 

    void follow_camera(const Eigen::Vector3f &p, const Eigen::Quaternionf &q);
    
    void draw_3D_points(const std::vector<Eigen::Vector3f> &points, Eigen::Vector3f color, float pt_size);
    void draw_history_3D_points(Eigen::Vector3f color, float pt_size);
    void draw_trajectory(const std::vector<Eigen::Vector3f> &traj, Eigen::Vector3f color = Eigen::Vector3f(1.0f, 0.0f, 0.0f));
    void draw_plane_history_tri_points(Eigen::Vector3f color, float pt_size);
    void draw_plane_history_vio_stable_points(Eigen::Vector3f color, float pt_size);
    void draw_history_plane_horizontal();
    void draw_history_plane_vertical();
    void draw_trajectory_gt(const std::vector<Eigen::Vector3f> &traj, Eigen::Vector3f color = Eigen::Vector3f(1.0f, 0.0f, 0.0f));
};

#endif 
