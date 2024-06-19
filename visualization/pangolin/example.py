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
from scipy.spatial.transform import Rotation as R

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

# data = pd.read_csv('./data.csv',delimiter=',').values
data = make_traj().values
viewer = pangolin_viewer.PangolinViewer(640, 480, False)
viewer.set_img_resolution(752, 480)
# viewer.run()
viewer.view_init()
cur_pos = np.zeros((100, 3))
idx = 0
while viewer.should_not_quit():
    idx = (idx + 1) % len(data)
    random_walk = random_walk_3d(100)
    cur_pos = cur_pos + random_walk
    viewer.publish_3D_points(cur_pos, cur_pos)
    viewer.publish_traj(t_wc=data[idx,1:4], q_wc=data[idx,4:])
    viewer.show(delay_time_in_s=0.0)
    time.sleep(0.03)
