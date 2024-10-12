#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <memory>
#include <iostream>
#include "apriltags/TagFamily.h"
#include "apriltags/TagDetection.h"
#include "apriltags/TagDetector.h"
#include "apriltags/Tag36h11.h"

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cout << "Usage: " << argv[0] << " <path/to/img.png>" << std::endl;
		return 0;
	}
	auto tagCodes = AprilTags::tagCodes36h11;
	auto blackTagBorder = 2;
    auto tagDetector = std::make_shared<AprilTags::TagDetector>(tagCodes, blackTagBorder);
	cv::Mat image = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
	std::vector<AprilTags::TagDetection> detections = tagDetector->extractTags(image);
	for (int i = 0; i < detections.size(); ++i) {
		std::cout << i << ", " << detections[i].id << std::endl;
	}
	return 0;
}
