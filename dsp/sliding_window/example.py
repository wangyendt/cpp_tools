# author: wangye(Wayne)
# license: Apache Licence
# file: example.py
# time: 2023-11-06-16:28:17
# contact: wang121ye@hotmail.com
# site:  wangyendt@github.com
# software: PyCharm
# code is far away from bugs.


import os
import sys

sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j12")
import sliding_window_dsp
import numpy as np

dsp = sliding_window_dsp.WelfordStd(100)
arr = np.arange(100)
for i in range(105):
	dsp.calcSlidingStd(i)
	print(dsp.getCnt(), dsp.getStd(), np.std(arr[:i+1], ddof=1))
