
#include "llmclient.h"
#include "function_tools.h"
#include "llm_tool.h"
#include "utils/url.h"
#include "utils/logger.hpp"

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>
#include <iomanip>

using namespace cv;

using namespace cpp_streamer;

void OnReceiveMessageFromLLM(std::shared_ptr<LLMClient> llm_client_ptr) {
	while (true) {
		ResponseTuple resp;
		bool ret = llm_client_ptr->GetRespQueue(resp);
		if (!ret) {
			//sleep 1sec			
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		int code = std::get<0>(resp);
		if (code != 0) {
			std::string err_msg = std::get<1>(resp);
			std::cout << "\nAI: handle error, code:" << code << ", error:" << err_msg << "\r\n";
			continue;
		}
		std::string id = std::get<2>(resp);
		std::shared_ptr< ChatCompletionsResponse> resp_ptr = std::get<3>(resp);
		if (!resp_ptr) {
			std::cout << "\nAI: response is null\n";
			continue;
		}
		if (resp_ptr->choices.empty()) {
			std::cout << "\nAI: response messages is null\r\n";
			continue;
		}
		for (auto choice : resp_ptr->choices) {
			if (choice.message.role == "assistant") {
				// std::wstring utf8_content = Utf8ToWstring(choice.message.content);
				std::cout << "\n\nAI: " << choice.message.content << "\r\n";

				size_t pos = choice.message.content.find("bye");

				if (pos != std::string::npos) {
					exit(0);
				}
				pos = choice.message.content.find("Bye");
				if (pos != std::string::npos) {
					exit(0);
				}
				break;
			}
		}
	}
	std::cout << "end receiving message from llm\r\n";
}

void ToolsInit(std::shared_ptr<LLMClient> llm_client_ptr) {
	auto weather_def = CreateWeatherFunctionDefinition();
	auto convert_colorimg_to_grayimg_def = ConvertColorImg2GrayImgFunctionDefinition();
	auto makeup_def = ApplyBeautyFilterFunctionDefinition();
	auto cartoon_def = ApplyCartoonFilterFunctionDefinition();
	auto sun_glasses_def = ApplySunGlassesFunctionDefinition();
	auto cyber_def = ConvertImage2CyberPunkStyleFunctionDefinition();

	llm_client_ptr->AddFunctionTool(weather_def.function.name, weather_def, GetWeather);
	llm_client_ptr->AddFunctionTool(convert_colorimg_to_grayimg_def.function.name, convert_colorimg_to_grayimg_def, ConvertColorImg2GrayImgTool);
	llm_client_ptr->AddFunctionTool(makeup_def.function.name, makeup_def, ApplyBeautyFilterTool);
	llm_client_ptr->AddFunctionTool(cartoon_def.function.name, cartoon_def, ApplyCartoonFilterTool);
	llm_client_ptr->AddFunctionTool(sun_glasses_def.function.name, sun_glasses_def, ApplySunGlassesTool);
	llm_client_ptr->AddFunctionTool(cyber_def.function.name, cyber_def, ConvertImage2CyberPunkStyleTool);

	const auto& tool_defs = llm_client_ptr->GetToolDefinitions();
	std::cout << "Registered Tools:" << std::endl;
	size_t max_name_len = 0;
	for (const auto & def : tool_defs) {
		if (def.function.name.length() > max_name_len) {
			max_name_len = def.function.name.length();
		}
	}

	for (const auto& def : tool_defs) {

		std::cout << "tool:"
			<< std::left << std::setw(max_name_len) << def.function.name
			<< " desc:" << def.function.description
			<< "\r\n";
	}
	std::cout << "\r\n";
}

// Configure terminal for UTF-8 output (cross-platform)
void SetupTerminal() {
#ifdef _WIN32
	// Windows: Force console to use UTF-8 for input/output
	if (!SetConsoleCP(CP_UTF8)) {  // Set input code page to UTF-8
		throw std::runtime_error("Failed to set console input code page to UTF-8");
	}
	if (!SetConsoleOutputCP(CP_UTF8)) {  // Set output code page to UTF-8
		throw std::runtime_error("Failed to set console output code page to UTF-8");
	}
#else
	// Linux/macOS: Ensure system locale uses UTF-8
	setlocale(LC_ALL, "en_US.UTF-8");
#endif
}

int main(int argc, char** argv) {
	//std::string llmUrl = "https://api.hunyuan.cloud.tencent.com/v1/chat/completions";
	//std::string model_name = "hunyuan-turbo";
	std::string llmUrl = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
	std::string model_name = "qwen-plus";

	SetupTerminal();

	std::cout << "OpenCV version:" << CV_VERSION << std::endl;
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

	bool isHttps = false;
	std::string host;
	uint16_t port = 0;
	std::string subpath;

	bool parse_ok = ParseUrl(llmUrl, isHttps, host, port, subpath);
	
	if (!parse_ok) {
		std::cout << "ParseUrl failed, invalid url:" << llmUrl << std::endl;
		return -1;
	}

	char* api_key_env = nullptr;
	size_t len = 0;
	errno_t err = _dupenv_s(&api_key_env, &len, "LLM_API_KEY");
	if (err != 0 || api_key_env == nullptr) {
		std::cout << "Failed to retrieve LLM_API_KEY from environment variables." << std::endl;
		return -1;
	}

	std::shared_ptr<Logger> logger_ptr = std::make_shared<Logger>("aiagent.log", LOGGER_INFO_LEVEL);
	logger_ptr->DisableConsole();
	
	LogInfof(logger_ptr.get(), "llm url:%s, host:%s, port:%d, subpath:%s, key:%s",
		llmUrl.c_str(), host.c_str(), port, subpath.c_str(), api_key_env);
	LLMClient::Init(uv_default_loop(), logger_ptr.get());

	std::shared_ptr<LLMClient> llm_client_ptr = std::make_shared<LLMClient>(uv_default_loop(), model_name, host, port, api_key_env, subpath, logger_ptr.get());

	ToolsInit(llm_client_ptr);

	std::thread resp_thread(OnReceiveMessageFromLLM, llm_client_ptr);
	resp_thread.detach();

	uint64_t index = 0;
	while (true) {
		// Prompt for input
		std::cout << "user: ";

		// Read wide string input to handle Unicode characters
		std::wstring w_input;
		if (!std::getline(std::wcin, w_input)) {
			std::cerr << "Error reading input" << std::endl;
			break;
		}
		std::cout << "\r\n";

		if (w_input.empty()) {
			continue;
		}
		// Check if user wants to quit
		if (w_input == L"quit" || w_input == L"exit") {
			std::cout << "Exiting program..." << std::endl;
			break;
		}

		try {
			// Convert wide string to UTF-8 encoded std::string
			std::string u8_input = WStringToUtf8(w_input);

			std::string id = "session_" + std::to_string(index++);
			llm_client_ptr->SendPrompt(id, u8_input);
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		}
		catch (const std::runtime_error& e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}
	return 0;
}
