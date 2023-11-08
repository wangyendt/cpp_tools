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


def random_walk_3d(steps=1000):
    steps = np.random.uniform(-0.001, 0.001, (steps, 3))
    walk = np.cumsum(steps, axis=0)
    return walk


viewer = pangolin_viewer.PangolinViewer(640, 480, False)
viewer.set_img_resolution(752, 480)
# viewer.run()
viewer.view_init()
cur_pos = np.zeros((100, 3))
while viewer.should_not_quit():
    random_walk = random_walk_3d(100)
    cur_pos = cur_pos + random_walk
    viewer.publish_3D_points(cur_pos, cur_pos)
    viewer.show(delay_time_in_s=0.0)
