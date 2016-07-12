/*******************************************************************
*   main.cpp
*   KFAST
*
*	Author: Kareem Omar
*	kareem.omar@uah.edu
*	https://github.com/komrad36
*
*	Last updated Jul 11, 2016
*******************************************************************/

#include <chrono>
#include <iostream>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "KFAST.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

using namespace std::literals;
using namespace std::chrono;

void printReport(const std::string& name, const nanoseconds& dur, size_t num_keypoints) {
	std::cout << std::setw(20) << name << " took " << std::setw(10) << static_cast<double>(dur.count()) * 1e-3 << " us / 30,000 us to find " << std::setw(5) << num_keypoints << " keypoint" << (num_keypoints == 1 ? "." : "s.") << std::endl;
}

int main(int argc, char* argv[]) {
	static_cast<void>(argc);
	static_cast<void>(argv);

	constexpr bool display_images = false;
	constexpr bool nonmax_suppression = true;
	constexpr auto warmups = 100;
	constexpr auto Kruns = 400;
	constexpr auto CVruns = 400;
	constexpr auto thresh = 50;
	constexpr char name[] = "test.jpg";

	cv::Mat image;
	image = cv::imread(name, CV_LOAD_IMAGE_GRAYSCALE);
	if (!image.data) {
		std::cerr << "ERROR: failed to open image. Aborting." << std::endl;
		return EXIT_FAILURE;
	}

	std::vector<Keypoint> my_keypoints;
	// warmup
	for (int i = 0; i < warmups; ++i) KFAST<nonmax_suppression>(image.data, image.cols, image.rows, image.cols, my_keypoints, thresh);
	my_keypoints.clear();
	{
		high_resolution_clock::time_point start = high_resolution_clock::now();
		for (int32_t i = 0; i < Kruns; ++i) {
			KFAST<nonmax_suppression>(image.data, image.cols, image.rows, image.cols, my_keypoints, thresh);
		}
		high_resolution_clock::time_point end = high_resolution_clock::now();
		nanoseconds sum = (end - start) / Kruns;
		printReport("my FAST", sum, my_keypoints.size());
	}


	std::vector<cv::KeyPoint> keypoints;
	// warmup
	for (int i = 0; i < warmups; ++i) cv::FAST(image, keypoints, thresh, nonmax_suppression);
	keypoints.clear();
	{
		high_resolution_clock::time_point start = high_resolution_clock::now();
		for (int32_t i = 0; i < CVruns; ++i) {
			cv::FAST(image, keypoints, thresh, nonmax_suppression);
		}
		high_resolution_clock::time_point end = high_resolution_clock::now();
		nanoseconds sum = (end - start) / CVruns;
		printReport("OpenCV", sum, keypoints.size());
	}

	if (keypoints.size() != my_keypoints.size()) {
		std::cout << "ERROR! Sizes disagree!!!!!" << std::endl;
	}
	else {
		for (size_t i = 0; i < keypoints.size(); ++i) {
			if (keypoints[i].pt.x != my_keypoints[i].x || keypoints[i].pt.y != my_keypoints[i].y) {
				std::cout << "ERROR! Points disagree!!!!!" << std::endl;
				break;
			}
		}
	}

	if (display_images) {
		cv::Mat image2;
		cv::drawKeypoints(image, keypoints, image2, cv::Scalar(255.0, 0.0, 0.0));
		cv::namedWindow("OpenCV FAST", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO);
		cv::imshow("OpenCV FAST", image2);

		cv::Mat image3;
		std::vector<cv::KeyPoint> converted_kps;
		for (const auto& kp : my_keypoints) converted_kps.emplace_back(static_cast<float>(kp.x), static_cast<float>(kp.y), 0.0f, 0.0f, static_cast<float>(kp.score));

		cv::drawKeypoints(image, converted_kps, image3, cv::Scalar(255.0, 0.0, 0.0));
		cv::namedWindow("KFAST", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO);
		cv::imshow("KFAST", image3);
		cv::waitKey(0);
	}
}
