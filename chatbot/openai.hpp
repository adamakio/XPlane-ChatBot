#ifndef XP_OPENAI_HPP_
#define XP_OPENAI_HPP_

// Required standard libraries
#include <string>
#include <queue>
#include <iostream>
#include <stdexcept>

// Thread synchronization libraries
#include <mutex>
#include <condition_variable>

// Curl communication libraries
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// Audio processing libraries
#include <ogg/ogg.h>
#include <opus/opus.h>
#include <portaudio.h>

// Project headers
#include "ChatStructures.hpp"

namespace XPlaneChatBot {
namespace openai {
    // Define constants for audio settings
    constexpr int SAMPLE_RATE = 24000;
    constexpr int CHANNELS = 1;
    constexpr int FRAMES_PER_BUFFER = 960;

    class SharedAudioData; // Forward declaration of SharedAudioData class

    /// @brief Struct that contains the text and audio data for a message
    struct TextAudioPair {
        std::string text; ///< Text of the message
        std::shared_ptr<SharedAudioData> audioData; ///< Audio data of the message
        bool played = false;  ///< Flag to indicate if the audio has begun playing
    };

    class SharedAudioData {

    public:
        // Constructor
        SharedAudioData() : dataReady(false), opusDecoder(nullptr), opusError(OPUS_OK), oggInitialized(false), serial_number(-1) {
            // Initialize the Ogg sync state
            ogg_sync_init(&oy);
        }

        // Destructor
        ~SharedAudioData() {
            if (opusDecoder) {
                opus_decoder_destroy(opusDecoder);
                opusDecoder = nullptr;
            }
            if (oggInitialized) {
                ogg_stream_clear(&os);
                ogg_sync_clear(&oy);
                oggInitialized = false;
            }
        }

        // Initialize the Ogg stream state; should be called when a new stream is detected
        void initOggStream(int serial) {
            if (ogg_stream_init(&os, serial) != 0) {
                throw std::runtime_error("Failed to initialize Ogg stream state.");
            }
            serial_number = serial;
            oggInitialized = true;
        }

        // Reset the Ogg stream state; should be called for a new logical stream
        void resetStreamAndDecoder() {
            if (oggInitialized) {
                ogg_stream_clear(&os);
                ogg_sync_clear(&oy);
                oggInitialized = false;
            }
            if (opusDecoder) {
                opus_decoder_destroy(opusDecoder);
                opusDecoder = nullptr;
                initOpusDecoder();  // Re-initialize the Opus decoder for the next playback
            }
            else {
                initOpusDecoder();
            }
            serial_number = -1;  // Reset serial number for the new stream
        }


        void initOpusDecoder() {
            if (!opusDecoder) {
                opusDecoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opusError);
                if (opusError != OPUS_OK) {
                    throw std::runtime_error("Failed to create Opus decoder: " + std::string(opus_strerror(opusError)));
                }
            }
        }

        void processData(void* ptr, size_t size) {
            // Buffer to store the incoming Ogg data
            char* buffer = ogg_sync_buffer(&oy, static_cast<long>(size));
            memcpy(buffer, ptr, size);
            ogg_sync_wrote(&oy, static_cast<long>(size));

            // Process the Ogg pages and extract Opus packets
            while (ogg_sync_pageout(&oy, &og) == 1) {
                if (!oggInitialized || serial_number == -1) {
                    serial_number = ogg_page_serialno(&og);
                    initOggStream(serial_number);
                }

                if (ogg_stream_pagein(&os, &og) != 0) {
                    Base::Logger::log("Failed to read Ogg page into stream.", Base::ERR, __FUNCTION__);
                }

                while (ogg_stream_packetout(&os, &op) == 1) {
                    // Check for header packets
                    if (op.bytes >= 19 && strncmp(reinterpret_cast<char*>(op.packet), "OpusHead", 8) == 0) {
                        Base::Logger::log("OpusHead header found.", Base::DEBUG, __FUNCTION__);
                        // The OpusHead header format:
                        // - "OpusHead" (8 bytes)
                        // - Version number (1 byte)
                        // - Channel count (1 byte)
                        // - Pre-skip (2 bytes)
                        // - Sample rate (4 bytes)
                        // - Output gain (2 bytes)
                        // - Channel mapping (1 byte)
                        unsigned char version = op.packet[8];
                        unsigned char channel_count = op.packet[9];
                        unsigned short pre_skip = *reinterpret_cast<unsigned short*>(op.packet + 10);
                        unsigned int sample_rate = *reinterpret_cast<unsigned int*>(op.packet + 12);
                        short output_gain = *reinterpret_cast<short*>(op.packet + 16);
                        unsigned char channel_mapping = op.packet[18];

                        Base::Logger::log("Version number: " + std::to_string(static_cast<unsigned int>(version)), Base::DEBUG, __FUNCTION__);
                        Base::Logger::log("Channel count: " + std::to_string(static_cast<unsigned int>(channel_count)), Base::DEBUG, __FUNCTION__);
                        Base::Logger::log("Pre-skip: " + std::to_string(pre_skip), Base::DEBUG, __FUNCTION__);
                        Base::Logger::log("Sample rate: " + std::to_string(sample_rate), Base::DEBUG, __FUNCTION__);
                        Base::Logger::log("Output gain: " + std::to_string(output_gain), Base::DEBUG, __FUNCTION__);
                        Base::Logger::log("Channel mapping: " + std::to_string(static_cast<unsigned int>(channel_mapping)), Base::DEBUG, __FUNCTION__);

                        continue; // Skip decoding the header packet
                    }

                    if (op.bytes >= 16 && strncmp(reinterpret_cast<char*>(op.packet), "OpusTags", 8) == 0) {
                        Base::Logger::log("OpusTags header found.", Base::DEBUG, __FUNCTION__);
                        // The OpusTags header format:
                        // - "OpusTags" (8 bytes)
                        // - Vendor string length (4 bytes)
                        // - Vendor string (variable length)
                        unsigned int vendor_length = *reinterpret_cast<unsigned int*>(op.packet + 8);
                        std::string vendor_string(reinterpret_cast<char*>(op.packet + 12), vendor_length);

                        Base::Logger::log("Vendor string: " + vendor_string, Base::DEBUG, __FUNCTION__);
                        // You might want to include more processing here to read comments
                        continue; // Skip decoding the tag packet
                    }


                    // Decode the Opus packet
                    float decodedPCM[FRAMES_PER_BUFFER * CHANNELS];
                    int frameSize = opus_decode_float(opusDecoder, op.packet, op.bytes, decodedPCM, FRAMES_PER_BUFFER, 0);
                    if (frameSize < 0) {
                        Base::Logger::log("Opus decoding error: " + std::string(opus_strerror(frameSize)), Base::ERR, __FUNCTION__);
                        continue;
                    }

                    addData(decodedPCM, frameSize * CHANNELS);
                    dataReady = true;
                }
            }
        }

        void addData(const float* data, size_t size) {
            std::lock_guard<std::mutex> lock(audioMutex);
            for (size_t i = 0; i < size; ++i) {
                audioBuffer.push(data[i]);
            }
        }

        size_t getData(float* output, size_t framesPerBuffer) {
            std::lock_guard<std::mutex> lock(audioMutex);
            size_t i = 0;
            for (; i < framesPerBuffer && !audioBuffer.empty(); ++i) {
                output[i] = audioBuffer.front();
                audioBuffer.pop();
            }
            return i; // Number of frames read
        }

        // Getters
        bool isDataReady() const { return dataReady; }
        bool isEndOfData() const { return endOfData; }

        // Setters
        void setDataReady(bool ready) { dataReady = ready; }
        void signalEndOfData() { endOfData = true; }
    private:
        std::atomic<bool> dataReady{ false };
        std::atomic<bool> endOfData{ false };
        std::queue<float> audioBuffer;
        std::mutex audioMutex;

        ogg_sync_state oy;          // Ogg sync state, for syncing with the Ogg stream
        ogg_stream_state os;        // Ogg stream state, for handling logical streams
        ogg_page og;                // Ogg page, a single unit of data in an Ogg stream
        ogg_packet op;              // Ogg packet, contains encoded Opus data
        OpusDecoder* opusDecoder;   // Opus decoder state
        int opusError;              // Error code returned by Opus functions
        bool oggInitialized;        // Flag to track if Ogg and Opus have been initialized
        int serial_number;          // Serial number for the Ogg stream
    };


    class OpusPlayer {
    public:
        OpusPlayer() {
            m_err = Pa_Initialize();
            if (m_err != paNoError) {
                Base::Logger::log("PortAudio stream initialization error: " + std::string(Pa_GetErrorText(m_err)), Base::ERR, __FUNCTION__);
            }
        }

        ~OpusPlayer() {
            m_err = Pa_Terminate();
            if (m_err != paNoError) {
                Base::Logger::log("PortAudio stream termination error: " + std::string(Pa_GetErrorText(m_err)), Base::ERR, __FUNCTION__);
            }
        }

        void playAudio(SharedAudioData* shared_data) {
            // Open PortAudio stream
            m_err = Pa_OpenDefaultStream(&m_stream, 0, CHANNELS, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, audioCallback, shared_data);
            if (m_err != paNoError) {
                Base::Logger::log("PortAudio stream open error: " + std::string(Pa_GetErrorText(m_err)), Base::ERR, __FUNCTION__);
                return;
            }

            // Start PortAudio stream for playback
            m_err = Pa_StartStream(m_stream);
            if (m_err != paNoError) {
                Base::Logger::log("PortAudio stream start error: " + std::string(Pa_GetErrorText(m_err)), Base::ERR, __FUNCTION__);
                Pa_CloseStream(m_stream);
                return;
            }

            while (Pa_IsStreamActive(m_stream) == 1) {
				Pa_Sleep(1000);
			}

            // Stop and close PortAudio stream
            m_err = Pa_StopStream(m_stream);
            if (m_err != paNoError) {
                Base::Logger::log("PortAudio stream stop error: " + std::string(Pa_GetErrorText(m_err)), Base::ERR, __FUNCTION__);
                return;
            }
        }

    private:
        PaStream* m_stream{ nullptr };
        PaError m_err{ paNoError };

        // Define PortAudio callback function to play audio
        static int audioCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData) {
            SharedAudioData* sharedData = static_cast<SharedAudioData*>(userData);

            float* out = static_cast<float*>(outputBuffer);
            std::fill(out, out + framesPerBuffer * CHANNELS, 0.0f); // Fill buffer with silence

            if (!sharedData->isDataReady()) {
                // No data available yet, just play silence
                return paContinue;
            }

            size_t bytesRead = sharedData->getData(out, framesPerBuffer * CHANNELS);
            if (bytesRead < framesPerBuffer * CHANNELS) {
                // Buffer underflow, not enough data available
                sharedData->setDataReady(false); // Wait for more data
                if (sharedData->isEndOfData()) {
                    return paComplete; // End of data
                }
            }

            return paContinue;
        }

    };


    /**
    * @brief Class to handle the curl session
    */
    class Session {
    public:
        /// @brief Construct a new Session object by initializing curl and setting the options
        Session() {
            // Initialize curl
            curl_global_init(CURL_GLOBAL_ALL);// || CURL_VERSION_THREADSAFE);

            curl_ = curl_easy_init();
            if (curl_ == nullptr) {
                Base::Logger::log("OpenAI curl_easy_init() failed", Base::ERR, __FUNCTION__);
            }

            // Ignore SSL
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        }

        /// @brief Destroy the Session object by cleaning up curl
        ~Session() {
            curl_easy_cleanup(curl_);
            curl_global_cleanup();
        }

        /// @brief Set the url to make the request to
        void setUrl(const std::string& url) { url_ = url; }

        /// @brief Set the token to use for authentication
        void setToken(const std::string& token) {
            token_ = token;
        }

        /// @brief Set the body of the request to send
        void setBody(const std::string& data) {
            if (curl_) {
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.length());
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
            }
        }

        template <typename Data>
        bool makeRequest(Data* data, size_t(*writeFunc)(void*, size_t, size_t, void*)) {
            std::lock_guard<std::mutex> lock(mutex_request_);

            auto headers = createHeaders();
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers.get());
            curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
            curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunc);
            curl_easy_setopt(curl_, CURLOPT_WRITEDATA, data);

            return curl_easy_perform(curl_) == CURLE_OK;
        }

        /// @brief Callback function to write the audio response to the SharedAudioData object
        static size_t writeBinaryData(void* ptr, size_t size, size_t nmemb, void* stream) {
            // Get real size
            size_t realSize = size * nmemb;
            SharedAudioData* sharedData = static_cast<SharedAudioData*>(stream);
            sharedData->processData(ptr, realSize);
            return realSize;
        }

        /// @brief Callback function to write the response to our Message object
        static size_t writeStreamData(void* ptr, size_t size, size_t nmemb, void* message) {
            size_t realsize = size * nmemb;
            std::string text((char*)ptr, realsize);
            Chat::Message* msg = static_cast<Chat::Message*>(message);
            msg->setAIResponse(text);
            return size * nmemb;
        }

    private:
        struct CurlHeadersDeleter {
            void operator()(curl_slist* headers) {
                curl_slist_free_all(headers);
            }
        };

        std::unique_ptr<curl_slist, CurlHeadersDeleter> createHeaders() {
            std::unique_ptr<curl_slist, CurlHeadersDeleter> headers(nullptr, CurlHeadersDeleter{});
            headers.reset(curl_slist_append(headers.release(), std::string{ "Authorization: Bearer " + token_ }.c_str()));
            headers.reset(curl_slist_append(headers.release(), "Content-Type: application/json"));
            return headers;
        }

        CURL* curl_; ///< The curl session
        CURLcode    res_; ///< The curl result
        std::string url_; ///< The url to make the request to
        std::string token_; ///< The token to use for authentication
        std::mutex  mutex_request_; ///< Mutex to avoid concurrent requests
    };

    /// @brief Class to handle the OpenAI API
    class OpenAI {
    public:
        /// @brief Construct a new OpenAI object
        /// @param token The token to use for authentication (optional if set as environment variable (OPENAI_API_KEY)
        OpenAI(const std::string& token = "")
            : token_{ token } {
            if (token.empty()) { // If no token is provided, try to get it from the environment variable
                char* value = NULL;
                size_t size = 0;
                errno_t err = _dupenv_s(&value, &size, "OPENAI_API_KEY");
                if (!err && value) {
                    token_ = std::string{ value }; // Set the token from the environment variable
                    free(value); // Don't forget to free the allocated memory
                }
                else { // If the environment variable is not set, log an error
                    Base::Logger::log("OPENAI_API_KEY environment variable not set", Base::ERR, __FUNCTION__);
                }
            }
            session_.setToken(token_);
        }

        OpenAI(const OpenAI&) = delete;
        OpenAI& operator=(const OpenAI&) = delete;
        OpenAI(OpenAI&&) = delete;
        OpenAI& operator=(OpenAI&&) = delete;

        bool post(const std::string& suffix, const std::string& data, SharedAudioData* shared_data = nullptr, Chat::Message* msg = nullptr) {
            auto complete_url = base_url + suffix;
            session_.setUrl(complete_url);
            session_.setBody(data);
            Base::Logger::log("<< request: " + complete_url + "  " + data, Base::DEBUG, __FUNCTION__);
            if (shared_data) {
                return session_.makeRequest(shared_data, &Session::writeBinaryData);
            }
            else if (msg) {
                return session_.makeRequest(msg, &Session::writeStreamData);
            }
            else {
                Base::Logger::log("No file path or message provided", Base::ERR, __FUNCTION__);
                return false;
            }
        }

        bool chat(const std::string& input, Chat::Message* message) {
            bool success = post("chat/completions", input, nullptr, message);
            if (!success) {
				Base::Logger::log("Chat request failed", Base::ERR, __FUNCTION__);
                return false;
			}
            return true;
        }

        bool textToSpeech(const std::string& text, SharedAudioData* shared_data) {
            shared_data->initOpusDecoder();

            // Prepare the data for the TTS request
            nlohmann::json data;
            data["input"] = text; // Set the input text for the TTS request
            data["model"] = "tts-1-hd"; // Set the model to use for the TTS request
            data["voice"] = "alloy"; // Set the voice to use for the TTS request
            data["response_format"] = "opus"; // Set the response format to use for the TTS request
            data["speed"] = 1.0f; // Set the speed to use for the TTS request (could be setting)

            std::string dataStr = data.dump();
            bool success = post("audio/speech", dataStr, shared_data);
            shared_data->signalEndOfData();
            if (!success) {
				Base::Logger::log("TTS request failed", Base::ERR, __FUNCTION__);
				return false;
			}
            return true;
        }

    private:
        Session session_;
        std::string token_;
        std::string organization_;
        std::string base_url = "https://api.openai.com/v1/";
    };


} // namespace openai
} // namespace XPlaneChatBot
#endif // XP_OPENAI_HPP_
