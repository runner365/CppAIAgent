#include "llm_http_client.h"

LLMHttpClient::LLMHttpClient(uv_loop_t* loop, const std::string& host, uint16_t port,
	const std::string& subpath,
	const std::string& model_name,
	const std::string& api_key, 
	const std::string& id,
	LLMResponseInterface* cb,
	Logger* logger)
{
	loop_ = loop;
	host_ = host;
	port_ = port;
	subpath_ = subpath;
	model_name_ = model_name;
	api_key_ = api_key;
	logger_ = logger;
	http_client_ = std::make_shared<HttpClient>(loop, host, port, this, logger, true);
	is_connected_ = false;;
	id_ = id;
	cb_ = cb;

	// Output initialization parameters to the log
	LogInfof(logger_, "LLMHttpClient parameters - host: %s, port: %d, subpath:%s, api_key: %s, module_name:%s, id:%s",
		host.c_str(), port, subpath.c_str(), api_key.c_str(), model_name.c_str(), id.c_str());
}

LLMHttpClient::~LLMHttpClient()
{
	Close();
}

void LLMHttpClient::OnHttpRead(int ret, std::shared_ptr<HttpClientResponse> resp_ptr)
{
	if (ret != 0) {
		LogErrorf(logger_, "HTTP read error: %d", ret);
		cb_->OnResponse(ret, "HTTP read error", id_, nullptr);
		return;
	}
	if (!resp_ptr) {
		LogErrorf(logger_, "HTTP response is null");
		cb_->OnResponse(-1, "HTTP response is null", id_, nullptr);
		return;
	}
	LogInfof(logger_, "HTTP Response Status: %s (%d)", resp_ptr->status_.c_str(), resp_ptr->status_code_);
	LogInfof(logger_, "HTTP Response Body: %s", std::string((char*)resp_ptr->data_.Data(), resp_ptr->data_.DataLen()).c_str());

	std::string resp_str((char*)resp_ptr->data_.Data(), resp_ptr->data_.DataLen());

	try {
		auto resp_json = json::parse(resp_str);
		auto chat_resp_ptr = ChatCompletionsResponse::Parse(resp_json);
		if (chat_resp_ptr) {
			LogInfof(logger_, "Parsed ChatCompletionsResponse: %s", chat_resp_ptr->Dump().c_str());
			cb_->OnResponse(0, "OK", id_, chat_resp_ptr);
		}
		else {
			LogErrorf(logger_, "Failed to parse ChatCompletionsResponse");
		}
	}
	catch (const std::exception& e) {
		LogErrorf(logger_, "JSON parse error: %s", e.what());
	}
}

int LLMHttpClient::SendPrompt(const std::list<ChatCompletionsMessage>& messages, const std::vector<ToolDefinition>& tools_definition)
{
	ChatCompletionsInfo info;

	info.model = model_name_;
	info.messages = messages;
	info.tools_definition = tools_definition;

	std::string json_payload = info.DumpJson();
	LogInfof(logger_, "Sending JSON Payload: %s", json_payload.c_str());

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "application/json";
	headers["Authorization"] = "Bearer " + api_key_;
	return http_client_->Post(subpath_, headers, json_payload);
}

void LLMHttpClient::Close() {

}