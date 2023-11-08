#include <unistd.h>
#include "PangolinViewer.h"

struct PangolinViewer::RuntimeInfo {
    pangolin::GlTexture tex_track;
    pangolin::GlTexture tex_plane_detection;
    pangolin::View* track_result;
    pangolin::View* plane_detection_result;
    pangolin::Plotter* plotter_extrin_t;
    pangolin::Plotter* plotter_dt;
    pangolin::Plotter* plotter_vel;
    pangolin::Plotter* plotter_bg;
    pangolin::Plotter* plotter_ba;
    pangolin::View* Visualization3D_display;
	pangolin::OpenGlRenderState* Visualization3D_camera;


    pangolin::Var<bool>* pb_follow_camera { new pangolin::Var<bool>("ui.Follow Camera", false, true) };
    pangolin::Var<bool>* pb_camera_view { new pangolin::Var<bool>("ui.Camera View", false, false) };

    pangolin::Var<bool>* pb_show_traj { new pangolin::Var<bool>("ui.Show Trajectory", true, true) };
    pangolin::Var<bool>* pb_show_points { new pangolin::Var<bool>("ui.Show 3D Points", true, true) };
    pangolin::Var<bool>* pb_show_history_points { new pangolin::Var<bool>("ui.Show History 3D", true, true) };
    pangolin::Var<bool>* pb_show_history_plane_tri_points { new pangolin::Var<bool>("ui.Show Plane tri 3D", true, true) };
    pangolin::Var<bool>* pb_show_history_plane_vio_stable_points { new pangolin::Var<bool>("ui.Show Plane vio 3D", true, true) };
    pangolin::Var<bool>* pb_show_plane { new pangolin::Var<bool>("ui.Show Plane", true, true) };

    pangolin::Var<bool>* pb_show_est_bg { new pangolin::Var<bool>("ui.show_est_bg", true, true) };
    pangolin::Var<bool>* pb_show_est_ba { new pangolin::Var<bool>("ui.show_est_ba", true, true) };
    pangolin::Var<bool>* pb_show_est_timeoffset { new pangolin::Var<bool>("ui.show_est_dt", false, true) };
    pangolin::Var<bool>* pb_show_est_vel { new pangolin::Var<bool>("ui.show_est_vel",false, true) };
    pangolin::Var<bool>* pb_show_est_extrin_trans { new pangolin::Var<bool>("ui.show_est_ex_t", false, true) };

    pangolin::Var<bool>* pb_step_by_step { new pangolin::Var<bool>("ui.Step by Step", false, false) };
    pangolin::Var<bool>* pb_start_algorithm { new pangolin::Var<bool>("ui.Start Button", false, false) };
};

PangolinViewer::PangolinViewer(int w, int h, bool start_run_thread) 
{
    this->w = w;
	this->h = h;
	running = true;
    need_reset = false;

    algorithm_wait_flag = false;
    visualize_opencv_mat = false;

    cur_t_wc.setZero();
    cur_r_wc.setIdentity();

    cur_t_wc_gt.setZero();
    cur_r_wc_gt.setIdentity();

    vio_traj.clear();
    cur_slam_pts.clear();
    cur_msckf_pts.clear();
    his_slam_pts.clear();
    his_plane_tri_pts.clear();

    vio_dt_data_log.Clear();
    vio_extrin_t_data_log.Clear();
    vio_vel_data_log.Clear();
    vio_bg_data_log.Clear();
    vio_ba_data_log.Clear();

    mRuntimeInfo = std::make_shared<RuntimeInfo>();

    track_img_changed = false;
    //track_img = cv::Mat::zeros(track_img_width, track_img_height, CV_8UC3);

    plane_detection_img_changed = false; 
    //plane_detection_img = cv::Mat::zeros(track_img_width, track_img_height, CV_8UC3);

    if(start_run_thread) {
        run_thread = std::thread(&PangolinViewer::run, this);
    }

    // extern_init();
}

PangolinViewer::~PangolinViewer()
{
    pangolin::QuitAll();
}

void PangolinViewer::close()
{
	running = false;
}

void PangolinViewer::join()
{
    close();
	run_thread.join();
	printf("JOINED Pangolin thread!\n");
}

void PangolinViewer::reset()
{
	need_reset = true;
}

void PangolinViewer::reset_internal()
{
    cur_t_wc.setZero();
    cur_r_wc.setIdentity();

    vio_traj.clear();
    cur_slam_pts.clear();
    cur_msckf_pts.clear();
    his_slam_pts.clear();
    his_plane_tri_pts.clear();

    vio_dt_data_log.Clear();
    vio_extrin_t_data_log.Clear();
    vio_vel_data_log.Clear();
    vio_bg_data_log.Clear();
    vio_ba_data_log.Clear();

    track_img_changed = false;
    track_img = cv::Mat::zeros(track_img_width, track_img_height, CV_8UC3);

    plane_detection_img_changed = false; 
    plane_detection_img = cv::Mat::zeros(track_img_width, track_img_height, CV_8UC3);

    need_reset = false;

    algorithm_wait_flag = false;
    visualize_opencv_mat = false;
}

void PangolinViewer::run()
{
    extern_init();
    while (extern_should_not_quit())
    {
        extern_run_single_step();
    }
    printf("Finished pangolin run\n");
}

void PangolinViewer::extern_init() {
	printf("START PANGOLIN FOR VIEWING!\n");
    pangolin::CreateWindowAndBind("Main", 4 * w, 3 * h);
	const int UI_WIDTH = 180;
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mRuntimeInfo->Visualization3D_camera = new pangolin::OpenGlRenderState(
		pangolin::ProjectionMatrix(w, h, 500, 500, w/2, h/2, 0.1, 1000),
		pangolin::ModelViewLookAt(0.0, -0.7, -1.8, 0, 0, 0, 0.0, -1.0, 0.0)
		);

	mRuntimeInfo->Visualization3D_display = &pangolin::CreateDisplay()
		.SetBounds(0.0, 0.5, 0.3, 1.0, -w/(float)h)
		.SetHandler(new pangolin::Handler3D(*mRuntimeInfo->Visualization3D_camera));

	mRuntimeInfo->track_result = &pangolin::Display("img_track_video")
	    .SetAspect(track_img_width / (float)track_img_height);
    mRuntimeInfo->tex_track.Reinitialise(track_img_width, track_img_height, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);

    pangolin::CreateDisplay()
		  .SetBounds(0.0, 0.3, pangolin::Attach::Pix(UI_WIDTH), 0.3)
		  .SetLayout(pangolin::LayoutEqual)
		  .AddDisplay(*mRuntimeInfo->track_result);

    mRuntimeInfo->plane_detection_result = &pangolin::Display("img_plane_detection_video")
	    .SetAspect(track_img_width / (float)track_img_height);
    mRuntimeInfo->tex_plane_detection.Reinitialise(track_img_width, track_img_height, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);

    pangolin::CreateDisplay()
		  .SetBounds(0.3, 0.6, pangolin::Attach::Pix(UI_WIDTH), 0.3)
		  .SetLayout(pangolin::LayoutEqual)
		  .AddDisplay(*mRuntimeInfo->plane_detection_result);

    // for dt plot 
    pangolin::View& plot_dt_display = pangolin::CreateDisplay().SetBounds(0.95, 1.0, 0.4, 1.0);
    mRuntimeInfo->plotter_dt = new pangolin::Plotter(&vio_dt_data_log, 0.0, 300, -0.1, 0.1, 0.01f, 0.01f);
    mRuntimeInfo->plotter_dt->ClearSeries();
    mRuntimeInfo->plotter_dt->ClearMarkers();
    plot_dt_display.AddDisplay(*mRuntimeInfo->plotter_dt);

    // for extrinsic translation plot 
    pangolin::View& plot_extrin_t_display = pangolin::CreateDisplay().SetBounds(0.9, 0.95, 0.4, 1.0);
    mRuntimeInfo->plotter_extrin_t = new pangolin::Plotter(&vio_extrin_t_data_log, 0.0, 300, -0.01, 0.01, 0.01f, 0.01f);
    mRuntimeInfo->plotter_extrin_t->ClearSeries();
    mRuntimeInfo->plotter_extrin_t->ClearMarkers();
    plot_extrin_t_display.AddDisplay(*mRuntimeInfo->plotter_extrin_t);

    // for velocity 
    pangolin::View& plot_vel_display = pangolin::CreateDisplay().SetBounds(0.85, 0.9, 0.4, 1.0);
    mRuntimeInfo->plotter_vel = new pangolin::Plotter(&vio_vel_data_log, 0.0, 300, -2, 2, 0.01f, 0.01f);
    mRuntimeInfo->plotter_vel->ClearSeries();
    mRuntimeInfo->plotter_vel->ClearMarkers();
    plot_vel_display.AddDisplay(*mRuntimeInfo->plotter_vel);

    // for bg 
    pangolin::View& plot_bg_display = pangolin::CreateDisplay().SetBounds(0.80, 0.85, 0.4, 1.0);
    mRuntimeInfo->plotter_bg = new pangolin::Plotter(&vio_bg_data_log, 0.0, 300, -0.05, 0.05, 0.01f, 0.01f);
    plot_bg_display.AddDisplay(*mRuntimeInfo->plotter_bg);

    // for ba 
    pangolin::View& plot_ba_display = pangolin::CreateDisplay().SetBounds(0.75, 0.80, 0.4, 1.0);
    mRuntimeInfo->plotter_ba = new pangolin::Plotter(&vio_bg_data_log, 0.0, 300, -0.1, 0.1, 0.01f, 0.01f);
    plot_ba_display.AddDisplay(*mRuntimeInfo->plotter_ba);

    pangolin::CreatePanel("ui").SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(UI_WIDTH));
}

void PangolinViewer::extern_run_single_step(float delay_time_in_s) {
    // Clear entire screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::unique_lock<std::mutex> lk3d(mutex_3D_show);

    if(pangolin::Pushed(*mRuntimeInfo->pb_step_by_step)) {
        notify_algorithm();
        set_algorithm_wait_flag(true);
    }

    if(pangolin::Pushed(*mRuntimeInfo->pb_start_algorithm)) {
        notify_algorithm();
        set_algorithm_wait_flag(false);
    }

    Eigen::Matrix4f Twc = Eigen::Matrix4f::Identity();
    Twc.block<3, 3>(0, 0) = cur_r_wc.toRotationMatrix();
    Twc.block<3, 1>(0, 3) = cur_t_wc;

    pangolin::OpenGlMatrix TwcGL;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            TwcGL.m[i*4+j] = Twc(j, i);
        }
    }
    pangolin::OpenGlMatrix OwGL;
    OwGL.SetIdentity(); // Initialize to identity matrix
    OwGL.m[12] = cur_t_wc.x(); // px
    OwGL.m[13] = cur_t_wc.y(); // py
    OwGL.m[14] = cur_t_wc.z(); // pz

    if (*mRuntimeInfo->pb_follow_camera && b_follow_camera) {
        if (b_camera_view) {
            mRuntimeInfo->Visualization3D_camera->Follow(TwcGL);
        } else {
            mRuntimeInfo->Visualization3D_camera->Follow(OwGL);
        }
    } else if (*mRuntimeInfo->pb_follow_camera && !b_follow_camera) {
        if (b_camera_view) {
            mRuntimeInfo->Visualization3D_camera->SetProjectionMatrix(
                pangolin::ProjectionMatrix(w, h, 500, 500, w/2, h/2, 0.1, 1000)
            );
            mRuntimeInfo->Visualization3D_camera->SetModelViewMatrix(
                pangolin::ModelViewLookAt(0, -0.7, -1.8, 0, 0, 0, 0.0, -1.0, 0.0)
            );
            mRuntimeInfo->Visualization3D_camera->Follow(TwcGL);
        } else {
            mRuntimeInfo->Visualization3D_camera->SetProjectionMatrix(
                pangolin::ProjectionMatrix(w, h, 3000, 3000, w/2, h/2, 0.1, 1000)
            );
            mRuntimeInfo->Visualization3D_camera->SetModelViewMatrix(
                pangolin::ModelViewLookAt(0, 0.01, 10, 0, 0, 0, 0.0, 0.0, 1.0)
            );
            mRuntimeInfo->Visualization3D_camera->Follow(OwGL);
        }
        b_follow_camera = true;
    } else if (!*mRuntimeInfo->pb_follow_camera && b_follow_camera) {
        b_follow_camera = false;
    }
    if (*mRuntimeInfo->pb_camera_view) {
        *mRuntimeInfo->pb_camera_view = false;
        b_camera_view = true;
        mRuntimeInfo->Visualization3D_camera->SetProjectionMatrix(
            pangolin::ProjectionMatrix(w, h, 500,500, w/2, h/2, 0.1, 10000)
        );
        mRuntimeInfo->Visualization3D_camera->SetModelViewMatrix(
            pangolin::ModelViewLookAt(0,-0.7,-1.8, 0,0,0, 0.0, -1.0, 0.0)
        );
        mRuntimeInfo->Visualization3D_camera->Follow(TwcGL);
    }
    // follow_camera(cur_t_wc, cur_r_wc);


    mRuntimeInfo->Visualization3D_display->Activate(*mRuntimeInfo->Visualization3D_camera);
    glClearColor(1.0f,1.0f,1.0f,1.0f);

    if(b_show_trajectory) {
        draw_trajectory(vio_traj, Eigen::Vector3f(0.0f, 1.0f, 0.0f));
        draw_trajectory_gt(traj_gt, Eigen::Vector3f(1.0f, 0.0f, 0.0f));
        draw_current_camera(cur_t_wc, cur_r_wc);
        draw_current_camera(cur_t_wc_gt, cur_r_wc_gt);
    }
    if(b_show_3D_points) {
        draw_3D_points(cur_slam_pts, Eigen::Vector3f(1.0f, 0.0f, 0.0f), 8);
        draw_3D_points(cur_msckf_pts, Eigen::Vector3f(0.0f, 0.0f, 1.0f), 8);
    }
    if(b_show_history_points) {
        draw_history_3D_points(Eigen::Vector3f(0.5f, 0.5f, 0.5f), 4);
    }
    if(b_show_plane_tri_points) {
        draw_plane_history_tri_points(Eigen::Vector3f(1.0f, 0.5f, 0.0f), 4);
    }
    if(b_show_plane_vio_stable_points) {
        draw_plane_history_vio_stable_points(Eigen::Vector3f(0.0f, 0.0f, 1.0f), 4);
    }
    if(b_show_plane) {
        draw_history_plane_horizontal();
        draw_history_plane_vertical();
    }

    lk3d.unlock();

    mutex_img.lock();
    if(track_img_changed) mRuntimeInfo->tex_track.Upload(track_img.data, GL_BGR, GL_UNSIGNED_BYTE);
    mutex_img.unlock();

    {   
        mRuntimeInfo->track_result->Activate();
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        mRuntimeInfo->tex_track.RenderToViewportFlipY();
        track_img_changed = false;
    }

    mutex_plane_img.lock();
    if(plane_detection_img_changed) mRuntimeInfo->tex_plane_detection.Upload(plane_detection_img.data, GL_BGR, GL_UNSIGNED_BYTE);
    mutex_plane_img.unlock();

    {   
        mRuntimeInfo->plane_detection_result->Activate();
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        mRuntimeInfo->tex_plane_detection.RenderToViewportFlipY();
        plane_detection_img_changed = false;
    }
    
    {
        this->b_follow_camera = (*mRuntimeInfo->pb_follow_camera).Get();
        this->b_show_3D_points = (*mRuntimeInfo->pb_show_points).Get();
        this->b_show_trajectory = (*mRuntimeInfo->pb_show_traj).Get();
        this->b_show_history_points = (*mRuntimeInfo->pb_show_history_points).Get();
        this->b_show_plane_tri_points = (*mRuntimeInfo->pb_show_history_plane_tri_points).Get();
        this->b_show_plane_vio_stable_points = (*mRuntimeInfo->pb_show_history_plane_vio_stable_points).Get();
        this->b_show_plane = (*mRuntimeInfo->pb_show_plane).Get();

        this->b_show_est_bg = (*mRuntimeInfo->pb_show_est_bg).Get();
        this->b_show_est_ba = (*mRuntimeInfo->pb_show_est_ba).Get();
        this->b_show_est_dt = (*mRuntimeInfo->pb_show_est_timeoffset).Get();
        this->b_show_est_vel = (*mRuntimeInfo->pb_show_est_vel).Get();
        this->b_show_est_extrin_t = (*mRuntimeInfo->pb_show_est_extrin_trans).Get();
    }

    if(b_show_est_dt) {
        mRuntimeInfo->plotter_dt->ClearSeries();
        mRuntimeInfo->plotter_dt->ClearMarkers();
        mRuntimeInfo->plotter_dt->AddSeries("$0", "$1", pangolin::DrawingModeLine, pangolin::Colour::Red(), "timeoffset", &vio_dt_data_log);
        mRuntimeInfo->plotter_dt->AddMarker(pangolin::Marker::Vertical, cur_t, pangolin::Marker::Equal, pangolin::Colour::White());
    }

    if(b_show_est_extrin_t) {
        mRuntimeInfo->plotter_extrin_t->ClearSeries();
        mRuntimeInfo->plotter_extrin_t->ClearMarkers();
        mRuntimeInfo->plotter_extrin_t->AddSeries("$0", "$1", pangolin::DrawingModeLine, pangolin::Colour::Red(), "extrin x", &vio_extrin_t_data_log);
        mRuntimeInfo->plotter_extrin_t->AddSeries("$0", "$2", pangolin::DrawingModeLine, pangolin::Colour::Green(), "extrin y", &vio_extrin_t_data_log);
        mRuntimeInfo->plotter_extrin_t->AddSeries("$0", "$3", pangolin::DrawingModeLine, pangolin::Colour::Blue(), "extrin z", &vio_extrin_t_data_log);
        mRuntimeInfo->plotter_extrin_t->AddMarker(pangolin::Marker::Vertical, cur_t, pangolin::Marker::Equal, pangolin::Colour::White());
    }

    if(b_show_est_vel) {
        mRuntimeInfo->plotter_vel->ClearSeries();
        mRuntimeInfo->plotter_vel->ClearMarkers();
        mRuntimeInfo->plotter_vel->AddSeries("$0", "$1", pangolin::DrawingModeLine, pangolin::Colour::Red(), "vel x", &vio_vel_data_log);
        mRuntimeInfo->plotter_vel->AddSeries("$0", "$2", pangolin::DrawingModeLine, pangolin::Colour::Green(), "vel y", &vio_vel_data_log);
        mRuntimeInfo->plotter_vel->AddSeries("$0", "$3", pangolin::DrawingModeLine, pangolin::Colour::Blue(), "vel z", &vio_vel_data_log);
        mRuntimeInfo->plotter_vel->AddMarker(pangolin::Marker::Vertical, cur_t, pangolin::Marker::Equal, pangolin::Colour::White());
    }

    if(b_show_est_bg) {
        mRuntimeInfo->plotter_bg->ClearSeries();
        mRuntimeInfo->plotter_bg->ClearMarkers();
        mRuntimeInfo->plotter_bg->AddSeries("$0", "$1", pangolin::DrawingModeLine, pangolin::Colour::Red(), "gyro bias x", &vio_bg_data_log);
        mRuntimeInfo->plotter_bg->AddSeries("$0", "$2", pangolin::DrawingModeLine, pangolin::Colour::Green(), "gyro bias y", &vio_bg_data_log);
        mRuntimeInfo->plotter_bg->AddSeries("$0", "$3", pangolin::DrawingModeLine, pangolin::Colour::Blue(), "gyro bias z", &vio_bg_data_log);
        mRuntimeInfo->plotter_bg->AddMarker(pangolin::Marker::Vertical, cur_t, pangolin::Marker::Equal, pangolin::Colour::White());
    }

    if(b_show_est_ba) {
        mRuntimeInfo->plotter_ba->ClearSeries();
        mRuntimeInfo->plotter_ba->ClearMarkers();
        mRuntimeInfo->plotter_ba->AddSeries("$0", "$1", pangolin::DrawingModeLine, pangolin::Colour::Red(), "accel bias x", &vio_ba_data_log);
        mRuntimeInfo->plotter_ba->AddSeries("$0", "$2", pangolin::DrawingModeLine, pangolin::Colour::Green(), "accel bias y", &vio_ba_data_log);
        mRuntimeInfo->plotter_ba->AddSeries("$0", "$3", pangolin::DrawingModeLine, pangolin::Colour::Blue(), "accel bias z", &vio_ba_data_log);
        mRuntimeInfo->plotter_ba->AddMarker(pangolin::Marker::Vertical, cur_t, pangolin::Marker::Equal, pangolin::Colour::White());
    }

    //usleep(5000);
    // control step and step debug, stupid implementation 
    // should improved with keyboard response thread
    // cv::Mat response_img = cv::Mat::zeros(100, 100, CV_8UC1);
    // cv::imshow("keyboard response", response_img);
    // if(plane_detection_img.empty()) {
    //     plane_detection_img = cv::Mat::zeros(track_img.rows, track_img.cols, CV_8UC1);
    // }
    // cv::imshow("plane_detection_img", plane_detection_img);
    // char c = cv::waitKey(2);
    // if(c == 'w' || c == 'W') {
    //     set_algorithm_wait_flag(false);
    //     notify_algorithm();
    // }
    // else if( c == 'q' || c == 'Q') {
    //     set_algorithm_wait_flag(true);
    //     notify_algorithm();
    // }
        
    // Swap frames and Process Events
    pangolin::FinishFrame();
    if(need_reset) reset_internal();
}

bool PangolinViewer::extern_should_not_quit() {
    return !pangolin::ShouldQuit() && running;
}

// main publish function =============================
void PangolinViewer::publish_traj(Eigen::Quaternionf& q_wc, Eigen::Vector3f& t_wc)
{
    if(!b_show_trajectory) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);
    cur_t_wc = t_wc;
    cur_r_wc = q_wc;
    vio_traj.push_back(t_wc);
    return;
}

void PangolinViewer::publish_3D_points(std::vector<Eigen::Vector3f>& slam_pts, std::vector<Eigen::Vector3f>& msckf_pts)
{
    if(!b_show_3D_points) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);
    cur_slam_pts = slam_pts;
    cur_msckf_pts = msckf_pts;
    return;
}

void PangolinViewer::publish_3D_points(std::map<size_t, Eigen::Vector3f>& slam_pts, std::vector<Eigen::Vector3f>& msckf_pts) 
{
    if(!b_show_3D_points) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);
    cur_msckf_pts = msckf_pts;

    cur_slam_pts.clear();
    for(auto& elem : slam_pts) {
        cur_slam_pts.push_back(elem.second);
        size_t feat_id = elem.first;
        if(his_slam_pts.find(feat_id) != his_slam_pts.end()) {
            continue;
        } else {
            his_slam_pts[feat_id] = elem.second;
        }
    }
    return;
}

void PangolinViewer::publish_track_img(cv::Mat& track_img_)
{
    std::unique_lock<std::mutex> lk(mutex_img);
    track_img = track_img_;
    track_img_changed = true;
    return;
}

void PangolinViewer::publish_plane_detection_img(cv::Mat& plane_img) {
    std::unique_lock<std::mutex> lk(mutex_plane_img);
    plane_detection_img = plane_img;
    plane_detection_img_changed = true;
    return;
}

void PangolinViewer::publish_plane_triangulate_pts(std::map<size_t, Eigen::Vector3f>& plane_tri_pts) {
    if(!b_show_plane_tri_points) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);

    for(auto& pair : plane_tri_pts) {
        his_plane_tri_pts[pair.first] = pair.second;
    }
    return;
}

void PangolinViewer::publish_plane_vio_stable_pts(std::map<size_t, Eigen::Vector3f>& plane_vio_pts) {
    if(!b_show_plane_vio_stable_points) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);
    for(auto& pair: plane_vio_pts) {
        his_plane_vio_stable_pts[pair.first] = pair.second;
    }
    return;
}

void PangolinViewer::publish_planes_horizontal(std::map<size_t, std::vector<Eigen::Vector3f>>& planes) {
    if(!b_show_plane) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);

    for(auto& pair : planes) {
       // his_planes_horizontal[pair.first].emplace_back(pair.second);
       his_planes_horizontal[pair.first] = pair.second;
    }
    return;
}

void PangolinViewer::publish_planes_vertical(std::map<size_t, std::vector<Eigen::Vector3f>>& planes) {
    if(!b_show_plane) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);

    for(auto& pair : planes) {
        his_planes_vertical[pair.first].emplace_back(pair.second);
    }
    return;
}

// the order is timestamp, timesoffset, extrin_trans, velocity, bg, ba 
//              (0), (1), (2, 3, 4), (5, 6, 7), (8, 9, 10), (11, 12 ,13)
void PangolinViewer::publish_vio_opt_data(std::vector<float> vals)
{
    // vio_opt_data_log.Log(vals);
    cur_t = vals.at(0);
    std::vector<float> dt_vals;
    dt_vals.push_back(cur_t); 
    dt_vals.push_back(vals.at(1));
    vio_dt_data_log.Log(dt_vals);

    std::vector<float> extrin_t_vals;
    extrin_t_vals.push_back(cur_t); 
    extrin_t_vals.push_back(vals.at(2));
    extrin_t_vals.push_back(vals.at(3));
    extrin_t_vals.push_back(vals.at(4));
    vio_extrin_t_data_log.Log(extrin_t_vals);

    std::vector<float> vel_vals;
    vel_vals.push_back(cur_t); 
    vel_vals.push_back(vals.at(5));
    vel_vals.push_back(vals.at(6));
    vel_vals.push_back(vals.at(7));
    vio_vel_data_log.Log(vel_vals);

    std::vector<float> bg_vals;
    bg_vals.push_back(cur_t); 
    bg_vals.push_back(vals.at(8));
    bg_vals.push_back(vals.at(9));
    bg_vals.push_back(vals.at(10));
    vio_bg_data_log.Log(bg_vals);

    std::vector<float> ba_vals;
    ba_vals.push_back(cur_t); 
    ba_vals.push_back(vals.at(11));
    ba_vals.push_back(vals.at(12));
    ba_vals.push_back(vals.at(13));
    vio_ba_data_log.Log(ba_vals);

    return;
}

// some helper function ===============================
void PangolinViewer::draw_current_camera(
        const Eigen::Vector3f &p,
        const Eigen::Quaternionf &q,
        Eigen::Vector3f color,
        float cam_size)
{
    Eigen::Vector3f center = p;

    const float length = cam_size;
    Eigen::Vector3f m_cam[5] = {
        Eigen::Vector3f(0.0f, 0.0f, 0.0f),
        Eigen::Vector3f(-length, -length, length),
        Eigen::Vector3f(-length, length, length),
        Eigen::Vector3f(length, length, length),
        Eigen::Vector3f(length, -length, length)
    };

    for (int i = 0; i < 5; ++i)
        m_cam[i] = q * m_cam[i] + center;
    // [0;0;0], [X;Y;Z], [X;-Y;Z], [-X;Y;Z], [-X;-Y;Z]
    glColor3fv(&color(0));
    glBegin(GL_LINE_LOOP);
    glVertex3fv(m_cam[0].data());
    glVertex3fv(m_cam[1].data());
    glVertex3fv(m_cam[4].data());
    glVertex3fv(m_cam[3].data());
    glVertex3fv(m_cam[2].data());
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(m_cam[0].data());
    glVertex3fv(m_cam[3].data());
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(m_cam[0].data());
    glVertex3fv(m_cam[4].data());
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(m_cam[1].data());
    glVertex3fv(m_cam[2].data());
    glEnd();
}


void PangolinViewer::draw_3D_points(
        const std::vector<Eigen::Vector3f> &points, 
        Eigen::Vector3f color, float pt_size) 
{
    glColor3f(color.x(), color.y(), color.z());
    glPointSize(pt_size);
    glBegin(GL_POINTS);

    for (uint i = 0; i < points.size(); ++i) {
        const Eigen::Vector3f p = points[i];
        glVertex3f(p.x(), p.y(), p.z());
    }
    glEnd();
    return;
}

void PangolinViewer::draw_history_3D_points(Eigen::Vector3f color, float pt_size)
{
    std::vector<Eigen::Vector3f> all_history_points;
    for(auto& elem : his_slam_pts) {
        all_history_points.push_back(elem.second);
    }
    draw_3D_points(all_history_points, color, pt_size);
    return;
}

void PangolinViewer::draw_trajectory(
    const std::vector<Eigen::Vector3f> &traj, Eigen::Vector3f color) 
{
    glBegin(GL_LINE_STRIP);
    glColor3fv(color.data());
    for (const auto &p : traj) {
        glVertex3fv(&p[0]);
    }
    glEnd();
}

void PangolinViewer::follow_camera(const Eigen::Vector3f &p, const Eigen::Quaternionf &q) 
{
    pangolin::OpenGlMatrix M;
    Eigen::Vector3f twc = p;
    Eigen::Matrix3f Rwc = q.toRotationMatrix();

    M.m[0] = Rwc(0, 0);
    M.m[1] = Rwc(1, 0);
    M.m[2] = Rwc(2, 0);
    M.m[3] = 0.0;

    M.m[4] = Rwc(0, 1);
    M.m[5] = Rwc(1, 1);
    M.m[6] = Rwc(2, 1);
    M.m[7] = 0.0;

    M.m[8] = Rwc(0, 2);
    M.m[9] = Rwc(1, 2);
    M.m[10] = Rwc(2, 2);
    M.m[11] = 0.0;

    M.m[12] = twc(0);
    M.m[13] = twc(1);
    M.m[14] = twc(2);
    M.m[15] = 1.0;
}

void PangolinViewer::draw_plane_history_tri_points(Eigen::Vector3f color, float pt_size) {
    std::vector<Eigen::Vector3f> all_history_plane_tri_points;
    for(auto& elem : his_plane_tri_pts) {
        all_history_plane_tri_points.push_back(elem.second);
    }
    draw_3D_points(all_history_plane_tri_points, color, pt_size);
    return;
}

void PangolinViewer::draw_plane_history_vio_stable_points(Eigen::Vector3f color, float pt_size) {
    std::vector<Eigen::Vector3f> all_history_plane_vio_stable_points;
    for(auto& elem : his_plane_vio_stable_pts) {
        all_history_plane_vio_stable_points.push_back(elem.second);
    }
    draw_3D_points(all_history_plane_vio_stable_points, color, pt_size);
    return;
}

void PangolinViewer::draw_history_plane_horizontal() {
    
    std::vector<Eigen::Vector3f> corner_pts;
    
    for(auto& plane : his_planes_horizontal) {
        size_t plane_id = plane.first;
        auto& history_polygan = plane.second;

        // float b = ((plane_id * 10) % 255 ) / (float)(255.0f);
        // float g = ((plane_id * 100) % 255 ) / (float)(255.0f);
        // float r = ((plane_id * 1) % 255 ) / (float)(255.0f);

        float b = 0.75f;  //0
        float g = 0.75f;  //1
        float r = 0.75f;  //0
        //float alpha = 0.2f; 

        //for(auto& one_plane : history_polygan) {
            corner_pts = history_polygan;
            size_t num_pts = corner_pts.size();
            std::vector<float> color, points;
            color.resize(3 * num_pts);
            points.resize(3 * num_pts);
            for(size_t i = 0; i < num_pts; i++) {
                color.at(i*3) = b;
                color.at(i*3 + 1) = g;
                color.at(i*3 + 2) = r;
                //color.at(i*3 + 3) = alpha;

                points.at(i*3) = corner_pts.at(i)(0);
                points.at(i*3 + 1) = corner_pts.at(i)(1);
                points.at(i*3 + 2) = corner_pts.at(i)(2);
            }
            pangolin::glDrawColoredVertices<float,float>(num_pts, &points[0], &color[0], GL_POLYGON, 3, 3);
        //}
    }

    return;
}

void PangolinViewer::draw_history_plane_vertical() {
    
    std::vector<Eigen::Vector3f> corner_pts;
    
    for(auto& plane : his_planes_vertical) {
        size_t plane_id = plane.first;
        auto& history_polygan = plane.second;

        // float b = ((plane_id * 10) % 255 ) / (float)(255.0f);
        // float g = ((plane_id * 5) % 255 ) / (float)(255.0f);
        // float r = ((plane_id * 15) % 255 ) / (float)(255.0f);

        float b = 1.0f;
        float g = 0.0f;
        float r = 0.0f;

        for(auto& one_plane : history_polygan) {
            corner_pts = one_plane;
            size_t num_pts = corner_pts.size();
            std::vector<float> color, points;
            color.resize(3 * num_pts);
            points.resize(3 * num_pts);
            for(size_t i = 0; i < num_pts; i++) {
                color.at(i*3) = b;
                color.at(i*3 + 1) = g;
                color.at(i*3 + 2) = r;

                points.at(i*3) = corner_pts.at(i)(0);
                points.at(i*3 + 1) = corner_pts.at(i)(1);
                points.at(i*3 + 2) = corner_pts.at(i)(2);
            }
            pangolin::glDrawColoredVertices<float,float>(num_pts, &points[0], &color[0], GL_POLYGON, 3, 3);
        }
    }

    return;
}
void PangolinViewer::publish_traj_gt(Eigen::Quaternionf& q_wc, Eigen::Vector3f& t_wc) {
    if(!b_show_trajectory) return;
    std::unique_lock<std::mutex> lk(mutex_3D_show);
    cur_t_wc_gt = t_wc;
    cur_r_wc_gt = q_wc;
    traj_gt.push_back(t_wc);
    return;
}
void PangolinViewer::draw_trajectory_gt(const std::vector<Eigen::Vector3f> &traj, Eigen::Vector3f color) {
    glBegin(GL_LINE_STRIP);
    glColor3fv(color.data());
    for (const auto &p : traj) {
        glVertex3fv(&p[0]);
    }
    glEnd();
    return;
}
