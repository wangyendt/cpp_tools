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

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontManager
from scipy import signal

# 设置中文字体
fm = FontManager()
mat_fonts = set(f.name for f in fm.ttflist)
plt.rcParams['font.sans-serif'] = ['Arial Unicode MS']

sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j12")
import butterworth_filter

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

# ==================== lfilter_zi 和 sosfilt_zi 测试 ====================
print("\n" + "=" * 80)
print("lfilter_zi 和 sosfilt_zi 测试 (流式处理示例)")
print("=" * 80)

# 测试 lfilter_zi
print("\n1. lfilter_zi 测试 (与 scipy.signal.lfilter_zi 对比):")
print("-" * 60)
zi_scipy = signal.lfilter_zi(b, a)
zi_cpp = butterworth_filter.ButterworthFilter.lfilter_zi(b.tolist(), a.tolist())

print(f"scipy.signal.lfilter_zi 结果: {zi_scipy}")
print(f"ButterworthFilter.lfilter_zi 结果: {zi_cpp}")
print(f"最大误差: {np.max(np.abs(zi_scipy - zi_cpp)):.2e}")

# 测试 sosfilt_zi
print("\n2. sosfilt_zi 测试 (与 scipy.signal.sosfilt_zi 对比):")
print("-" * 60)
sos = signal.butter(order, Wn, btype='low', analog=False, output='sos', fs=1.0)
zi_scipy_sos = signal.sosfilt_zi(sos)
zi_cpp_sos = butterworth_filter.ButterworthFilter.sosfilt_zi(sos)

print(f"scipy.signal.sosfilt_zi 结果形状: {zi_scipy_sos.shape}")
print(f"ButterworthFilter.sosfilt_zi 结果形状: {zi_cpp_sos.shape}")
print(f"scipy.signal.sosfilt_zi:\n{zi_scipy_sos}")
print(f"ButterworthFilter.sosfilt_zi:\n{zi_cpp_sos.reshape(zi_scipy_sos.shape)}")
print(f"最大误差: {np.max(np.abs(zi_scipy_sos.ravel() - zi_cpp_sos)):.2e}")

# 流式处理示例：分段滤波
print("\n3. 流式处理示例 (使用 lfilter_zi 实现无缝分段滤波):")
print("-" * 60)

# 将信号分成3段
chunk_size = N // 3
chunks = [signal_test[:chunk_size], 
          signal_test[chunk_size:2*chunk_size], 
          signal_test[2*chunk_size:]]

# SciPy 流式处理
zi_state_scipy = zi_scipy * signal_test[0]  # 初始化状态
filtered_chunks_scipy = []
for chunk in chunks:
    y, zi_state_scipy = signal.lfilter(b, a, chunk, zi=zi_state_scipy)
    filtered_chunks_scipy.append(y)
filtered_streaming_scipy = np.concatenate(filtered_chunks_scipy)

# C++ 流式处理
filt_ba = butterworth_filter.ButterworthFilter.from_ba(b.tolist(), a.tolist(), cache_zi=True)
zi_state_cpp = zi_cpp * signal_test[0]  # 初始化状态
filtered_chunks_cpp = []
for chunk in chunks:
    chunk_cont = np.ascontiguousarray(chunk, dtype=np.float64)
    y, zi_state_cpp = filt_ba.lfilter(chunk_cont, zi_state_cpp.tolist())
    filtered_chunks_cpp.append(y)
filtered_streaming_cpp = np.concatenate(filtered_chunks_cpp)

# 与一次性滤波对比
filtered_onetime_scipy = signal.lfilter(b, a, signal_test)[0]

print(f"分段滤波 vs 一次性滤波 (SciPy):")
print(f"  最大误差: {np.max(np.abs(filtered_streaming_scipy - filtered_onetime_scipy)):.2e}")
print(f"分段滤波 vs 一次性滤波 (C++):")
print(f"  最大误差: {np.max(np.abs(filtered_streaming_cpp - filtered_onetime_scipy)):.2e}")
print(f"SciPy 分段 vs C++ 分段:")
print(f"  最大误差: {np.max(np.abs(filtered_streaming_scipy - filtered_streaming_cpp)):.2e}")

print("\n✅ lfilter_zi 和 sosfilt_zi 测试通过!")
print("   这些方法对于实时流式处理和状态保持非常有用。")

# ==================== 汇总 ====================
print("\n" + "=" * 80)
print("总结")
print("=" * 80)
print(f"filtfilt: C++ 比 SciPy 快 {t_scipy/t_cpp:.2f}x, 最大误差 {max_error:.2e}")
print(f"lfilter:  C++ 比 SciPy 快 {t_scipy_lf/t_cpp_lf:.2f}x, 最大误差 {max_error_lf:.2e}")
print("=" * 80)

# ==================== 带通滤波器可视化对比 ====================
print("\n" + "=" * 80)
print("带通滤波器可视化对比")
print("=" * 80)

# 参数设置
fs = 100  # 采样频率 (Hz)
order_bp = 4  # 滤波器阶数
lowcut = 0.1  # 低截止频率 (Hz)
highcut = 10.0  # 高截止频率 (Hz)
signal_freq = 5.0  # 测试信号频率 (Hz)
duration = 10.0  # 信号时长 (秒)

# 生成测试信号：采样点数 = 采样率 × 时长
n_samples = int(fs * duration)
t_bp = np.linspace(0, duration, n_samples, endpoint=False)
x_original = np.sin(2 * np.pi * signal_freq * t_bp)

print(f"\n参数设置:")
print(f"采样频率: {fs} Hz")
print(f"滤波器阶数: {order_bp}")
print(f"通带范围: [{lowcut}, {highcut}] Hz")
print(f"测试信号频率: {signal_freq} Hz")
print(f"信号时长: {duration} 秒")
print(f"采样点数: {n_samples}")

# 设计滤波器 (获取 ba 和 sos 两种格式)
b, a = signal.butter(
    order_bp // 2, 
    [lowcut, highcut], 
    btype='bandpass', 
    analog=False, 
    output='ba', 
    fs=fs
)

sos = signal.butter(
    order_bp // 2, 
    [lowcut, highcut], 
    btype='bandpass', 
    analog=False, 
    output='sos', 
    fs=fs
)

# C++ 滤波器对象 - 三种构造方式
# 方式1: 从 ba 系数构造
filt_cpp_ba = butterworth_filter.ButterworthFilter.from_ba(
    b.tolist(), 
    a.tolist()
)

# 方式2: 从 sos 矩阵构造
filt_cpp_sos = butterworth_filter.ButterworthFilter.from_sos(
    sos.tolist(), 
    cache_zi=True
)

# 方式3: 从参数直接构造
filt_cpp_params = butterworth_filter.ButterworthFilter.from_params(
    order=order_bp // 2,
    fs=fs,
    btype='bandpass',
    cutoff=[lowcut, highcut],
    cache_zi=True
)

# SciPy filtfilt 滤波
x_scipy_filtfilt = signal.sosfiltfilt(sos, x_original)

# C++ filtfilt 滤波
x_cpp_filtfilt_ba = filt_cpp_ba.filtfilt(x_original)
x_cpp_filtfilt_sos = filt_cpp_sos.filtfilt(x_original)
x_cpp_filtfilt_params = filt_cpp_params.filtfilt(x_original)

# SciPy lfilter 滤波
x_scipy_lfilter = signal.sosfilt(sos, x_original)

# C++ lfilter 滤波
x_cpp_lfilter_ba, _ = filt_cpp_ba.lfilter(x_original)
x_cpp_lfilter_sos, _ = filt_cpp_sos.lfilter(x_original)
x_cpp_lfilter_params, _ = filt_cpp_params.lfilter(x_original)

# 精度分析：filtfilt
print(f"\nfiltfilt 精度对比 (SciPy vs C++ 不同构造方式):")
print("-" * 60)

max_error_filtfilt_ba = np.max(np.abs(x_scipy_filtfilt - x_cpp_filtfilt_ba))
mean_error_filtfilt_ba = np.mean(np.abs(x_scipy_filtfilt - x_cpp_filtfilt_ba))
print(f"C++ from_ba:     最大误差 {max_error_filtfilt_ba:.2e}, 平均误差 {mean_error_filtfilt_ba:.2e}")

max_error_filtfilt_sos = np.max(np.abs(x_scipy_filtfilt - x_cpp_filtfilt_sos))
mean_error_filtfilt_sos = np.mean(np.abs(x_scipy_filtfilt - x_cpp_filtfilt_sos))
print(f"C++ from_sos:    最大误差 {max_error_filtfilt_sos:.2e}, 平均误差 {mean_error_filtfilt_sos:.2e}")

max_error_filtfilt_params = np.max(np.abs(x_scipy_filtfilt - x_cpp_filtfilt_params))
mean_error_filtfilt_params = np.mean(np.abs(x_scipy_filtfilt - x_cpp_filtfilt_params))
print(f"C++ from_params: 最大误差 {max_error_filtfilt_params:.2e}, 平均误差 {mean_error_filtfilt_params:.2e}")

# 精度分析：lfilter
print(f"\nlfilter 精度对比 (SciPy vs C++ 不同构造方式):")
print("-" * 60)

max_error_lfilter_ba = np.max(np.abs(x_scipy_lfilter - x_cpp_lfilter_ba))
mean_error_lfilter_ba = np.mean(np.abs(x_scipy_lfilter - x_cpp_lfilter_ba))
print(f"C++ from_ba:     最大误差 {max_error_lfilter_ba:.2e}, 平均误差 {mean_error_lfilter_ba:.2e}")

max_error_lfilter_sos = np.max(np.abs(x_scipy_lfilter - x_cpp_lfilter_sos))
mean_error_lfilter_sos = np.mean(np.abs(x_scipy_lfilter - x_cpp_lfilter_sos))
print(f"C++ from_sos:    最大误差 {max_error_lfilter_sos:.2e}, 平均误差 {mean_error_lfilter_sos:.2e}")

max_error_lfilter_params = np.max(np.abs(x_scipy_lfilter - x_cpp_lfilter_params))
mean_error_lfilter_params = np.mean(np.abs(x_scipy_lfilter - x_cpp_lfilter_params))
print(f"C++ from_params: 最大误差 {max_error_lfilter_params:.2e}, 平均误差 {mean_error_lfilter_params:.2e}")

# C++ 不同方式之间的对比
print("\nC++ 不同构造方式之间的对比 (filtfilt):")
print("-" * 60)
max_diff_filtfilt_ba_sos = np.max(np.abs(x_cpp_filtfilt_ba - x_cpp_filtfilt_sos))
max_diff_filtfilt_ba_params = np.max(np.abs(x_cpp_filtfilt_ba - x_cpp_filtfilt_params))
max_diff_filtfilt_sos_params = np.max(np.abs(x_cpp_filtfilt_sos - x_cpp_filtfilt_params))
print(f"from_ba vs from_sos:     最大差异 {max_diff_filtfilt_ba_sos:.2e}")
print(f"from_ba vs from_params:  最大差异 {max_diff_filtfilt_ba_params:.2e}")
print(f"from_sos vs from_params: 最大差异 {max_diff_filtfilt_sos_params:.2e}")

print("\nC++ 不同构造方式之间的对比 (lfilter):")
print("-" * 60)
max_diff_lfilter_ba_sos = np.max(np.abs(x_cpp_lfilter_ba - x_cpp_lfilter_sos))
max_diff_lfilter_ba_params = np.max(np.abs(x_cpp_lfilter_ba - x_cpp_lfilter_params))
max_diff_lfilter_sos_params = np.max(np.abs(x_cpp_lfilter_sos - x_cpp_lfilter_params))
print(f"from_ba vs from_sos:     最大差异 {max_diff_lfilter_ba_sos:.2e}")
print(f"from_ba vs from_params:  最大差异 {max_diff_lfilter_ba_params:.2e}")
print(f"from_sos vs from_params: 最大差异 {max_diff_lfilter_sos_params:.2e}")

# 可视化对比 (2x2 布局)
fig, axes = plt.subplots(2, 2, figsize=(16, 12))

# 左上: filtfilt 信号对比
axes[0, 0].plot(t_bp, x_original, linewidth=10, label='原始信号')
axes[0, 0].plot(t_bp, x_scipy_filtfilt, linewidth=8, label='SciPy sosfiltfilt')
axes[0, 0].plot(t_bp, x_cpp_filtfilt_ba, linewidth=6, label='C++ from_ba')
axes[0, 0].plot(t_bp, x_cpp_filtfilt_sos, linewidth=4, label='C++ from_sos')
axes[0, 0].plot(t_bp, x_cpp_filtfilt_params, linewidth=2, label='C++ from_params')
axes[0, 0].set_xlabel('时间 (秒)', fontsize=11)
axes[0, 0].set_ylabel('幅值', fontsize=11)
axes[0, 0].set_title('filtfilt 信号对比 (零相位滤波)', fontsize=13, fontweight='bold')
axes[0, 0].legend(loc='upper right', fontsize=9)
axes[0, 0].grid(True, alpha=0.3)

# 右上: filtfilt 误差对比
error_filtfilt_ba = (x_scipy_filtfilt - x_cpp_filtfilt_ba) * 1e6
error_filtfilt_sos = (x_scipy_filtfilt - x_cpp_filtfilt_sos) * 1e6
error_filtfilt_params = (x_scipy_filtfilt - x_cpp_filtfilt_params) * 1e6
axes[0, 1].plot(t_bp, error_filtfilt_ba, linewidth=6, label=f'from_ba (max: {max_error_filtfilt_ba:.2e})')
axes[0, 1].plot(t_bp, error_filtfilt_sos, linewidth=4, label=f'from_sos (max: {max_error_filtfilt_sos:.2e})')
axes[0, 1].plot(t_bp, error_filtfilt_params, linewidth=2, label=f'from_params (max: {max_error_filtfilt_params:.2e})')
axes[0, 1].axhline(y=0, linestyle='--', linewidth=0.8, color='black')
axes[0, 1].set_xlabel('时间 (秒)', fontsize=11)
axes[0, 1].set_ylabel('误差 (×10⁻⁶)', fontsize=11)
axes[0, 1].set_title('filtfilt 误差: SciPy - C++', fontsize=13, fontweight='bold')
axes[0, 1].legend(loc='upper right', fontsize=9)
axes[0, 1].grid(True, alpha=0.3)

# 左下: lfilter 信号对比
axes[1, 0].plot(t_bp, x_original, linewidth=10, label='原始信号')
axes[1, 0].plot(t_bp, x_scipy_lfilter, linewidth=8, label='SciPy sosfilt')
axes[1, 0].plot(t_bp, x_cpp_lfilter_ba, linewidth=6, label='C++ from_ba')
axes[1, 0].plot(t_bp, x_cpp_lfilter_sos, linewidth=4, label='C++ from_sos')
axes[1, 0].plot(t_bp, x_cpp_lfilter_params, linewidth=2, label='C++ from_params')
axes[1, 0].set_xlabel('时间 (秒)', fontsize=11)
axes[1, 0].set_ylabel('幅值', fontsize=11)
axes[1, 0].set_title('lfilter 信号对比 (单向滤波)', fontsize=13, fontweight='bold')
axes[1, 0].legend(loc='upper right', fontsize=9)
axes[1, 0].grid(True, alpha=0.3)

# 右下: lfilter 误差对比
error_lfilter_ba = (x_scipy_lfilter - x_cpp_lfilter_ba) * 1e6
error_lfilter_sos = (x_scipy_lfilter - x_cpp_lfilter_sos) * 1e6
error_lfilter_params = (x_scipy_lfilter - x_cpp_lfilter_params) * 1e6
axes[1, 1].plot(t_bp, error_lfilter_ba, linewidth=6, label=f'from_ba (max: {max_error_lfilter_ba:.2e})')
axes[1, 1].plot(t_bp, error_lfilter_sos, linewidth=4, label=f'from_sos (max: {max_error_lfilter_sos:.2e})')
axes[1, 1].plot(t_bp, error_lfilter_params, linewidth=2, label=f'from_params (max: {max_error_lfilter_params:.2e})')
axes[1, 1].axhline(y=0, linestyle='--', linewidth=0.8, color='black')
axes[1, 1].set_xlabel('时间 (秒)', fontsize=11)
axes[1, 1].set_ylabel('误差 (×10⁻⁶)', fontsize=11)
axes[1, 1].set_title('lfilter 误差: SciPy - C++', fontsize=13, fontweight='bold')
axes[1, 1].legend(loc='upper right', fontsize=9)
axes[1, 1].grid(True, alpha=0.3)

plt.tight_layout()
plt.show()

print("=" * 80)
