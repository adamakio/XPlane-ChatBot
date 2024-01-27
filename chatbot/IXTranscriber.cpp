#include "IXTranscriber.h"

namespace XPlaneChatBot {
namespace Chat {


IXTranscriber::IXTranscriber(int sample_rate)
    : m_sampleRate(sample_rate)
    , m_framesPerBuffer(static_cast<int>(sample_rate * 0.2f))
{
    // WebSocket initialization
    ix::initNetSystem(); // For windows

    // PortAudio initialization
    try {
        m_audioErr = Pa_Initialize();
        if (m_audioErr != paNoError) {
            Base::Logger::log("PortAudio error when initializing: " + std::string(Pa_GetErrorText(m_audioErr)), Base::ERR, __FUNCTION__);
            return;
        }

        m_audioErr = Pa_OpenDefaultStream(
            &m_audioStream,
            m_channels,
            0,
            m_format,
            m_sampleRate,
            m_framesPerBuffer,
            &IXTranscriber::pa_callback,
            this
        );
        if (m_audioErr != paNoError) {
            Base::Logger::log("PortAudio error when opening stream: " + std::string(Pa_GetErrorText(m_audioErr)), Base::ERR, __FUNCTION__);
            Pa_Terminate();
            return;
        }
    }
    catch (const std::exception& e) {
		Base::Logger::log(std::string("Exception in IXTranscriber constructor: ") + e.what(), Base::ERR, __FUNCTION__);
		throw;
	}
}

IXTranscriber::~IXTranscriber() {
    if (m_running)
        stop_transcription();
    if (m_audioStream) {
        Pa_CloseStream(m_audioStream);
    }
    Pa_Terminate();
    ix::uninitNetSystem();
}


void IXTranscriber::start_transcription(std::shared_ptr<Message> message) {
    std::lock_guard<std::mutex> lock(m_startStopMutex);
    if (m_running) {
		Base::Logger::log("Transcription already started.", Base::ERR, __FUNCTION__);
		return;
	}
    if (m_webSocket.getReadyState() != ix::ReadyState::Closed) {
        Base::Logger::log("Connection must be closed to start transcription", Base::ERR, __FUNCTION__);
        return;
    }

    m_message = message;

    m_audioErr = Pa_StartStream(m_audioStream);
    if(m_audioErr != paNoError) {
        Base::Logger::log("PortAudio error when starting stream: " + std::string(Pa_GetErrorText(m_audioErr)), Base::ERR, __FUNCTION__);
        Pa_CloseStream(m_audioStream);
        Pa_Terminate();
        m_audioStream = nullptr;
        return;
    }
    Base::Logger::log("PortAudio stream started", Base::DEBUG, __FUNCTION__);

    // Set up WebSocket URL and headers
    std::string url = "wss://api.assemblyai.com/v2/realtime/ws?sample_rate=" + std::to_string(m_sampleRate);
    m_webSocket.setExtraHeaders({ {"Authorization", m_aaiAPItoken} });
    m_webSocket.setUrl(url);

    // Initialize WebSocket
    m_webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        this->on_message(msg);
        });

    // Start WebSocket
    m_webSocket.start();
    m_webSocket.enableAutomaticReconnection();

    // Per message deflate connection is enabled by default. You can tweak its parameters or disable it
    m_webSocket.disablePerMessageDeflate();
    
    // Optional heart beat, sent every 20 seconds when there is not any traffic
    // to make sure that load balancers do not kill an idle connection.
    m_webSocket.setPingInterval(20);

    m_running = true;
    if (m_message->getType() == MessageType::UserTranscription)
        m_pauseStartTime = std::chrono::steady_clock::now();

    Base::Logger::log("Transcription started", Base::INFO, __FUNCTION__);
}

void IXTranscriber::stop_transcription() {
    std::lock_guard<std::mutex> lock(m_startStopMutex);
    if (!m_running) {
        Base::Logger::log("Transcription already stopped.", Base::ERR, __FUNCTION__);
        return;
    }

    // Stop portaudio stream
    m_audioErr = Pa_IsStreamActive(m_audioStream);
    if (m_audioErr == 1) {
        m_audioErr = Pa_StopStream(m_audioStream);
        if (m_audioErr != paNoError) {
            Base::Logger::log("PortAudio error when stopping stream: " + std::string(Pa_GetErrorText(m_audioErr)), Base::ERR, __FUNCTION__);
        }
    }

    ix::WebSocketSendInfo sendInfo = m_webSocket.send(m_terminateMsg);
    if (sendInfo.success) {
        Base::Logger::log("Terminate message sent successfully", Base::DEBUG, __FUNCTION__);
    }
    else {
		Base::Logger::log("Terminate message sending failed", Base::ERR, __FUNCTION__);
	}
    m_webSocket.stop();   

    m_running = false;
}


int IXTranscriber::pa_callback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    try {
        Base::Logger::log("Portaudio callback called", Base::DEBUG, __FUNCTION__);
        auto* transcriber = static_cast<IXTranscriber*>(userData);
        int code = transcriber->on_audio_data(inputBuffer, framesPerBuffer);
        return code;
    }
    catch (const std::exception& e) {
        Base::Logger::log(std::string("Exception in PortAudio callback: ") + e.what(), Base::ERR, __FUNCTION__);
        throw;
    }
}

int IXTranscriber::on_audio_data(const void* inputBuffer, unsigned long framesPerBuffer) 
{
    if (!m_running || m_webSocket.getReadyState() != ix::ReadyState::Open) {
       Base::Logger::log("Audio data received while not running", Base::WARN, __FUNCTION__);
       return paContinue;
    }

    // Cast data passed through stream to our structure.
    const auto* in = static_cast<const int16_t*>(inputBuffer);
    m_audioDataBuffer.assign(reinterpret_cast<const char*>(in), reinterpret_cast<const char*>(in) + framesPerBuffer * m_channels * sizeof(int16_t)); // Copy the audio data into the buffer

    m_audioJSON["audio_data"] = base64_encode(m_audioDataBuffer);
    Base::Logger::log("Audio data prepared", Base::DEBUG, __FUNCTION__);
    ix::WebSocketSendInfo sendInfo = m_webSocket.send(m_audioJSON.dump());
    if (sendInfo.success) {
        Base::Logger::log("Audio data sent successfully", Base::DEBUG, __FUNCTION__);
    }
    else {
        Base::Logger::log("Audio data sending failed", Base::ERR, __FUNCTION__);
    }
    return paContinue;
}


void IXTranscriber::on_message(const ix::WebSocketMessagePtr& msg) {
    try {
        switch (msg->type) {
        case ix::WebSocketMessageType::Message: {
            // Parse the JSON message
            Json json_msg = Json::parse(msg->str);
            Base::Logger::log("Received message: " + json_msg.dump(2), Base::DEBUG, __FUNCTION__);

            if (json_msg.contains("error")) {
                Base::Logger::log("Error from websocket: " + json_msg["error"], Base::ERR, __FUNCTION__);
                return;
            }

            // Extract the message type
            std::string message_type = json_msg["message_type"];

            if (message_type == "PartialTranscript") {
                std::string text = json_msg["text"].get<std::string>();
                if (!text.empty()) {
                    Base::Logger::log("Partial message received: " + text, Base::DEBUG, __FUNCTION__);
                    m_message->setPartialTranscript(text);
                    if (m_message->getType() == MessageType::UserTranscription)
                        m_pauseStartTime = std::chrono::steady_clock::now();
                }
                else if (m_message->getType() == MessageType::UserTranscription && m_message->receivedFinal()) {
                    if (isPauseDurationExceeded()) {
                        Base::Logger::log("Pause duration exceeded threshold", Base::DEBUG, __FUNCTION__);
                        m_message->stopUpdating();
                    }
                }
            }
            else if (message_type == "FinalTranscript") {
                std::string text = json_msg["text"].get<std::string>();
                if (!text.empty()) {
                    Base::Logger::log("Final message received: " + text, Base::DEBUG, __FUNCTION__);
                    m_message->setFinalTranscript(text);
                    if (m_message->getType() == MessageType::UserTranscription)
                        m_pauseStartTime = std::chrono::steady_clock::now();
                }
            }
            else if (message_type == "SessionBegins") {
                std::string session_id = json_msg["session_id"];
                std::string expires_at = json_msg["expires_at"];
                if (m_message->getType() == MessageType::UserTranscription)
                    m_pauseStartTime = std::chrono::steady_clock::now();
                Base::Logger::log("Session started with ID: " + session_id + " and expires at: " + expires_at, Base::INFO, __FUNCTION__);
            }
            else if (message_type == "SessionTerminated") {
                Base::Logger::log("Session terminated.", Base::INFO, __FUNCTION__);
            }
            else {
                Base::Logger::log("Unknown message type: " + message_type, Base::ERR, __FUNCTION__);
            }
            return;
        }
        case ix::WebSocketMessageType::Open:
            Base::Logger::log("WebSocket connection opened with message " + msg->str, Base::INFO, __FUNCTION__);
            Base::Logger::log(
                "Connection to " + msg->openInfo.uri
                + " opened with protocol: " + msg->openInfo.protocol
                + " and handshake headers:",
                Base::INFO, __FUNCTION__
            );
            for (auto it : msg->openInfo.headers) {
                Base::Logger::log(it.first + ": " + it.second, Base::INFO, __FUNCTION__);
            }
            if (m_message->getType() == MessageType::UserTranscription)
                m_pauseStartTime = std::chrono::steady_clock::now();
            return;
        case ix::WebSocketMessageType::Close:
            Base::Logger::log("WebSocket connection closed with message " + msg->str, Base::INFO, __FUNCTION__);
            Base::Logger::log(
                "Closing connection because error code " + std::to_string(msg->closeInfo.code)
                + " : " + msg->closeInfo.reason + "(remote ? " + std::to_string(msg->closeInfo.remote) + ")",
                Base::INFO, __FUNCTION__
            );
            return;
        case ix::WebSocketMessageType::Error:
            Base::Logger::log("WebSocket error with message " + msg->str, Base::INFO, __FUNCTION__);
            std::stringstream ss;
            ss << "Error: " << msg->errorInfo.reason;
            ss << "\n#retries: " << msg->errorInfo.retries;
            ss << "\nWait time(ms): " << msg->errorInfo.wait_time;
            ss << "\nHTTP Status: " << msg->errorInfo.http_status;
            Base::Logger::log(ss.str(), Base::ERR, __FUNCTION__);
            return;
        }
    }
    catch (const std::exception& e) {
		Base::Logger::log(std::string("Exception in WebSocket: ") + e.what(), Base::ERR, __FUNCTION__);
		throw;
	}
}

const bool IXTranscriber::isPauseDurationExceeded() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - m_pauseStartTime) >= m_pauseThreshold;
}

} // namespace Chat
} // namespace XPlaneChatBot



