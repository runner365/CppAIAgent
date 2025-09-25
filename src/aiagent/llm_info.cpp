#include "llm_info.h"

std::string ChatCompletionsInfo::DumpJson() const {
	json j;

	j["model"] = model;
	j["messages"] = json::array();

	for (const auto& msg : messages) {
		json jmsg;
		jmsg["role"] = msg.role;
		jmsg["content"] = msg.content;
		if (!msg.tool_call_id.empty()) {
			jmsg["tool_call_id"] = msg.tool_call_id;
		}
		if (!msg.tool_calls.empty()) {
			jmsg["tool_calls"] = json::array();
			for (const auto& tool : msg.tool_calls) {
				json jtool;
				jtool["id"] = tool.id;
				jtool["type"] = tool.type;
				jtool["function"] = json::object();
				jtool["function"]["name"] = tool.function_parameters.name;
				jtool["function"]["arguments"] = tool.function_parameters.parameters;
				jmsg["tool_calls"].push_back(jtool);
			}
		}
		j["messages"].push_back(jmsg);
	}
	if (tools_definition.size() > 0) {
		j["tools"] = json::array();
		for (const auto& tool_def : tools_definition) {
			j["tools"].push_back(tool_def.ToJson());
		}
	}

	return j.dump();
}

std::string ChatCompletionsMessage::Dump() {
	json j;
	j["role"] = role;
	j["content"] = content;

	if (!tool_call_id.empty()) {
		j["tool_call_id"] = tool_call_id;
	}
	if (!tool_calls.empty()) {
		j["tool_calls"] = json::array();
		for (const auto& tool : tool_calls) {
			json jtool;
			jtool["id"] = tool.id;
			jtool["type"] = tool.type;
			jtool["function"] = json::object();
			jtool["function"]["name"] = tool.function_parameters.name;
			jtool["function"]["arguments"] = tool.function_parameters.parameters;
			j["tool_calls"].push_back(jtool);
		}
	}
	return j.dump();
}

std::shared_ptr<ChatCompletionsMessage> ChatCompletionsMessage::Parse(json& input_json) {
	auto msg_ptr = std::make_shared<ChatCompletionsMessage>();
	auto role_it = input_json.find("role");
	if (role_it != input_json.end() && role_it->is_string()) {
		msg_ptr->role = role_it->get<std::string>();
	}
	else {
		return nullptr;
	}
	auto content_it = input_json.find("content");
	if (content_it != input_json.end() && content_it->is_string()) {
		msg_ptr->content = content_it->get<std::string>();
	}
	else {
		return nullptr;
	}
	auto tool_calls_it = input_json.find("tool_calls");
	if (tool_calls_it != input_json.end() && tool_calls_it->is_array()) {
		for (auto& tool_item : *tool_calls_it) {
			ToolCall tool_call;

			auto id_it = tool_item.find("id");
			if (id_it != tool_item.end() && id_it->is_string()) {
				tool_call.id = id_it->get<std::string>();
			}
			else {
				return nullptr;
			}
			auto type_it = tool_item.find("type");
			if (type_it != tool_item.end() && type_it->is_string()) {
				tool_call.type = type_it->get<std::string>();
			}
			else {
				return nullptr;
			}
			auto function_it = tool_item.find("function");
			if (function_it != tool_item.end() && function_it->is_object()) {
				auto func_params_it = function_it->find("arguments");
				if (func_params_it != function_it->end() && func_params_it->is_string()) {
					tool_call.function_parameters.parameters = func_params_it->get<std::string>();
				}
				auto func_name_it = function_it->find("name");
				if (func_name_it != function_it->end() && func_name_it->is_string()) {
					tool_call.function_parameters.name = func_name_it->get<std::string>();
				}
			}
			msg_ptr->tool_calls.push_back(tool_call);
		}
	}
	return msg_ptr;
}

std::shared_ptr<ChatCompletionsChoice> ChatCompletionsChoice::Parse(json& input_json) {
	auto choice_ptr = std::make_shared<ChatCompletionsChoice>();

	auto index_it = input_json.find("index");
	if (index_it != input_json.end() && index_it->is_number_integer()) {
		choice_ptr->index = index_it->get<int>();
	}
	else {
		return nullptr;
	}
	auto finish_reason_it = input_json.find("finish_reason");
	if (finish_reason_it != input_json.end() && finish_reason_it->is_string()) {
		choice_ptr->finish_reason = finish_reason_it->get<std::string>();
	}
	else {
		return nullptr;
	}
	auto message_it = input_json.find("message");
	if (message_it != input_json.end() && message_it->is_object()) {
		auto msg_ptr = ChatCompletionsMessage::Parse(*message_it);
		if (msg_ptr == nullptr) {
			return nullptr;
		}
		choice_ptr->message = *msg_ptr;
	}
	else {
		return nullptr;
	}
	return choice_ptr;
}

std::string ChatCompletionsChoice::Dump() {
	json j;

	j["index"] = index;
	j["finish_reason"] = finish_reason;

	std::string msg_str = message.Dump();

	j["message"] = json::parse(msg_str);
	return j.dump();
}

std::string ChatCompletionsResponse::Dump() {
	json j;

	j["id"] = id;
	j["object"] = object;
	j["created"] = created;
	j["model"] = model_name;
	j["choices"] = json::array();

	for (auto& choice : choices) {
		std::string choice_str = choice.Dump();
		j["choices"].push_back(json::parse(choice_str));
	}
	return j.dump();
}

std::shared_ptr<ChatCompletionsResponse> ChatCompletionsResponse::Parse(json& input_json) {
	std::shared_ptr<ChatCompletionsResponse> resp_ptr = std::make_shared<ChatCompletionsResponse>();

	auto id_it = input_json.find("id");
	if (id_it != input_json.end() && id_it->is_string()) {
		resp_ptr->id = id_it->get<std::string>();
	} else {
		return nullptr;
	}
	auto object_it = input_json.find("object");
	if (object_it != input_json.end() && object_it->is_string()) {
		resp_ptr->object = object_it->get<std::string>();
	} else {
		return nullptr;
	}
	auto created_it = input_json.find("created");
	if (created_it != input_json.end() && created_it->is_number_integer()) {
		resp_ptr->created = created_it->get<int64_t>();
	}
	else {
		return nullptr;
	}
	auto model_it = input_json.find("model");
	if (model_it != input_json.end() && model_it->is_string()) {
		resp_ptr->model_name = model_it->get<std::string>();
	}
	else {
		return nullptr;
	}

	auto choices_it = input_json.find("choices");
	if (choices_it != input_json.end() && choices_it->is_array()) {
		for (auto& choice_item : *choices_it) {
			auto choice_ptr = ChatCompletionsChoice::Parse(choice_item);
			if (choice_ptr == nullptr) {
				continue;
			}
			resp_ptr->choices.push_back(*choice_ptr);
		}
	}
	else {
		return nullptr;
	}
	return resp_ptr;
}

json FunctionParameter::ToJson() const {
	json j;
	j["type"] = type;
	j["properties"] = json::object();

	for (const auto& prop : properties) {
		json jprop;
		jprop["type"] = prop.second.type;
		jprop["description"] = prop.second.description;

		j["properties"][prop.first] = jprop;
	}
	if (required_vec.empty()) {
		return j;
	}
	j["required"] = json::array();

	for (const auto& req : required_vec) {
		j["required"].push_back(req);
	}
	return j;
}

json FunctionDefinition::ToJson() const {
	json j;
	j["name"] = name;
	j["description"] = description;
	j["parameters"] = parameters.ToJson();
	return j;
}

json ToolDefinition::ToJson() const {
	json j;
	j["type"] = type;
	j["function"] = function.ToJson();
	return j;
}
