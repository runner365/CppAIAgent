#include "image_process.h"
#include "utils/logger.hpp"

using namespace cpp_streamer;

cv::Mat cartoonifyImage(cv::Mat src, int edgeThreshold = 90);

int ConvertColorImg2GrayImg(const char* inputPath, const char* outputPath, Logger* logger) {
    // 1. Read color image
    cv::Mat src = cv::imread(inputPath, cv::IMREAD_COLOR);
    if (src.empty()) {
        LogErrorf(logger, "cv reand img error");
        return -1;
    }

    // 2. Convert to grayscale image
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    // 3. Gaussian blur for noise reduction
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);

    // 4. Save processing result
    bool saved = cv::imwrite(outputPath, blurred);
    if (!saved) {
        LogErrorf(logger, "write output file error, output:%s", outputPath);
        return -1;
    }
	return 0;
}

// Beauty filter function: Implements skin smoothing and wrinkle reduction
int ApplyBeautyFilter(const std::string& inputPath, const std::string& outputPath, float smoothStrength, Logger* logger) {
    // 1. Load input image (using C++ interface's imread)
    cv::Mat src = cv::imread(inputPath);
    if (src.empty()) {
		LogErrorf(logger, "Failed to load image: %s", inputPath.c_str());
        return -1;
    }

    // 2. Create intermediate variables (using cv::Mat instead of IplImage)
    cv::Mat gray, mask, smoothed, temp;

    // 3. Convert to grayscale for edge detection (to preserve important features)
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    // 4. Edge detection and mask generation (protects eyebrows, eyes, lips, etc.)
    cv::Canny(gray, mask, 30, 90);                // Detect edges
    cv::threshold(mask, mask, 10, 255, cv::THRESH_BINARY_INV);  // Invert mask (non-edge areas will be smoothed)
    cv::GaussianBlur(mask, mask, cv::Size(5, 5), 0);  // Soften mask edges to avoid harsh transitions

    // 5. Bilateral filtering (core skin smoothing algorithm: smooths skin while preserving edges)
    int d = cvRound(5 + smoothStrength * 10);  // Diameter of pixel neighborhood
    double sigmaColor = 100 + smoothStrength * 50;  // Color similarity sigma
    double sigmaSpace = 30 + smoothStrength * 20;   // Spatial similarity sigma
    cv::bilateralFilter(src, temp, d, sigmaColor, sigmaSpace);

    // 6. Gaussian blur to enhance wrinkle reduction
    int ksize = cvRound(3 + smoothStrength * 4) | 1;  // Ensure kernel size is odd
    cv::GaussianBlur(temp, smoothed, cv::Size(ksize, ksize), 0);

    // 7. Image blending (merge smoothed areas with original facial features)
    src.copyTo(temp);  // Copy original image to temporary variable
    smoothed.copyTo(temp, mask);  // Apply smoothing only to non-edge areas

    // 8. Save result
    if (!cv::imwrite(outputPath, temp)) {
		LogErrorf(logger, "Failed to save output image: %s", outputPath.c_str());
        return -2;
    }

	LogInfof(logger, "Beauty filter applied successfully, output saved to: %s", outputPath.c_str());
    return 0;
}

cv::Mat cartoonifyImage(cv::Mat src, int edgeThreshold)
{
    // 1. Edge detection (extract outlines)
    cv::Mat gray, edges;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);        // Convert to grayscale
    cv::medianBlur(gray, gray, 7);                      // Remove noise to reduce spurious edges
    cv::Canny(gray, edges, edgeThreshold / 2, edgeThreshold); // Canny edge detector
    edges = 255 - edges;                                // Invert: black edges become white
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);     // Convert to 3-channel for blending

    // 2. Color simplification (smoothing + quantization)
    cv::Mat smooth;
    cv::bilateralFilter(src, smooth, 15, 80, 80);       // Edge-preserving smoothing
    cv::Mat cartoon;
    cv::bitwise_and(smooth, edges, cartoon);            // Combine smoothed image with edges

    return cartoon;
}

// Add sun glasses for a person
int addSunGlasses(cv::Mat& src, cv::Mat& sunglasses)
{
    // Load the frontal-face Haar cascade
    cv::CascadeClassifier faceCascade;
    if (!faceCascade.load("haarcascade_frontalface_default.xml")) {
        return -1;
    }

    // Detect faces
    std::vector<cv::Rect> faces;
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(100, 100));

    for (auto& face : faces)
    {
        // Estimate eye region: upper third of the face
        int eyeY = face.y + face.height / 4;
        int eyeWidth = static_cast<int>(face.width * 0.8);
        int eyeHeight = eyeWidth / 3;
        cv::Rect eyeROI(face.x + static_cast<int>(face.width * 0.1),
            eyeY,
            eyeWidth,
            eyeHeight);

        // Resize sunglasses to fit the estimated eye region
        cv::Mat resizedSunglasses;
        cv::resize(sunglasses, resizedSunglasses, eyeROI.size());

        // Overlay using alpha channel (assume sunglasses is 4-channel BGRA)
        cv::Mat roi = src(eyeROI);
        for (int c = 0; c < 3; ++c)
        {
            for (int i = 0; i < eyeROI.height; ++i)
            {
                for (int j = 0; j < eyeROI.width; ++j)
                {
                    uchar alpha = resizedSunglasses.data[i * resizedSunglasses.step + j * 4 + 3];
                    if (alpha > 10) // non-transparent area
                    {
                        roi.data[i * roi.step + j * 3 + c] =
                            resizedSunglasses.data[i * resizedSunglasses.step + j * 4 + c];
                    }
                }
            }
        }
    }
    return 0;
}


// make picture cartoon style
int ApplyCartoonFilter(const std::string& inputPath, const std::string& outputPath, Logger* logger) {
	// 1. Load input image
	cv::Mat src = cv::imread(inputPath);
	if (src.empty()) {
		LogErrorf(logger, "Failed to load image: %s", inputPath.c_str());
		return -1;
	}
	// 2. Apply cartoon effect
	cv::Mat cartoon = cartoonifyImage(src, 10);
	// 3. Save result
	if (!cv::imwrite(outputPath, cartoon)) {
		LogErrorf(logger, "Failed to save output image: %s", outputPath.c_str());
		return -2;
	}
	LogInfof(logger, "Cartoon filter applied successfully, output saved to: %s", outputPath.c_str());
	return 0;
}

// add sun glasses for a person
int ApplySunGlasses(const std::string& inputPath, const std::string& outputPath, Logger* logger) {
	// 1. Load input image
	cv::Mat src = cv::imread(inputPath);
	if (src.empty()) {
		LogErrorf(logger, "Failed to load image: %s", inputPath.c_str());
		return -1;
	}
	// 2. Load sunglasses image (with alpha channel)
    cv::Mat sunglasses;

	try {
        int ret = addSunGlasses(src, sunglasses);
        if (ret < 0) {
            LogErrorf(logger, "Failed to add sunglasses, err:%d", ret);
            return ret;
        }
	}
	catch (const cv::Exception& e) {
		LogErrorf(logger, "Exception loading sunglasses image: %s", e.what());
		return -1;
	}

	// 3. Save result
	if (!cv::imwrite(outputPath, sunglasses)) {
		LogErrorf(logger, "Failed to save output image: %s", outputPath.c_str());
		return -2;
	}
	return 0;
}