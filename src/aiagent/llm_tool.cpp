#include "llm_tool.h"

LLMTool::LLMTool(Logger* logger) {
	logger_ = logger;
	LogInfof(logger_, "LLMTool initialized");
}
LLMTool::~LLMTool() {
	tools_.clear();
	LogInfof(logger_, "LLMTool destroyed");
}

void LLMTool::AddTool(const std::string& id, ToolFunction func) {
	if (id.empty() || !func) {
		LogErrorf(logger_, "Invalid tool id or function");
		return;
	}
	tools_[id] = func;
	LogInfof(logger_, "Added tool with id: %s", id.c_str());
}
ToolFunction LLMTool::GetTool(const std::string& id) {
	auto it = tools_.find(id);
	if (it != tools_.end()) {
		return it->second;
	}
	LogErrorf(logger_, "Tool with id: %s not found", id.c_str());
	return nullptr;
}