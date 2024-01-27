#ifndef IXTRANSCRIBER_H
#define IXTRANSCRIBER_H

#include "base/logger.h"
#include "ChatStructures.hpp"

#include "portaudio.h"
#include <nlohmann/json.hpp>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>
#include "XPLMProcessing.h"

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <deque>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <sstream>


namespace XPlaneChatBot {
    namespace Chat {
        using Json = nlohmann::json;

        class IXTranscriber
        {
        public:
            IXTranscriber(int sample_rate);
            ~IXTranscriber();

            void start_transcription(std::shared_ptr<Message>);
            void stop_transcription();

        private:
            static int pa_callback(
                const void* inputBuffer,
                void* outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags,
                void* userData
            );

            int on_audio_data(const void* inputBuffer, unsigned long framesPerBuffer);
            void on_message(const ix::WebSocketMessagePtr& msg);
            const bool isPauseDurationExceeded() const;

            std::shared_ptr<Message> m_message;
            ix::WebSocket m_webSocket;
            std::atomic<bool> m_running { false };

            std::string m_terminateMsg{Json({{"terminate_session", true}}).dump()};

            std::vector<char> m_audioDataBuffer;
            Json m_audioJSON{ {"audio_data", ""} };

            std::deque<std::vector<char>> m_audioQueue;
            std::mutex m_audioQueueMutex;

            std::mutex m_startStopMutex;

            PaStream* m_audioStream{ nullptr };
            PaError m_audioErr{ paNoError };

            const std::string m_aaiAPItoken{ "7e4983bb8d1d47acb2dec97ee5e4c3ed" };
            const int m_sampleRate;
            const int m_framesPerBuffer;
            const PaSampleFormat m_format{ paInt16 };
            const int m_channels{ 1 };

            // To track pause duration
            std::chrono::steady_clock::time_point m_pauseStartTime{ std::chrono::steady_clock::now() }; ///< Start time of the pause
            const std::chrono::seconds m_pauseThreshold{ 2 }; ///< Threshold for pause duration (2 seconds)
        };

    } // namespace Chat
} // namespace XPlaneChatBot

#endif // IXTRANSCRIBER_H


