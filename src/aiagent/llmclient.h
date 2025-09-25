#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include "uv.h"
#include "llm_http_client.h"
#include "llm_info.h"
#include "llm_tool.h"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>
#include <memory>
#include <mutex>

using namespace cpp_streamer;

// int code, std::string err_msg, std::string id, std::shared_ptr< ChatCompletionsResponse>
using ResponseTuple = std::tuple<int, std::string, std::string, std::shared_ptr< ChatCompletionsResponse>>;

class LLMClient : public TimerInterface, public LLMResponseInterface
{
public:
	LLMClient(uv_loop_t* loop, const std::string& model_name, const std::string& host, uint16_t port, const std::string& api_key, const std::string& subpath, Logger* logger);
	~LLMClient();
	
public:
	static void Init(uv_loop_t* loop, Logger* logger);
	static void UVAsyncCallback(uv_async_t* handle);

public:
	virtual void OnResponse(int code, const std::string& err_msg, const std::string& id, std::shared_ptr<ChatCompletionsResponse> resp_ptr) override;

public:
	// Send a prompt to the LLM and receive a response
	void SendPrompt(const std::string& id, const std::string& prompt);
	bool GetRespQueue(ResponseTuple& resp_tuple);
	void AddFunctionTool(const std::string& name, const ToolDefinition& def, ToolFunction func);
	const std::vector<ToolDefinition>& GetToolDefinitions() const;

protected:
	virtual void OnTimer() override;

private:
	void AddRecentMessage(const ChatCompletionsMessage& message);
	std::list<ChatCompletionsMessage> GetRecentMessages();
	void OnAsyncCallback();
	void AddPromptToQueue(const std::string& id,const std::string& prompt);
	std::pair<std::string, std::string> GetPromptFromQueue();
	void OnSendPrompt(const std::string& id, const std::string& prompt);

private:
	void InsertRespQueue(int code, const std::string& err_msg, const std::string& id, std::shared_ptr<ChatCompletionsResponse>);

private:
	std::mutex mutex_;
	std::mutex prompt_mutex_;
	std::mutex resp_mutex_;

private:
	static bool init_;

private:
	uv_loop_t* loop_ = nullptr;
	std::string model_;
	std::string host_;
	uint16_t port_ = 0;
	std::string api_key_;
	std::string subpath_;
	Logger* logger_ = nullptr;

private:
	std::list<std::pair<std::string, std::string>> prompt_queue_;
	std::map<std::string, std::shared_ptr<LLMHttpClient>> model_clients_; // key: session_id, value: shared_ptr<LLMHttpClient>
	std::list<ChatCompletionsMessage> recent_messages_;
	std::queue<std::string> remove_id_queue_;
	std::queue<ResponseTuple> response_queue_;

private:
	std::unique_ptr<LLMTool> llm_tool_ptr_;
private:
	uv_async_t async_;
};

#endif
