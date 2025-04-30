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

def random_walk_3d(steps=1000):
    steps = np.random.uniform(-0.001, 0.001, (steps, 3))
    walk = np.cumsum(steps, axis=0)
    return walk

def make_traj():
    timestamps = pd.date_range(start='2024-01-01', periods=100, freq='T')
    
    # 椭圆参数
    a = 0.5  # 椭圆长轴
    b = 0.2  # 椭圆短轴

    # 时间步数
    theta = np.linspace(0, 2 * np.pi, 100)

    # 椭圆轨迹
    t_wc_x = a * np.cos(theta)
    t_wc_y = b * np.sin(theta)
    t_wc_z = np.zeros(100)
    t_wc = np.column_stack((t_wc_x, t_wc_y, t_wc_z))

    # 生成四元数数据 q_wc，使其姿态一直朝向运动方向
    q_wc = []
    for i in range(len(t_wc_x)):
        angle = np.arctan2(t_wc_y[i], t_wc_x[i])
        quat = R.from_euler('z', angle).as_quat()
        q_wc.append(quat)
    q_wc = np.array(q_wc)

    # 构建 DataFrame
    data = np.column_stack((t_wc, q_wc))
    df = pd.DataFrame(data, columns=['t_wc_x', 't_wc_y', 't_wc_z', 'q_wc_x', 'q_wc_y', 'q_wc_z', 'q_wc_w'])
    df.insert(0, 'timestamp', timestamps)
    
    return df

if __name__ == "__main__":
    print("Pangolin 可视化示例")
    # 生成轨迹数据
    traj_data = make_traj().values
    
    # 创建Pangolin查看器
    viewer = pangolin_viewer.PangolinViewer(640, 480, False)
    viewer.set_img_resolution(752, 480)
    viewer.view_init()
    
    # 创建一些测试图像
    img = np.zeros((480, 752, 3), dtype=np.uint8)
    cv2.putText(img, "Pangolin Viewer", (50, 240), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 3)
    
    # 准备一些点云数据（只创建一次，在每帧绘制时使用）
    print("准备点云数据...")
    
    # 立方体点云 - 红色
    cube_size = 0.5
    cube_points = []
    for x in [-cube_size, cube_size]:
        for y in [-cube_size, cube_size]:
            for z in [-cube_size, cube_size]:
                cube_points.append([x, y, z])
    cube_points = np.array(cube_points, dtype=np.float32)
    
    # 球体点云 - 每个点有不同颜色
    theta = np.linspace(0, 2*np.pi, 20)
    phi = np.linspace(0, np.pi, 10)
    sphere_points = []
    sphere_colors = []
    
    radius = 1.0
    for t in theta:
        for p in phi:
            x = radius * np.sin(p) * np.cos(t)
            y = radius * np.sin(p) * np.sin(t)
            z = radius * np.cos(p)
            sphere_points.append([x, y, z])
            
            # 创建彩虹色
            r = (np.cos(t) + 1) / 2
            g = (np.sin(p) + 1) / 2
            b = (np.sin(t) + 1) / 2
            sphere_colors.append([r, g, b])
    
    sphere_points = np.array(sphere_points, dtype=np.float32)
    sphere_colors = np.array(sphere_colors, dtype=np.float32)
    
    # 网格点云 - 绿色
    grid_size = 3
    grid_points = []
    grid_step = 0.2
    for x in np.arange(-grid_size/2, grid_size/2+grid_step, grid_step):
        for z in np.arange(-grid_size/2, grid_size/2+grid_step, grid_step):
            grid_points.append([x, -1.5, z])
    
    grid_points = np.array(grid_points, dtype=np.float32)
    
    # 线段 - 使用默认颜色
    line_points = np.array([[0, 0, 0], [0, 2, 0]], dtype=np.float32)
    
    # 随机点云 - 蓝色
    random_points_1 = np.random.rand(20, 3).astype(np.float32) * 0.5 + np.array([1.5, 0, 1.0], dtype=np.float32)
    
    # 另一个随机点云 - 使用默认颜色
    random_points_2 = np.random.rand(50, 3).astype(np.float32) * 0.5 + np.array([-1.5, 0, 1.0], dtype=np.float32)
    
    # 运行可视化循环
    print("开始可视化循环...")
    idx = 0
    frame_count = 0
    
    # 用于存储动态添加的点
    cube_extra_points = []
    line_extra_points = []
    random_extra_points = []
    
    while viewer.should_not_quit():
        frame_count += 1
        idx = (idx + 1) % len(traj_data)
        
        # 更新轨迹
        viewer.publish_traj(t_wc=traj_data[idx,1:4], q_wc=traj_data[idx,4:])
        
        # 清除上一帧的所有点云
        viewer.clear_all_points()
        
        # 添加基础点云
        viewer.add_points(cube_points, COLOR_RED, "cube", 6.0)
        viewer.add_points_with_colors(sphere_points, sphere_colors, "sphere", 5.0)
        viewer.add_points(grid_points, COLOR_GREEN, "grid", 4.0)
        viewer.add_points(line_points, COLOR_BLUE, "line", 6.0)
        viewer.add_points(random_points_1, COLOR_BLUE, "random_points", 4.0)
        viewer.add_points(random_points_2, COLOR_YELLOW, "random_points_2", 4.0)
        
        # 动态添加点云
        # 示例：每10帧添加一些随机点到立方体集合
        if frame_count % 10 == 0:
            new_points = np.random.rand(5, 3).astype(np.float32) * cube_size - cube_size/2
            cube_extra_points.append(new_points)
            # 只保留最近5次添加的点
            if len(cube_extra_points) > 5:
                cube_extra_points.pop(0)
        
        # 绘制立方体额外点
        for points in cube_extra_points:
            viewer.add_points(points, COLOR_MAGENTA, "cube_extra", 7.0)
        
        # 示例：每20帧添加默认颜色的点
        if frame_count % 20 == 0:
            new_random_points = np.random.rand(3, 3).astype(np.float32) * 0.5 + np.array([1.5, 0, 1.0], dtype=np.float32)
            random_extra_points.append(new_random_points)
            if len(random_extra_points) > 3:
                random_extra_points.pop(0)
        
        # 绘制随机额外点
        for i, points in enumerate(random_extra_points):
            # 随着时间变化颜色
            color = np.array([0.7, 0.2 + i * 0.2, 0.1], dtype=np.float32)
            viewer.add_points(points, color, f"random_extra_{i}", 5.0)
        
        # 示例：每30帧添加点到线段
        if frame_count % 30 == 0:
            height = frame_count * 0.02
            new_line_point = np.array([[0, 2 + height, 0]], dtype=np.float32)
            line_extra_points.append(new_line_point)
            if len(line_extra_points) > 10:
                line_extra_points.pop(0)
        
        # 绘制线段额外点
        for points in line_extra_points:
            viewer.add_points(points, COLOR_CYAN, "line_extra", 8.0)
        
        # 使用颜色名称添加点云
        if frame_count % 40 == 0:
            spinning_point = np.array([[
                np.cos(frame_count * 0.1) * 2, 
                np.sin(frame_count * 0.1) * 2, 
                0.5
            ]], dtype=np.float32)
            viewer.add_points_with_color_name(spinning_point, "orange", "spinning_point", 10.0)
        
        # 更新和显示图像
        cv2.putText(img, f"Frame: {frame_count}", (50, 300), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
        viewer.publish_track_img(img)
        viewer.publish_plane_detection_img(img)
        
        # 渲染一帧
        viewer.show(delay_time_in_s=0.0)
        time.sleep(0.03)
