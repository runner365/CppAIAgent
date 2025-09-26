#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H

#include "utils/logger.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <string>

using namespace cpp_streamer;

//Image Processing Function: Converts a color image to a grayscale image and performs edge detection
int ConvertColorImg2GrayImg(const char* inputPath, const char* outputPath, Logger* logger);

// Beauty filter function: Implements skin smoothing and wrinkle reduction, smoothStrength:(0.0..1.0)
int ApplyBeautyFilter(const std::string& inputPath, const std::string& outputPath, float smoothStrength, Logger* logger);

// Cartoonify filter function: Applies a cartoon effect to the image
int ApplyCartoonFilter(const std::string& inputPath, const std::string& outputPath, Logger* logger);

// add sun glasses for a person
int ApplySunGlasses(const std::string& inputPath, const std::string& outputPath, Logger* logger);

// Converts an image to cyberpunk style 
int ConvertImage2CyberPunkStyle(const std::string& inputPath, const std::string& outputPath, Logger* logger);

#endif
