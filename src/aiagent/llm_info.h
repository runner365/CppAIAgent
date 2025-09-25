#ifndef LLM_INFO_H
#define LLM_INFO_H

#include "utils/json.hpp"
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <list>
#include <vector>
#include <memory>

using json = nlohmann::json;

class ChatCompletionsResponse;
class LLMResponseInterface
{
public:
	virtual void OnResponse(int code, const std::string& err_msg, const std::string& id, std::shared_ptr<ChatCompletionsResponse> resp_ptr) = 0;
};


//定义一个类，内含多种类型: number, string, 类名LLMValue
class LLMValue
{
public:
	enum ValueType {
		LLM_VALUE_NULL,
		LLM_VALUE_NUMBER,
		LLM_VALUE_STRING,
		LLM_VALUE_BOOL,
		LLM_VALUE_OBJECT,
		LLM_VALUE_ARRAY
	};
public:
	ValueType type;
	double number_value;
	std::string string_value;
	bool bool_value;
	json object_value; //使用nlohmann::json来表示对象
	std::list<LLMValue> array_value;
public:
	LLMValue() : type(LLM_VALUE_NULL), number_value(0), bool_value(false) {}
	~LLMValue() {}
};

//using 定义一个函数指针类型 LLMFunction，表示一个函数，参数为一个LLMValue对象，返回值为一个LLMValue对象
typedef LLMValue(*LLMFunction)(const LLMValue& input);

typedef struct
{
	std::string type;
	std::string description;
} ParameterProperties;

class FunctionParameter
{
public:
	FunctionParameter() = default;
	~FunctionParameter() = default;
	json ToJson() const;

public:
	std::string type;
	std::map<std::string, ParameterProperties> properties;
	std::vector<std::string> required_vec;
};

class FunctionDefinition
{
public:
	FunctionDefinition() = default;
	~FunctionDefinition() = default;
	json ToJson() const;

public:
	std::string name;
	std::string description;
	FunctionParameter parameters;
};

class ToolDefinition
{

public:
	ToolDefinition() = default;
	~ToolDefinition() = default;

	json ToJson() const;

public:
	std::string type;
	FunctionDefinition function;
};

typedef struct ToolCallParameter_S {
	std::string name;
	std::string parameters;
} ToolCallParameter;

class ToolCall
{
public:
	ToolCall() {}
	~ToolCall() {}

public:
	std::string id;
	std::string type;
	ToolCallParameter function_parameters;
};

class ChatCompletionsMessage
{
public:
	static std::shared_ptr<ChatCompletionsMessage> Parse(json& input_json);
	std::string Dump();
public:
	std::string role; // "user", "assistant", "system"
	std::string content;

public:
	std::string tool_call_id;//omitempty
	std::vector<ToolCall> tool_calls;//omitempty 
};

class ChatCompletionsInfo
{
public:
	ChatCompletionsInfo() = default;
	~ChatCompletionsInfo() = default;

public:
	std::string DumpJson() const;

public:
	std::string model;
	std::list<ChatCompletionsMessage> messages;
	std::vector<ToolDefinition> tools_definition;//omitempty
};

class TokensUsage
{
public:
	TokensUsage() = default;
	~TokensUsage() = default;

public:
	int prompt_tokens = 0;
	int completion_tokens = 0;
	int total_tokens = 0;
};

class ChatCompletionsChoice
{
public:
	ChatCompletionsChoice() = default;
	~ChatCompletionsChoice() = default;

public:
	static std::shared_ptr<ChatCompletionsChoice> Parse(json& input_json);
	std::string Dump();

public:
	int index = 0;
	ChatCompletionsMessage message;
	std::string finish_reason;
};


class ChatCompletionsResponse
{
public:
	ChatCompletionsResponse() = default;
	~ChatCompletionsResponse() = default;

public:
	static std::shared_ptr<ChatCompletionsResponse> Parse(json& input_json);
	std::string Dump();

public:
	std::string id;
	std::string object;
	int64_t created = 0;
	std::string model_name;//model
	std::vector<ChatCompletionsChoice> choices;
	TokensUsage usage;
	std::string note;//omitempty

};
#endif