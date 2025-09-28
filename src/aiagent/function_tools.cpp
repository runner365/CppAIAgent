#include "function_tools.h"
#include "opencv/image_process.h"
#include "utils/url.h"
#include "utils/timeex.hpp"

using namespace cpp_streamer;

FunctionResult GetWeather(std::map<std::string, LLMValue> inputs, Logger* logger) {

	auto location_it = inputs.find("location");
	if (location_it == inputs.end() || location_it->second.type != LLMValue::LLM_VALUE_STRING) {
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid or missing 'location' parameter";
		return error_result;
	}
	std::string location = location_it->second.string_value;

	auto unit_it = inputs.find("unit");
	if (unit_it == inputs.end() || unit_it->second.type != LLMValue::LLM_VALUE_STRING) {
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid or missing 'unit' parameter";
		return error_result;
	}
	std::string unit = unit_it->second.string_value;

	FunctionResult result;
	result.code = 0;
	result.desc = "Success";
	result.value.type = LLMValue::LLM_VALUE_STRING;
	result.value.string_value = location + " weather is 25 degrees " + unit;

	LogInfof(logger, "GetWeather called, returning dummy weather data");
	return result;
}

ToolDefinition CreateWeatherFunctionDefinition() {
	ToolDefinition def;
	FunctionDefinition fd;
	FunctionParameter params;

	params.type = "object";
	params.required_vec.push_back("location");
	params.required_vec.push_back("unit");

	ParameterProperties location_prop;
	location_prop.type = "string";
	location_prop.description = "The city and state, e.g. San Francisco, CA";
	ParameterProperties unit_prop;
	unit_prop.type = "string";
	unit_prop.description = "The unit of temperature, either 'celsius' or 'fahrenheit'";
	params.properties["location"] = location_prop;
	params.properties["unit"] = unit_prop;

	fd.name = "get_current_weather";
	fd.description = "Get the current weather in a given location";
	fd.parameters = params;

	def.type = "function";
	def.function = fd;
	
	return def;
}

// Image Processing Function : Converts a color image to a grayscale image and performs edge detection
FunctionResult ConvertColorImg2GrayImgTool(std::map<std::string, LLMValue> input_args, Logger* logger) {
	auto src_it = input_args.find("src_img");
	if (src_it == input_args.end()) {
		LogErrorf(logger, "Missing 'src_img' parameter");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Missing 'src_img' parameter";
		return error_result;
	}
	if (src_it->second.type != LLMValue::LLM_VALUE_STRING) {
		LogErrorf(logger, "Invalid 'src_img' parameter type");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid 'src_img' parameter type";
		return error_result;
	}
	std::string src_url = src_it->second.string_value;
	std::string src_dir;
	std::string src_filename;

	bool ret = GetSrcDirPathAndFilename(src_url, src_dir, src_filename);
	if (!ret) {
		LogErrorf(logger, "GetSrcDirPath failed for url: %s", src_url.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "GetSrcDirPath failed for 'src_img'";
		return error_result;
	}

	std::string dst_img_url = src_dir + "/output_" + std::to_string(now_millisec() %100000) + ".jpg";

	int proc_ret = ConvertColorImg2GrayImg(src_url.c_str(), dst_img_url.c_str(), logger);
	if (proc_ret < 0) {
		LogErrorf(logger, "ConvertColorImg2GrayImg failed for src: %s", src_filename.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "ConvertColorImg2GrayImg failed";
		return error_result;
	}
	FunctionResult result;
	result.code = 0;
	result.desc = "Success";
	result.value.type = LLMValue::LLM_VALUE_STRING;
	result.value.string_value = dst_img_url;
	return result;
}

ToolDefinition ConvertColorImg2GrayImgFunctionDefinition() {
	ToolDefinition def;

	FunctionDefinition fd;
	FunctionParameter params;
	params.type = "object";
	params.required_vec.push_back("src_img");
	ParameterProperties src_img_prop;
	src_img_prop.type = "string";
	src_img_prop.description = "The source image file path";
	params.properties["src_img"] = src_img_prop;
	fd.name = "convert_color_img_to_gray_img";
	fd.description = "Convert a color image to a grayscale image and perform edge detection";
	fd.parameters = params;
	def.type = "function";
	def.function = fd;

	return def;
}

// Beauty filter function: make the picture more beatiful, Implements skin smoothing and wrinkle reduction
FunctionResult ApplyBeautyFilterTool(std::map<std::string, LLMValue> input_args, Logger* logger) {
	FunctionResult result;

	auto src_it = input_args.find("src_img");
	if (src_it == input_args.end()) {
		LogErrorf(logger, "Missing 'src_img' parameter");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Missing 'src_img' parameter";
		return error_result;
	}
	if (src_it->second.type != LLMValue::LLM_VALUE_STRING) {
		LogErrorf(logger, "Invalid 'src_img' parameter type");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid 'src_img' parameter type";
		return error_result;
	}
	std::string src_url = src_it->second.string_value;
	std::string src_dir;
	std::string src_filename;

	bool ret = GetSrcDirPathAndFilename(src_url, src_dir, src_filename);
	if (!ret) {
		LogErrorf(logger, "GetSrcDirPath failed for url: %s", src_url.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "GetSrcDirPath failed for 'src_img'";
		return error_result;
	}

	std::string dst_img_url = src_dir + "/output_" + std::to_string(now_millisec() % 100000) + ".jpg";
	float smoothStrength = 0.4f;

	int proc_ret = ApplyBeautyFilter(src_url, dst_img_url, smoothStrength, logger);
	if (proc_ret < 0) {
		LogErrorf(logger, "ApplyBeautyFilter failed for src: %s", src_filename.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "ApplyBeautyFilter failed";
		return error_result;
	}
	result.code = 0;
	result.desc = "Success";
	result.value.type = LLMValue::LLM_VALUE_STRING;
	result.value.string_value = dst_img_url;
	return result;
}

ToolDefinition ApplyBeautyFilterFunctionDefinition() {
	ToolDefinition def;

	FunctionDefinition fd;
	FunctionParameter params;
	params.type = "object";
	params.required_vec.push_back("src_img");
	ParameterProperties src_img_prop;
	src_img_prop.type = "string";
	src_img_prop.description = "The source image file path";
	params.properties["src_img"] = src_img_prop;
	fd.name = "apply_beauty_filter";
	fd.description = "Apply a beauty filter to an image to make it more beautiful";
	fd.parameters = params;
	def.type = "function";
	def.function = fd;

	return def;
}

// Cartoonify filter function: Applies a cartoon effect to the image
FunctionResult ApplyCartoonFilterTool(std::map<std::string, LLMValue> input_args, Logger* logger) {
	auto src_it = input_args.find("src_img");
	if (src_it == input_args.end()) {
		LogErrorf(logger, "Missing 'src_img' parameter");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Missing 'src_img' parameter";
		return error_result;
	}
	if (src_it->second.type != LLMValue::LLM_VALUE_STRING) {
		LogErrorf(logger, "Invalid 'src_img' parameter type");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid 'src_img' parameter type";
		return error_result;
	}
	std::string src_url = src_it->second.string_value;
	std::string src_dir;
	std::string src_filename;
	bool ret = GetSrcDirPathAndFilename(src_url, src_dir, src_filename);
	if (!ret) {
		LogErrorf(logger, "GetSrcDirPath failed for url: %s", src_url.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "GetSrcDirPath failed for 'src_img'";
		return error_result;
	}
	std::string dst_img_url = src_dir + "/output_" + std::to_string(now_millisec() % 100000) + ".jpg";

	int proc_ret = ApplyCartoonFilter(src_url, dst_img_url, logger);
	if (proc_ret < 0) {
		LogErrorf(logger, "ApplyCartoonFilter failed for src: %s", src_filename.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "ApplyCartoonFilter failed";
		return error_result;
	}
	FunctionResult result;
	result.code = 0;
	result.desc = "Success";
	result.value.type = LLMValue::LLM_VALUE_STRING;
	result.value.string_value = dst_img_url;

	return result;
}

ToolDefinition ApplyCartoonFilterFunctionDefinition() {
	ToolDefinition def;
	FunctionDefinition fd;
	FunctionParameter params;
	params.type = "object";
	params.required_vec.push_back("src_img");
	ParameterProperties src_img_prop;
	src_img_prop.type = "string";
	src_img_prop.description = "The source image file path";
	params.properties["src_img"] = src_img_prop;
	fd.name = "apply_cartoon_filter";
	fd.description = "Apply a cartoon filter to an image to make it look like a cartoon";
	fd.parameters = params;
	def.type = "function";
	def.function = fd;
	return def;
}

FunctionResult ApplySunGlassesTool(std::map<std::string, LLMValue> input_args, Logger* logger) {
	auto src_it = input_args.find("src_img");
	if (src_it == input_args.end()) {
		LogErrorf(logger, "Missing 'src_img' parameter");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Missing 'src_img' parameter";
		return error_result;
	}
	if (src_it->second.type != LLMValue::LLM_VALUE_STRING) {
		LogErrorf(logger, "Invalid 'src_img' parameter type");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid 'src_img' parameter type";
		return error_result;
	}
	std::string src_url = src_it->second.string_value;
	std::string src_dir;
	std::string src_filename;
	bool ret = GetSrcDirPathAndFilename(src_url, src_dir, src_filename);
	if (!ret) {
		LogErrorf(logger, "GetSrcDirPath failed for url: %s", src_url.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "GetSrcDirPath failed for 'src_img'";
		return error_result;
	}
	std::string dst_img_url = src_dir + "/output_" + std::to_string(now_millisec() % 100000) + ".png";
	int proc_ret = ApplySunGlasses(src_url, dst_img_url, logger);
	if (proc_ret < 0) {
		LogErrorf(logger, "ApplySunGlasses failed for src: %s", src_filename.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "ApplySunGlasses failed";
		return error_result;
	}
	FunctionResult result;
	result.code = 0;
	result.desc = "Success";
	result.value.type = LLMValue::LLM_VALUE_STRING;
	result.value.string_value = dst_img_url;
	return result;
}

ToolDefinition ApplySunGlassesFunctionDefinition() {
	ToolDefinition def;
	FunctionDefinition fd;
	FunctionParameter params;
	params.type = "object";
	params.required_vec.push_back("src_img");
	ParameterProperties src_img_prop;
	src_img_prop.type = "string";
	src_img_prop.description = "The source image file path";
	params.properties["src_img"] = src_img_prop;
	fd.name = "apply_sun_glasses";
	fd.description = "Apply sun glasses to a person in the image";
	fd.parameters = params;
	def.type = "function";
	def.function = fd;
	return def;
}

FunctionResult ConvertImage2CyberPunkStyleTool(std::map<std::string, LLMValue> input_args, Logger* logger) {
	std::string src_url;

	auto src_it = input_args.find("src_img");
	if (src_it == input_args.end()) {
		LogErrorf(logger, "Missing 'src_img' parameter");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Missing 'src_img' parameter";
		return error_result;
	}
	if (src_it->second.type != LLMValue::LLM_VALUE_STRING) {
		LogErrorf(logger, "Invalid 'src_img' parameter type");
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "Invalid 'src_img' parameter type";
		return error_result;
	}
	src_url = src_it->second.string_value;
	std::string src_dir;
	std::string src_filename;
	bool ret = GetSrcDirPathAndFilename(src_url, src_dir, src_filename);
	if (!ret) {
		LogErrorf(logger, "GetSrcDirPath failed for url: %s", src_url.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "GetSrcDirPath failed for 'src_img'";
		return error_result;
	}
	std::string dst_img_url = src_dir + "/output_" + std::to_string(now_millisec() % 100000) + ".jpg";
	int proc_ret = ConvertImage2CyberPunkStyle(src_url, dst_img_url, logger);
	if (proc_ret < 0) {
		LogErrorf(logger, "ConvertImage2CyberPunkStyle failed for src: %s", src_filename.c_str());
		FunctionResult error_result;
		error_result.code = -1;
		error_result.desc = "ConvertImage2CyberPunkStyle failed";
		return error_result;
	}
	FunctionResult result;
	result.code = 0;
	result.desc = "Success";
	result.value.type = LLMValue::LLM_VALUE_STRING;
	result.value.string_value = dst_img_url;
	return result;
}

ToolDefinition ConvertImage2CyberPunkStyleFunctionDefinition() {
	ToolDefinition def;
	FunctionDefinition fd;
	FunctionParameter params;
	params.type = "object";
	params.required_vec.push_back("src_img");
	ParameterProperties src_img_prop;
	src_img_prop.type = "string";
	src_img_prop.description = "The source image file path";
	params.properties["src_img"] = src_img_prop;
	fd.name = "convert_image_to_cyberpunk_style";
	fd.description = "Convert an image to cyberpunk style";
	fd.parameters = params;
	def.type = "function";
	def.function = fd;
	return def;
}