# author: wangye(Wayne)
# license: Apache Licence
# file: example.py
# time: 2023-11-07-10:55:53
# contact: wang121ye@hotmail.com
# site:  wangyendt@github.com
# software: PyCharm
# code is far away from bugs.


import os
import sys

sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j12")

import adb_logcat_reader

logger = adb_logcat_reader.ADBLogcatReader()
logger.clearLogcat()
logger.startLogcat()
while True:
	line = logger.readLine()
	if not line: break
	print(line)
