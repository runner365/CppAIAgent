#ifndef LLM_HTTP_CLIENT_H
#define LLM_HTTP_CLIENT_H
#include "http_client.hpp"
#include "utils/logger.hpp"
#include "llm_info.h"
#include "llm_tool.h"

#include "uv.h"
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <list>

using namespace cpp_streamer;

class LLMHttpClient : public HttpClientCallbackI
{
public:
	LLMHttpClient(uv_loop_t* loop, const std::string& host, uint16_t port,
		const std::string& subpath,
		const std::string& model_name,
		const std::string& api_key,
		const std::string& id,
		LLMResponseInterface* cb,
		Logger* logger);
	virtual ~LLMHttpClient();

protected:
	virtual void OnHttpRead(int ret, std::shared_ptr<HttpClientResponse> resp_ptr) override;

public:
	int SendPrompt(const std::list< ChatCompletionsMessage>& messages, const std::vector<ToolDefinition>& tools_definition);;
	void Close();
	std::string GetId() const { return id_; }
	void SetId(const std::string& id) { id_ = id; }

private:
	uv_loop_t* loop_ = nullptr;
	std::string host_;
	uint16_t port_ = 0;
	std::string subpath_;
	std::string model_name_;
	std::string api_key_;
	Logger* logger_ = nullptr;
	std::shared_ptr<HttpClient> http_client_ = nullptr;
	bool is_connected_ = false;
	std::string pending_request_; // Store the request if not connected

private:
	std::string id_; // Unique identifier for the request
	LLMResponseInterface* cb_ = nullptr;
};
#endif