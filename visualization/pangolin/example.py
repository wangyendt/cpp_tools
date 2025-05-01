# author: wangye(Wayne)
# license: Apache Licence
# file: example.py
# time: 2023-11-05-12:00:17
# contact: wang121ye@hotmail.com
# site:  wangyendt@github.com
# software: PyCharm
# code is far away from bugs.


import os
import sys
sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j12")
import pangolin_viewer
import numpy as np
import pandas as pd
import time
import cv2
from scipy.spatial.transform import Rotation as R

# 定义颜色常量
COLOR_RED = np.array([1.0, 0.0, 0.0], dtype=np.float32)
COLOR_GREEN = np.array([0.0, 1.0, 0.0], dtype=np.float32)
COLOR_BLUE = np.array([0.0, 0.0, 1.0], dtype=np.float32)
COLOR_YELLOW = np.array([1.0, 1.0, 0.0], dtype=np.float32)
COLOR_CYAN = np.array([0.0, 1.0, 1.0], dtype=np.float32)
COLOR_MAGENTA = np.array([1.0, 0.0, 1.0], dtype=np.float32)
COLOR_WHITE = np.array([1.0, 1.0, 1.0], dtype=np.float32)
COLOR_BLACK = np.array([0.0, 0.0, 0.0], dtype=np.float32)
COLOR_ORANGE = np.array([1.0, 0.5, 0.0], dtype=np.float32)
COLOR_PURPLE = np.array([0.5, 0.0, 0.5], dtype=np.float32)

def random_walk_3d(steps=1000):
    steps = np.random.uniform(-0.001, 0.001, (steps, 3))
    walk = np.cumsum(steps, axis=0)
    return walk

def make_traj():
    timestamps = pd.date_range(start='2024-01-01', periods=100, freq='min')
    
    # 椭圆参数
    a = 0.5  # 椭圆长轴
    b = 0.2  # 椭圆短轴

    # 时间步数
    theta = np.linspace(0, 2 * np.pi, 100)

    # 椭圆轨迹
    t_wc_x = a * np.cos(theta)
    t_wc_y = b * np.sin(theta)
    t_wc_z = np.zeros(100)
    t_wc = np.column_stack((t_wc_x, t_wc_y, t_wc_z)).astype(np.float32)

    # 生成四元数数据 q_wc，使其姿态一直朝向运动方向 (xyzw, Hamilton)
    q_wc = []
    for i in range(len(t_wc_x)):
        angle = np.arctan2(t_wc_y[i], t_wc_x[i]) + np.pi / 2 # 朝向切线方向
        # R.from_euler 的默认顺序是 'zyx'
        quat = R.from_euler('z', angle).as_quat() # 输出 xyzw
        q_wc.append(quat)
    q_wc = np.array(q_wc).astype(np.float32)

    # 构建 DataFrame
    data = np.column_stack((t_wc, q_wc))
    df = pd.DataFrame(data, columns=['t_wc_x', 't_wc_y', 't_wc_z', 'q_wc_x', 'q_wc_y', 'q_wc_z', 'q_wc_w'])
    df.insert(0, 'timestamp', timestamps)
    
    return df

# 生成螺旋线轨迹数据 (用于演示新API)
def make_helix_traj(num_points=100, radius=0.3, height=1.0, turns=3):
    theta = np.linspace(0, turns * 2 * np.pi, num_points)
    t_x = radius * np.cos(theta)
    t_y = radius * np.sin(theta)
    t_z = np.linspace(0, height, num_points)
    positions = np.column_stack((t_x, t_y, t_z)).astype(np.float32)

    # 姿态：相机朝向Z轴正方向，X轴指向螺旋线外侧
    orientations_se3 = []
    for i in range(num_points):
        z_axis = np.array([0, 0, 1]) # Z轴保持不变
        # X轴指向外侧，大致方向为(cos(theta), sin(theta), 0)
        x_dir = np.array([np.cos(theta[i]), np.sin(theta[i]), 0])
        x_axis = x_dir / np.linalg.norm(x_dir)
        # Y轴通过叉乘得到
        y_axis = np.cross(z_axis, x_axis)
        
        rot_mat = np.eye(4)
        rot_mat[:3, 0] = x_axis
        rot_mat[:3, 1] = y_axis
        rot_mat[:3, 2] = z_axis
        rot_mat[:3, 3] = positions[i]
        orientations_se3.append(rot_mat)
        
    return np.array(orientations_se3, dtype=np.float32)


if __name__ == "__main__":
    print("Pangolin 可视化示例")
    # 生成主轨迹数据 (椭圆)
    main_traj_data = make_traj().values
    main_traj_t = main_traj_data[:, 1:4].astype(np.float32)
    main_traj_q = main_traj_data[:, 4:8].astype(np.float32) # xyzw
    
    # 生成额外的轨迹数据 (螺旋线)
    helix_traj_se3 = make_helix_traj()
    
    # 创建Pangolin查看器
    viewer = pangolin_viewer.PangolinViewer(640, 480, False)
    viewer.set_img_resolution(752, 480)
    viewer.view_init()
    
    # 创建一些测试图像
    img = np.zeros((480, 752, 3), dtype=np.uint8)
    cv2.putText(img, "Pangolin Viewer", (50, 240), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 3)
    
    # 准备点云数据
    print("准备点云数据...")
    cube_size = 0.5
    cube_points = np.array([[x, y, z] for x in [-cube_size, cube_size] for y in [-cube_size, cube_size] for z in [-cube_size, cube_size]], dtype=np.float32)
    theta, phi = np.linspace(0, 2*np.pi, 20), np.linspace(0, np.pi, 10)
    radius = 1.0
    sphere_points = np.array([[radius * np.sin(p) * np.cos(t), radius * np.sin(p) * np.sin(t), radius * np.cos(p)] for t in theta for p in phi], dtype=np.float32)
    sphere_colors = np.array([[(np.cos(t) + 1) / 2, (np.sin(p) + 1) / 2, (np.sin(t) + 1) / 2] for t in theta for p in phi], dtype=np.float32)
    grid_size, grid_step = 3, 0.2
    grid_points = np.array([[x, -1.5, z] for x in np.arange(-grid_size/2, grid_size/2+grid_step, grid_step) for z in np.arange(-grid_size/2, grid_size/2+grid_step, grid_step)], dtype=np.float32)
    line_points = np.array([[0, 0, 0], [0, 2, 0]], dtype=np.float32)
    random_points_1 = np.random.rand(20, 3).astype(np.float32) * 0.5 + np.array([1.5, 0, 1.0], dtype=np.float32)
    random_points_2 = np.random.rand(50, 3).astype(np.float32) * 0.5 + np.array([-1.5, 0, 1.0], dtype=np.float32)
    
    # 运行可视化循环
    print("开始可视化循环...")
    idx = 0
    frame_count = 0
    cube_extra_points, line_extra_points, random_extra_points = [], [], []
    
    while viewer.should_not_quit():
        frame_count += 1
        # 更新主轨迹索引（循环）
        main_idx = frame_count % len(main_traj_t)
        
        # 1. 发布主轨迹的当前位姿 (使用旧API)
        # 注意 scipy 输出的 quat 是 xyzw, 我们需要转成 Eigen/Pangolin 的 wxyz
        # 或者在绑定时处理格式
        # 当前绑定add_trajectory_quat默认处理wxyz输入, publish_traj的绑定没有处理，需要手动转
        # current_t = main_traj_t[main_idx]
        # current_q_xyzw = main_traj_q[main_idx]
        # current_q_wxyz = np.array([current_q_xyzw[3], current_q_xyzw[0], current_q_xyzw[1], current_q_xyzw[2]], dtype=np.float32)
        # viewer.publish_traj(t_wc=current_t, q_wc=current_q_wxyz)
        
        # 2. 清除上一帧的额外轨迹和点云
        viewer.clear_all_trajectories()
        viewer.clear_all_points()
        
        # 3. 添加额外的轨迹 (使用新API)
        # 只显示螺旋线的一部分，模拟动态增长
        helix_idx = frame_count % (len(helix_traj_se3) + 50) # 比轨迹长一点，让它显示完后消失一会
        if helix_idx < len(helix_traj_se3):
            # 使用 SE3 添加螺旋线轨迹，显示相机模型
            viewer.add_trajectory_se3(
                poses_se3=helix_traj_se3[:helix_idx+1], 
                color=COLOR_CYAN, 
                label="helix_se3", 
                line_width=2.0, 
                show_cameras=False, 
                camera_size=0.04
            )
        
        # 添加主轨迹的历史路径 (只画线)，使用 quat 添加
        # main_traj_q 是 xyzw, 指定 quat_format="xyzw"
        viewer.add_trajectory_quat(
            positions=main_traj_t[:main_idx+1],
            orientations=main_traj_q[:main_idx+1], # xyzw
            color=COLOR_GREEN,
            quat_format="xyzw", # 指定输入格式
            label="main_history",
            line_width=3.0,
            # show_cameras=True # 不显示历史相机模型
        )

        # 4. 添加点云 (使用之前的API)
        viewer.add_points(cube_points, COLOR_RED, "cube", 6.0)
        viewer.add_points_with_colors(sphere_points, sphere_colors, "sphere", 5.0)
        viewer.add_points(grid_points, COLOR_GREEN, "grid", 4.0)
        viewer.add_points(line_points, COLOR_BLUE, "line", 6.0)
        viewer.add_points(random_points_1, COLOR_BLUE, "random_points", 4.0)
        viewer.add_points(random_points_2, COLOR_YELLOW, "random_points_2", 4.0)
        
        # 5. 更新和显示图像
        cv2.putText(img, f"Frame: {frame_count}", (50, 300), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
        viewer.publish_track_img(img)
        viewer.publish_plane_detection_img(img)
        
        # 6. 渲染一帧
        viewer.show(delay_time_in_s=0.0)
        time.sleep(0.03)
