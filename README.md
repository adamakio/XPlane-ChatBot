# X-Plane ChatBot Plugin

Welcome to the repository for the X-Plane 12 speech-to-speech chatbot plugin. This plugin is designed to enhance the simulation experience by providing a fully interactive chatbot that users can interact with using their voice. The plugin is developed using modern C++ and integrates with the X-Plane environment seamlessly.

## Structure

Here's a brief overview of the codebase:

- `plugin.cc`: This is the main entry point of the plugin.
- `xplane-chatbot.h` and `xplane-chatbot.cpp`: These files define the Plugin class that encapsulates the chatbot functionality.
- `base/`: This directory contains the Logger class, which aids in outputting log information.
- `chatbot/`: This directory is the heart of the chatbot system.
    - `chatbot.h` and `chatbot.cpp`: These files manage the chat functionality and orchestrate the chatbot operations.
    - `openai.hpp`: A header-only file providing OpenAI functionalities.
    - `ChatStructures.hpp`: A header-only file that defines the data structures required by the chatbot.
    - `IXTranscriber.h` and `IXTranscriber.cpp`: These files contain a class that implements speech-to-text conversion using the Assembly AI and the IXWebSocket package.
- `ui/`: This directory houses the user interface components.
    - `FloatingWindow`: A class to create a floating window within the X-Plane interface.
    - `ImWindow`: Inherits from `FloatingWindow` and integrates Dear ImGui functionality for enhanced UI experience.

## Requirements

To build and run this plugin, you will need the following:

- X-Plane SDK (configured with all necessary preprocessors)
- PortAudio for audio streaming and capture
- Zlib for compression-related operations
- MbedTLS for secure communication
- Opus and libogg for audio encoding and decoding
- IXWebSocket for real-time web socket communication
- Dear ImGui for user interface rendering
- Curl for transferring data with URLs
- Nlohmann-JSON for handling JSON data

Please ensure all dependencies are correctly installed and configured before attempting to build the plugin.

## Contributions

Contributions to this project are welcome. If you have suggestions, bug reports, or contributions, please open an issue or a pull request.

## License

The X-Plane ChatBot Plugin is open-source and distributed under the MIT license.

Enjoy enhancing your X-Plane experience with interactive speech capabilities!