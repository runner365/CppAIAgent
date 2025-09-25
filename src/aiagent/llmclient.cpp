#include "llmclient.h"
#include <thread>

bool LLMClient::init_ = false;
const size_t MAX_RECENT_MESSAGES = 100;

LLMClient::LLMClient(uv_loop_t* loop, const std::string& model_name, const std::string& host, uint16_t port, const std::string& api_key, const std::string& subpath, Logger* logger)
	: TimerInterface(loop, 200)
	, loop_(loop)
	, model_(model_name)
	, host_(host)
	, port_(port)
	, api_key_(api_key)
	, subpath_(subpath)
	, logger_(logger)
{
	StartTimer();

	uv_async_init(loop_, &async_, &LLMClient::UVAsyncCallback);
	async_.data = this;
	llm_tool_ptr_.reset(new LLMTool(logger_));
	//LogInfof输出所有参数
	LogInfof(logger_, "LLMClient initializing with model: %s, host: %s, port: %d, api_key: %s, subpath: %s",
		model_name.c_str(), host.c_str(), port, api_key.c_str(), subpath.c_str());
}

LLMClient::~LLMClient()
{
	model_clients_.clear();
	LogInfof(logger_, "LLMClient destroyed");
}

void LLMClient::Init(uv_loop_t* loop, Logger* logger)
{
	if (init_) {
		return;
	}
	init_ = true;

	std::thread run_uv_loop_thread([loop, logger]() {
		LogInfof(logger, "Starting UV loop thread");
		uv_run(loop, UV_RUN_DEFAULT);
		LogInfof(logger, "UV loop thread exiting");
		std::cout << "UV loop thread exiting" << std::endl;
		exit(0);
		});
	run_uv_loop_thread.detach();
	return;
}

void LLMClient::OnTimer() {
	while (remove_id_queue_.size() > 0) {
		std::string id = remove_id_queue_.front();
		remove_id_queue_.pop();
		auto it = model_clients_.find(id);
		if (it != model_clients_.end()) {
			model_clients_.erase(it);
			LogInfof(logger_, "Removed LLMHttpClient for id: %s", id.c_str());
		}
	}
}

void LLMClient::AddRecentMessage(const ChatCompletionsMessage & message) {
	std::lock_guard<std::mutex> lock(mutex_);
	recent_messages_.push_back(message);

	if (recent_messages_.size() > MAX_RECENT_MESSAGES) {
		recent_messages_.pop_front();
	}
}

std::list<ChatCompletionsMessage>  LLMClient::GetRecentMessages() {
	std::lock_guard<std::mutex> lock(mutex_);
	return recent_messages_;
}

void LLMClient::SendPrompt(const std::string& id, const std::string& prompt) {
	std::string message = prompt;
	message += ", response without markdown and without Emoji";
	AddPromptToQueue(id, message);
	uv_async_send(&async_);
}

void LLMClient::OnSendPrompt(const std::string& id, const std::string& prompt) {
	std::shared_ptr<LLMHttpClient> client_ptr = std::make_shared<LLMHttpClient>(loop_, host_, port_,
		subpath_, model_, api_key_, id, this, logger_);

	model_clients_[id] = client_ptr;

	AddRecentMessage(ChatCompletionsMessage{ "user", prompt });

	client_ptr->SendPrompt(GetRecentMessages(), llm_tool_ptr_->GetToolDefinitions());
}

void LLMClient::UVAsyncCallback(uv_async_t* handle) {
	LLMClient* client = static_cast<LLMClient*>(handle->data);
	if (client) {
		client->OnAsyncCallback();
	}
}

void LLMClient::OnAsyncCallback() {
	std::pair<std::string, std::string> prompt_pair = GetPromptFromQueue();
	if (prompt_pair.first.empty() || prompt_pair.second.empty()) {
		LogErrorf(logger_, "No prompt in queue");
		return;
	}
	OnSendPrompt(prompt_pair.first, prompt_pair.second);
}

void LLMClient::AddPromptToQueue(const std::string& id, const std::string& prompt) {
	std::lock_guard<std::mutex> lock(prompt_mutex_);
	prompt_queue_.emplace_back(id, prompt);
}

std::pair<std::string, std::string> LLMClient::GetPromptFromQueue() {
	std::lock_guard<std::mutex> lock(prompt_mutex_);
	if (prompt_queue_.empty()) {
		return std::make_pair("", "");
	}
	auto prompt = prompt_queue_.front();
	prompt_queue_.pop_front();
	return prompt;
}

void LLMClient::OnResponse(int code, const std::string& err_msg, const std::string& id, std::shared_ptr<ChatCompletionsResponse> resp_ptr) {
	LogInfof(logger_, "OnResponse called with code: %d, err_msg: %s, id: %s", code, err_msg.c_str(), id.c_str());

	if (resp_ptr) {
		LogInfof(logger_, "Received response for id: %s, response: %s", id.c_str(), resp_ptr->Dump().c_str());
		if (resp_ptr->choices.size() > 0) {
			for (const auto& choice : resp_ptr->choices) {
				if (choice.message.role == "assistant") {
					AddRecentMessage(choice.message);
					if (!choice.message.tool_calls.empty()) {
						for (const auto& tool_call : choice.message.tool_calls) {
							std::string call_id = tool_call.id;
							std::string func_name = tool_call.function_parameters.name;
							std::string params_str = tool_call.function_parameters.parameters;
							ToolFunction func = llm_tool_ptr_->GetTool(func_name);
							if (func) {
								std::map<std::string, LLMValue> params_map;
								if (params_str.size() > 0) {
									try {
										auto params_json = json::parse(params_str);

										if (params_json.is_object()) {
											for (auto it = params_json.begin(); it != params_json.end(); ++it) {
												//get iter key
												std::string key = it.key();

												LLMValue val;
												if (it.value().is_string()) {
													val.type = LLMValue::LLM_VALUE_STRING;
													val.string_value = it.value().get<std::string>();
												}
												else if (it.value().is_number()) {
													val.type = LLMValue::LLM_VALUE_NUMBER;
													val.number_value = it.value().get<double>();
												}
												else if (it.value().is_boolean()) {
													val.type = LLMValue::LLM_VALUE_BOOL;
													val.bool_value = it.value().get<bool>();
												}
												else {
													val.type = LLMValue::LLM_VALUE_NULL;
												}
												params_map.emplace(key, val);
											}
										}
										else {
											LogErrorf(logger_, "Function parameters is not a JSON object: %s", params_str.c_str());
										}
									}
									catch (const std::exception& e) {
										LogErrorf(logger_, "Failed to parse function parameters JSON: %s", e.what());
									}
								}

								FunctionResult func_result = func(params_map, logger_);
								
								std::string id = call_id;
								std::shared_ptr<LLMHttpClient> client_ptr = std::make_shared<LLMHttpClient>(loop_, host_, port_,
									subpath_, model_, api_key_, id, this, logger_);

								model_clients_[id] = client_ptr;

								ChatCompletionsMessage tool_msg;
								tool_msg.role = "tool";

								if (func_result.code != 0) {
									tool_msg.content = "Error: " + func_result.desc;
								}
								else
								{
									tool_msg.content = func_result.value.string_value;
									tool_msg.tool_call_id = call_id;
								}

								AddRecentMessage(tool_msg);

								client_ptr->SendPrompt(GetRecentMessages(), llm_tool_ptr_->GetToolDefinitions());
							}
							else {
								LogErrorf(logger_, "No tool function found for name: %s", func_name.c_str());
							}
						}
					}
					else {
						InsertRespQueue(code, err_msg, id, resp_ptr);
					}
					break;
				}
			}
		}
		else {
			LogErrorf(logger_, "Response choices are empty for id: %s", id.c_str());
		}
	}
	else {
		LogErrorf(logger_, "Received null response for id: %s", id.c_str());
	}

	remove_id_queue_.push(id);
}

void LLMClient::InsertRespQueue(int code, const std::string& err_msg, const std::string& id, std::shared_ptr<ChatCompletionsResponse> resp_ptr) {
	std::lock_guard<std::mutex> lock(resp_mutex_);

	ResponseTuple resp_tuple{
		code,
		err_msg,
		id,
		resp_ptr
	};
	response_queue_.push(resp_tuple);
}

bool LLMClient::GetRespQueue(ResponseTuple& resp_tuple) {
	std::lock_guard<std::mutex> lock(resp_mutex_);
	if (response_queue_.empty()) {
		return false;
	}
	resp_tuple = response_queue_.front();
	response_queue_.pop();

	return true;
}

void LLMClient::AddFunctionTool(const std::string& name, const ToolDefinition& def, ToolFunction func) {
	llm_tool_ptr_->AddToolDefinition(def);
	llm_tool_ptr_->AddTool(name, func);

	LogInfof(logger_, "Added function tool: %s, tool definition:%s", 
		name.c_str(), def.ToJson().dump().c_str());
}

const std::vector<ToolDefinition>& LLMClient::GetToolDefinitions() const {
	return llm_tool_ptr_->GetToolDefinitions();
}