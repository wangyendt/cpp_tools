#pragma once

#include <vector>
#include <random>
#include <opencv2/opencv.hpp>

namespace cw {
    inline std::vector<cv::Scalar> colors = {
        cv::Scalar(255, 0, 0),    // 红色
        cv::Scalar(0, 255, 0),    // 绿色
        cv::Scalar(0, 0, 255),    // 蓝色
        cv::Scalar(255, 255, 0),  // 黄色
        cv::Scalar(255, 0, 255),  // 洋红
        cv::Scalar(0, 255, 255),  // 青色
        cv::Scalar(128, 0, 0),    // 栗色
        cv::Scalar(128, 128, 0),  // 橄榄色
        cv::Scalar(0, 128, 0),    // 暗绿色
        cv::Scalar(128, 0, 128),  // 紫色
        cv::Scalar(0, 128, 128),  // 水鸭色
        cv::Scalar(0, 0, 128),    // 海军蓝
        cv::Scalar(255, 165, 0),  // 橙色
        cv::Scalar(255, 20, 147), // 深粉色
        cv::Scalar(75, 0, 130),   // 靛色
        cv::Scalar(173, 216, 230),// 浅蓝色
        cv::Scalar(139, 69, 19),  // 巧克力色
        cv::Scalar(255, 192, 203),// 粉色
        cv::Scalar(255, 218, 185),// 桃色
        cv::Scalar(47, 79, 79),   // 暗灰色
        cv::Scalar(105, 105, 105),// 灰色
        cv::Scalar(220, 20, 60),  // 猩红
        cv::Scalar(0, 255, 127),  // 春绿色
        cv::Scalar(255, 105, 180) // 热粉色
    };

    inline cv::Scalar get_random_color() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, colors.size() - 1);
        int rand_int = dis(gen);
        return colors[rand_int];
    }

	inline cv::Scalar get_color(int index) {
		return colors[index % colors.size()];
	}
}
