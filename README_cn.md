[中文](README_cn.md) | [English](README.md)

# CppAIAgent

CppAIAgent 是一个基于现代 C++20 开发的 AI Agent 平台，支持可扩展的工具开发。项目以 OpenCV 图像编辑为例，展示了如何在 C++ 中集成自然语言处理与图像处理能力，旨在为 C++ 开发者提供 AI Agent 的开发基础。

## 项目简介

CppAIAgent 允许用户通过自然语言描述与 AI Agent 交互，Agent 会自动解析用户输入并完成相应的图像编辑任务。该项目为 C++ 开发者扩展和定制 AI Agent 提供了平台和示例。

## 主要特性

- **工具化架构**：可轻松添加各类 AI 工具，扩展功能。
- **OpenCV 集成**：强大的图像处理与编辑能力。
- **自然语言接口**：用户可直接用语言描述所需编辑操作。
- **平台可扩展**：便于集成更多 AI 模型或服务，持续扩展。

## 内置工具列表

| 工具名称                              | 功能描述                                         |
|---------------------------------------|--------------------------------------------------|
| `get_current_weather`                 | 获取指定地点的当前天气信息                        |
| `convert_color_img_to_gray_img`       | 将彩色图片转换为灰度图，并进行边缘检测            |
| `apply_beauty_filter`                 | 对图片应用美颜滤镜，使其更美观                    |
| `apply_cartoon_filter`                | 对图片应用卡通滤镜，使其具有卡通效果              |
| `apply_sun_glasses`                   | 给图片中的人物佩戴太阳镜                         |
| `convert_image_to_cyberpunk_style`    | 将图片转换为赛博朋克风格                         |

## 使用方法

1. 使用 Visual Studio 2022 或任意支持 C++20 的编译器编译项目。
2. 在环境变量中设置你的 LLM API 密钥（`LLM_API_KEY`）。
3. 运行可执行文件，通过自然语言输入与 Agent 交互。
4. Agent 会自动处理请求并完成相应的图像编辑操作。

## 依赖要求

- C++20 编译器（已在 Visual Studio 2022 测试）
- OpenCV 库
- 可用的 LLM API 接口

## 许可证

本项目采用 MIT License 开源协议发布。