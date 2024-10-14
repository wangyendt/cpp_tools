# author: wangye(Wayne)
# license: Apache Licence
# file: example.py
# time: 2024-10-10-11:00:27
# contact: wang121ye@hotmail.com
# site:  wangyendt@github.com
# software: PyCharm
# code is far away from bugs.


import sys
import os

sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j12")

import apriltag_detection
import numpy as np
import cv2

def resize_image(image, width=640):
    """等比例缩放图像到指定宽度"""
    height = int(image.shape[0] * (width / image.shape[1]))
    return cv2.resize(image, (width, height))

if os.path.exists('test.png'):
    img_path = 'test.png'
else:
    if len(sys.argv) > 1:
        img_path = sys.argv[1]
        if not os.path.exists(img_path):
            print('Usage: python example.py <path/to/img>')
            exit(0)
    else:
        print('Usage: python example.py <path/to/img>')
        exit(0)



# 加载图像
image = cv2.imread(img_path)
gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

# 创建 TagDetector
tag_codes = apriltag_detection.tag_codes_36h11()
black_border = 2
detector = apriltag_detection.TagDetector(tag_codes, black_border)

# 检测标签
detections = detector.extract_tags(gray)

# 在图像上绘制检测结果
for detection in detections:
    # 绘制边框（绿色）
    corners = np.array(detection.corners, dtype=np.int32).reshape((-1, 1, 2))
    cv2.polylines(image, [corners], True, (0, 255, 0), 8)  # 绿色 (B, G, R)
    
    # 绘制角点（红色圆圈）
    for corner in corners:
        cv2.circle(image, tuple(corner[0]), 5, (0, 0, 255), -1)  # 红色 (B, G, R)
    
    # 绘制 ID（红色文字）
    center = tuple(map(int, detection.center))
    text = str(detection.id)
    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 2.0
    font_thickness = 6
    text_size = cv2.getTextSize(text, font, font_scale, font_thickness)[0]
    
    # 计算文字应该放置的位置，使其居中
    text_x = int(center[0] - text_size[0] / 2)
    text_y = int(center[1] + text_size[1] / 2)
    
    cv2.putText(image, text, (text_x, text_y), font, font_scale, (0, 0, 255), font_thickness)
    
    # 打印检测结果
    print(f"检测到 AprilTag:")
    print(f"  ID: {detection.id}")
    print(f"  汉明距离: {detection.hamming_distance}")
    print(f"  中心: {detection.center}")
    print(f"  角点: {detection.corners}")
    print()

# 等比例缩放图像
resized_image = resize_image(image, width=800)

# 显示结果
cv2.imshow('AprilTag Detection', resized_image)
cv2.waitKey(0)
cv2.destroyAllWindows()

# 保存结果图像（原始大小）
# cv2.imwrite('apriltag_detection_result.png', image)

# 保存缩放后的结果图像
# cv2.imwrite('apriltag_detection_result_resized.png', resized_image)
