#ifndef LLM_TOOL_H
#define LLM_TOOL_H
#include "utils/logger.hpp"
#include "llm_info.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>

using namespace cpp_streamer;

typedef struct
{
	int code;
	std::string desc;
	LLMValue value;
} FunctionResult;

using ToolFunction = FunctionResult(*)(std::map<std::string, LLMValue>, Logger* logger);

class LLMTool
{
public:
	LLMTool(Logger* logger);
	~LLMTool();

	void AddTool(const std::string& id, ToolFunction func);
	ToolFunction GetTool(const std::string& id);

public:
	void AddToolDefinition(const ToolDefinition& def) {
		tool_defs_.push_back(def);
	}
	const std::vector<ToolDefinition>& GetToolDefinitions() const {
		return tool_defs_;
	}
private:
	Logger* logger_ = nullptr;
	std::map<std::string, ToolFunction> tools_;// key: function_name, value: function pointer
	std::vector<ToolDefinition> tool_defs_;
};

#endif
