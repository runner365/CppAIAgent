English | [中文](README_cn.md)

# CppAIAgent

CppAIAgent is an AI agent platform developed in modern C++20, designed to support extensible tool development. It demonstrates how to build AI-powered applications in C++ by integrating natural language processing and image editing capabilities, using OpenCV as a practical example.

## Project Overview

CppAIAgent enables users to interact with the AI agent using natural language descriptions. The agent interprets user input and performs image editing tasks automatically. This project serves as a foundation for C++ developers to create and extend AI agents with custom tools.

## Features

- **Tool-based architecture:** Easily add new tools for various AI tasks.
- **OpenCV integration:** Powerful image processing and editing capabilities.
- **Natural language interface:** Users can describe desired edits in plain language.
- **Extensible platform:** Designed for further development and integration with other AI models or services.

## Included Tools

| Tool Name                        | Description                                                                 |
|-----------------------------------|-----------------------------------------------------------------------------|
| `get_current_weather`             | Get the current weather in a given location                                 |
| `convert_color_img_to_gray_img`   | Convert a color image to a grayscale image and perform edge detection        |
| `apply_beauty_filter`             | Apply a beauty filter to an image to make it more beautiful                 |
| `apply_cartoon_filter`            | Apply a cartoon filter to an image to make it look like a cartoon           |
| `apply_sun_glasses`               | Apply sun glasses to a person in the image                                  |
| `convert_image_to_cyberpunk_style`| Convert an image to cyberpunk style                                         |

## Usage

1. **Build the project** using Visual Studio 2022 or any C++20-compatible compiler.
2. **Set your LLM API key** in the environment variable `LLM_API_KEY`.
3. **Run the executable** and interact with the agent by entering natural language commands.
4. The agent will process your request and perform the corresponding image editing operation.


## Requirements

- C++20 compiler (tested with Visual Studio 2022)
- OpenCV library
- Access to a compatible LLM API

## License

This project is released under the MIT License.

