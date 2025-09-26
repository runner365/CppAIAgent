#ifndef LLM_FUNCTION_TOOLS_H
#define LLM_FUNCTION_TOOLS_H

#include "llm_tool.h"
#include "llm_info.h"

#include "utils/logger.hpp"

FunctionResult GetWeather(std::map<std::string, LLMValue>, Logger* logger);
ToolDefinition CreateWeatherFunctionDefinition();

// Image Processing Function : Converts a color image to a grayscale image and performs edge detection
FunctionResult ConvertColorImg2GrayImgTool(std::map<std::string, LLMValue>, Logger* logger);
ToolDefinition ConvertColorImg2GrayImgFunctionDefinition();

// Beauty filter function: make the picture more beatiful, Implements skin smoothing and wrinkle reduction
FunctionResult ApplyBeautyFilterTool(std::map<std::string, LLMValue>, Logger* logger);
ToolDefinition ApplyBeautyFilterFunctionDefinition();

// Cartoonify filter function: Applies a cartoon effect to the image
FunctionResult ApplyCartoonFilterTool(std::map<std::string, LLMValue>, Logger* logger);
ToolDefinition ApplyCartoonFilterFunctionDefinition();

// SunGlasses function: Add sun glasses for a person
FunctionResult ApplySunGlassesTool(std::map<std::string, LLMValue>, Logger* logger);
ToolDefinition ApplySunGlassesFunctionDefinition();

// CyberPunk style function: Convert image to cyberpunk style
FunctionResult ConvertImage2CyberPunkStyleTool(std::map<std::string, LLMValue>, Logger* logger);
ToolDefinition ConvertImage2CyberPunkStyleFunctionDefinition();

#endif

