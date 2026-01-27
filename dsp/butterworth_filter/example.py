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
import time

sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j12")
import butterworth_filter
import numpy as np
from scipy import signal

print("=" * 80)
print("Butterworth Filter 精度和速度对比测试")
print("=" * 80)

# 设计一个真实的 Butterworth 滤波器
# 4阶低通滤波器，截止频率 0.125 (归一化频率)
order = 4
Wn = 0.125
b, a = signal.butter(order, Wn, btype='low', analog=False, fs=1.0)

print(f"\n滤波器参数:")
print(f"阶数: {order}")
print(f"截止频率 Wn: {Wn}")
print(f"b 系数: {b}")
print(f"a 系数: {a}")

# 创建 pybind 滤波器对象
filt_cpp = butterworth_filter.ButterworthFilter(b.tolist(), a.tolist())
filt_cpp = butterworth_filter.ButterworthFilter.from_params(order, 1.0, 'lowpass', [Wn])

# 创建测试信号：正弦波 + 高频噪声
np.random.seed(42)  # 固定随机种子以便复现
N = 10000
t = np.linspace(0, 1, N)
# 低频信号 (5 Hz) + 高频噪声 (50 Hz)
signal_clean = np.sin(2 * np.pi * 5 * t)
signal_noise = 0.5 * np.sin(2 * np.pi * 50 * t)
signal_test = signal_clean + signal_noise + 0.1 * np.random.randn(N)

print(f"\n测试信号:")
print(f"信号长度: {N}")
print(f"采样时间: {t[-1]} 秒")

# ==================== filtfilt 测试 ====================
print("\n" + "=" * 80)
print("filtfilt (零相位滤波) 对比测试")
print("=" * 80)

# Scipy filtfilt
t0 = time.perf_counter()
for _ in range(100):
    filtered_scipy = signal.filtfilt(b, a, signal_test)
t_scipy = (time.perf_counter() - t0) / 100

# Pybind filtfilt
signal_test_copy = np.ascontiguousarray(signal_test, dtype=np.float64)
t0 = time.perf_counter()
for _ in range(100):
    filtered_cpp = filt_cpp.filtfilt(signal_test_copy)
t_cpp = (time.perf_counter() - t0) / 100

# 精度对比
max_error = np.max(np.abs(filtered_scipy - filtered_cpp))
mean_error = np.mean(np.abs(filtered_scipy - filtered_cpp))
relative_error = np.max(np.abs((filtered_scipy - filtered_cpp) / (filtered_scipy + 1e-10)))

print(f"\nSciPy filtfilt 平均耗时: {t_scipy*1000:.4f} ms")
print(f"C++ filtfilt 平均耗时:   {t_cpp*1000:.4f} ms")
print(f"加速比: {t_scipy/t_cpp:.2f}x")
print(f"\n精度对比:")
print(f"最大绝对误差: {max_error:.2e}")
print(f"平均绝对误差: {mean_error:.2e}")
print(f"最大相对误差: {relative_error:.2e}")
print(f"前10个值 (SciPy):  {filtered_scipy[:10]}")
print(f"前10个值 (C++):    {filtered_cpp[:10]}")
print(f"后10个值 (SciPy):  {filtered_scipy[-10:]}")
print(f"后10个值 (C++):    {filtered_cpp[-10:]}")

# ==================== lfilter 测试 ====================
print("\n" + "=" * 80)
print("lfilter (单向滤波) 对比测试")
print("=" * 80)

# Scipy lfilter
t0 = time.perf_counter()
for _ in range(100):
    filtered_scipy_lf = signal.lfilter(b, a, signal_test)
t_scipy_lf = (time.perf_counter() - t0) / 100

# Pybind lfilter
t0 = time.perf_counter()
for _ in range(100):
    filtered_cpp_lf, _ = filt_cpp.lfilter(signal_test_copy)
t_cpp_lf = (time.perf_counter() - t0) / 100

# 精度对比
max_error_lf = np.max(np.abs(filtered_scipy_lf - filtered_cpp_lf))
mean_error_lf = np.mean(np.abs(filtered_scipy_lf - filtered_cpp_lf))
relative_error_lf = np.max(np.abs((filtered_scipy_lf - filtered_cpp_lf) / (filtered_scipy_lf + 1e-10)))

print(f"\nSciPy lfilter 平均耗时: {t_scipy_lf*1000:.4f} ms")
print(f"C++ lfilter 平均耗时:   {t_cpp_lf*1000:.4f} ms")
print(f"加速比: {t_scipy_lf/t_cpp_lf:.2f}x")
print(f"\n精度对比:")
print(f"最大绝对误差: {max_error_lf:.2e}")
print(f"平均绝对误差: {mean_error_lf:.2e}")
print(f"最大相对误差: {relative_error_lf:.2e}")

# ==================== 汇总 ====================
print("\n" + "=" * 80)
print("总结")
print("=" * 80)
print(f"filtfilt: C++ 比 SciPy 快 {t_scipy/t_cpp:.2f}x, 最大误差 {max_error:.2e}")
print(f"lfilter:  C++ 比 SciPy 快 {t_scipy_lf/t_cpp_lf:.2f}x, 最大误差 {max_error_lf:.2e}")
print("=" * 80)

# import  matplotlib.pyplot as plt
# plt.figure()
# plt.plot(filtered_scipy, linewidth=4)
# plt.plot(filtered_cpp)
# plt.figure()
# plt.plot(filtered_scipy_lf, linewidth=4)
# plt.plot(filtered_cpp_lf)
# plt.show()
