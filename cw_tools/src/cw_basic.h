#pragma once

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>

namespace cw{
	template<typename T>
	inline void print(const T& text, const std::string& color = "default", bool bold = false) {
		// ANSI color codes
		std::unordered_map<std::string, std::string> colors = {
			{"default", "\033[0m"},  // Default color
			{"red", "\033[31m"},     // Red
			{"green", "\033[32m"},   // Green
			{"yellow", "\033[33m"},  // Yellow
			{"blue", "\033[34m"},    // Blue
			{"magenta", "\033[35m"}, // Magenta
			{"cyan", "\033[36m"},    // Cyan
			{"white", "\033[37m"}    // White
		};
		
		// Set bold style if requested
		std::string bold_code = bold ? "\033[1m" : "";
		
		// Get the color code, defaulting to "default" if not found
		std::string color_code = colors.count(color) ? colors[color] : colors["default"];
		
		// Reset code to default color and style
		std::string end_code = colors["default"];
		
		// Convert the input text to a string using stringstream
		std::ostringstream oss;
		oss << text;
		std::string text_str = oss.str();
		
		// Print the text with the selected color and style
		std::cout << color_code << bold_code << text_str << end_code << std::endl;
	}

	inline std::string getCurrentTimeString(bool showMilliseconds = false) {
		// 获取当前时间点
		auto now = std::chrono::system_clock::now();
		// 转换为time_t类型
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		// 格式化时间为 "YYYY_MM_DD_HH_MM_SS"
		std::tm buf;
		localtime_r(&in_time_t, &buf);
		std::ostringstream oss;
		oss << std::put_time(&buf, "%Y_%m_%d_%H_%M_%S");

		if (showMilliseconds) {
			// 获取当前时间点的毫秒部分
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
			// 格式化为 "YYYY_MM_DD_HH_MM_SS_fff"
			oss << '_' << std::setw(3) << std::setfill('0') << milliseconds.count();
		}

		return oss.str();
	}
}
