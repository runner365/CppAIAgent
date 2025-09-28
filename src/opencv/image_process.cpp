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
    // 1. Read the input image
    cv::Mat src = cv::imread(inputPath);
    if (src.empty()) {
        LogErrorf(logger, "Failed to load image: %s", inputPath.c_str());
        return -1;
    }

    // 2. Skin region detection (simple skin color mask, more advanced methods can be used in practice)
    cv::Mat ycrcb, skinMask;
    cv::cvtColor(src, ycrcb, cv::COLOR_BGR2YCrCb);
    cv::inRange(ycrcb, cv::Scalar(0, 133, 77), cv::Scalar(255, 173, 127), skinMask);
    cv::GaussianBlur(skinMask, skinMask, cv::Size(5, 5), 0);

    // 3. Bilateral filtering (skin smoothing, preserves edges)
    int d = cvRound(8 + smoothStrength * 10);
    double sigmaColor = 50 + smoothStrength * 50;
    double sigmaSpace = 20 + smoothStrength * 20;
    cv::Mat bilateral;
    cv::bilateralFilter(src, bilateral, d, sigmaColor, sigmaSpace);

    // 4. High-pass filtering (enhance details, prevent loss of facial features)
    cv::Mat gaussian, highpass;
    int ksize = cvRound(3 + smoothStrength * 4) | 1;
    cv::GaussianBlur(src, gaussian, cv::Size(ksize, ksize), 0);
    cv::addWeighted(src, 1.5, gaussian, -0.5, 0, highpass);

    // 5. Blending: apply smoothing + high-pass only to skin regions, keep original for non-skin regions
    cv::Mat beauty = src.clone();
    for (int y = 0; y < src.rows; ++y) {
        for (int x = 0; x < src.cols; ++x) {
            if (skinMask.at<uchar>(y, x) > 128) {
                // Blend smoothing and high-pass
                cv::Vec3b b = bilateral.at<cv::Vec3b>(y, x);
                cv::Vec3b h = highpass.at<cv::Vec3b>(y, x);
                // 0.7 weight for smoothing, 0.3 for details
                beauty.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    cv::saturate_cast<uchar>(b[0] * 0.7 + h[0] * 0.3),
                    cv::saturate_cast<uchar>(b[1] * 0.7 + h[1] * 0.3),
                    cv::saturate_cast<uchar>(b[2] * 0.7 + h[2] * 0.3)
                );
            }
        }
    }

    // 6. Whitening and brightening (increase brightness and slight whitening)
    cv::Mat hsv;
    cv::cvtColor(beauty, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsv, hsvChannels);
    hsvChannels[1] = hsvChannels[1] * (0.95 - 0.2 * smoothStrength); // Lower saturation for whiter skin
    hsvChannels[2] = hsvChannels[2] * (1.08 + 0.2 * smoothStrength); // Increase brightness
    cv::merge(hsvChannels, hsv);
    cv::cvtColor(hsv, beauty, cv::COLOR_HSV2BGR);

    // 7. Save the result
    if (!cv::imwrite(outputPath, beauty)) {
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

int ConvertImage2CyberPunkStyle(const std::string& inputPath, const std::string& outputPath, Logger* logger) {
    try {
        // Read the input image
        cv::Mat img = cv::imread(inputPath);
        if (img.empty()) {
            LogErrorf(logger, "Unable to read image: %s:", inputPath.c_str());
        }

        // Resize image for consistent processing while maintaining aspect ratio
        int maxDim = 1000;
        int height = img.rows;
        int width = img.cols;

        if (std::max(height, width) > maxDim) {
            double scale = static_cast<double>(maxDim) / std::max(height, width);
            cv::resize(img, img, cv::Size(static_cast<int>(width * scale), static_cast<int>(height * scale)));
        }

        // Convert to HSV color space for easier color manipulation
        cv::Mat hsv;
        cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

        // Split HSV channels and adjust saturation
        std::vector<cv::Mat> hsvChannels;
        cv::split(hsv, hsvChannels);

        // Increase saturation for more vibrant colors
        hsvChannels[1] += 50;
        cv::normalize(hsvChannels[1], hsvChannels[1], 0, 255, cv::NORM_MINMAX);

        // Merge HSV channels and convert back to BGR
        cv::merge(hsvChannels, hsv);
        cv::cvtColor(hsv, img, cv::COLOR_HSV2BGR);

        // Split BGR channels to create cyberpunk color profile
        std::vector<cv::Mat> bgrChannels;
        cv::split(img, bgrChannels);

        // Enhance blue channel (signature cyberpunk blue tone)
        bgrChannels[0] += 30;
        cv::normalize(bgrChannels[0], bgrChannels[0], 0, 255, cv::NORM_MINMAX);

        // Reduce green channel to increase contrast
        bgrChannels[1] -= 20;
        cv::normalize(bgrChannels[1], bgrChannels[1], 0, 255, cv::NORM_MINMAX);

        // Enhance red channel (neon effect)
        bgrChannels[2] += 10;
        cv::normalize(bgrChannels[2], bgrChannels[2], 0, 255, cv::NORM_MINMAX);

        // Merge BGR channels back
        cv::merge(bgrChannels, img);

        // Increase contrast and adjust brightness
        double alpha = 1.4;  // Contrast gain
        int beta = -50;      // Brightness offset
        img.convertTo(img, -1, alpha, beta);

        // Add glow effect
        cv::Mat glow;
        cv::GaussianBlur(img, glow, cv::Size(0, 0), 25);
        cv::addWeighted(img, 1.2, glow, 0.3, 0, img);

        // Sharpen the image
        cv::Mat kernel = (cv::Mat_<char>(3, 3) << -1, -1, -1,
            -1, 9, -1,
            -1, -1, -1);
        cv::filter2D(img, img, -1, kernel);

        // Save image if output path is provided
        if (!outputPath.empty()) {
            cv::imwrite(outputPath, img);
            LogInfof(logger, "Cyberpunk style image saved to: %s", outputPath.c_str());
        }
    }
    catch (const std::exception& e) {
        LogErrorf(logger, "Error processing image: %s", e.what());
        return -2;
    }

    return 0;
}