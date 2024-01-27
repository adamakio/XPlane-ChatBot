/**
 * @file ChatBot.cpp
 * @author zah
 * @brief Implementation file for chatbot that takes in audio and outputs streamed text
 * @see ChatBot.h
 * @version 0.1
 * @date 2023-10-20
 *
 */

#include "ChatBot.h" 

namespace XPlaneChatBot {
namespace Chat {


ChatBot::ChatBot() 
    : m_isListening(false)
    , m_transcriber(16'000)
{
    Base::Logger::log("Successfully initialized ChatBot", Base::LogLevel::INFO, __FUNCTION__);
}

ChatBot::~ChatBot() {
    stopListening();
}

void ChatBot::startListening(const MessageType& message_type) {
    if (m_isListening) {
        Base::Logger::log(
            "While already listening, start listening called",
            Base::ERR, __FUNCTION__
        );
        return;
    }

    std::shared_ptr<Message> message = std::make_shared<Message>(message_type); // not freed anywhere!
    m_transcriber.start_transcription(message);
    
    m_chatHistory.push_back(message);
    m_isListening.store(true);
}


void ChatBot::stopListening() {
    if (!m_isListening) {
        Base::Logger::log("Stop listening called while chatbot is not listening", Base::ERR, __FUNCTION__);
        return;
    }
    m_transcriber.stop_transcription();
    
    m_isListening.store(false);
}

bool ChatBot::isListening() const {
    return m_isListening;
}


void ChatBot::respond(const std::string& question, const std::string& context) {
    Json payload;
    if (context.empty()) {
        payload = {
            {"model", "ft:gpt-3.5-turbo-1106:further-protection::8Ik5e8WA"},
            {"messages", { // List of messages to feed to the chatbot, each one is a dictionary
                {{"role", "user"}, {"content", question}}
            }},
            {"max_tokens", 500},
            {"temperature", 0},
            {"stream", true}
        };
    }
    else {
        payload = {
            {"model", "ft:gpt-3.5-turbo-1106:further-protection::8Ik5e8WA"},
            {"messages", { // List of messages to feed to the chatbot, each one is a dictionary
                {{"role", "system"}, {"content", context}}, // TODO: Parametrize to match score from Score Manager
                {{"role", "user"}, {"content", question}}
            }},
            {"max_tokens", 500},
            {"temperature", 0},
            {"stream", true}
        };
    }
    Base::Logger::log("Constructed payload: " + payload.dump(2), Base::DEBUG, __FUNCTION__);

    std::shared_ptr<Message> message = std::make_shared<Message>(MessageType::AIGeneratedResponse);
    m_chatHistory.push_back(message);
    Base::Logger::log("Message added to chat history with type: " + messageTypeToString(message->getType()), Base::DEBUG, __FUNCTION__);

    chatThread = std::thread(
        [this](Json payload, std::shared_ptr<Message> msg) {
            openai::OpenAI openAI{}; // API key is set as environment variable OPENAI_API_KEY
            Base::Logger::log("OpenAI initialized", Base::DEBUG, __FUNCTION__);
            openAI.chat(payload.dump(), msg.get());
            Base::Logger::log("Chat method completed", Base::DEBUG, __FUNCTION__);
        }, payload, message
    );

    { // Text to speech threads
        
        // reset variables
        producerFinished = false;
        textAudioPairs.clear();

        producerThread = std::thread([this](std::shared_ptr<Message> message) {
            size_t played = 0;
            while (true) {
                if (played >= message->getUndisplayedText().length()) {
                    // Wait for more text to be added
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                std::string newText = message->getUndisplayedText().substr(played);
                size_t end_of_sentence_char = newText.find_first_of(".?!");
                if (end_of_sentence_char != std::string::npos) {
                    std::string tts_buffer = newText.substr(0, end_of_sentence_char + 1);
                    played += end_of_sentence_char + 1;

                    auto sharedData = std::make_shared<openai::SharedAudioData>();

                    openai::OpenAI openAI{};
                    openAI.textToSpeech(tts_buffer, sharedData.get());

                    std::lock_guard<std::mutex> lock(textAudioPairsMutex);
                    textAudioPairs.push_back({ tts_buffer, sharedData });
                }
                else if (!message->isUpdating()) {
                    producerFinished = true;
                    break;
                }
            }
        }, message
        );

        playerThread = std::thread([this]() {
            openai::OpusPlayer opusPlayer{}; ///< Player for playing audio
            while (true) {  
                std::shared_ptr<openai::SharedAudioData> audioData;
                {
                    std::lock_guard<std::mutex> lock(textAudioPairsMutex);
                    if (!textAudioPairs.empty()) {
                        audioData = textAudioPairs.front().audioData;
                        textAudioPairs.front().played = true;
                    }
                    else if (producerFinished) {
                        break;
                    }
                }

                if (audioData) {
                    opusPlayer.playAudio(audioData.get());
                }
            }
        });



        displayThread = std::thread([this](std::shared_ptr<Message> message) {
            constexpr int wordsPerMinute = 170;
            while (true) {
                std::string textToDisplay;
                {
                    std::lock_guard<std::mutex> lock(textAudioPairsMutex);
                    if (!textAudioPairs.empty())
                    {
                        if (textAudioPairs.front().played) {
                            textToDisplay = textAudioPairs.front().text;
                            textAudioPairs.erase(textAudioPairs.begin());
                        }
                    }
                    else if (producerFinished) {
                        break;
                    }
                }

                // Display text word by word
                if (!textToDisplay.empty()) {
                    std::istringstream iss(textToDisplay);
                    std::string word;
                    while (iss >> word) {
                        message->addWordToText(word + " ");

                        // Calculate display duration for each word
                        std::chrono::milliseconds duration((60 * 1000) / wordsPerMinute);
                        std::this_thread::sleep_for(duration);
                    }
                }
            }
            }, message
        );
    }
}

void ChatBot::joinResponseThreads() {
    if (!isFinishedResponding()) {
		Base::Logger::log("Joined response threads before completion", Base::ERR, __FUNCTION__);
        return;
    }
    if (producerThread.joinable()) {
		producerThread.join();
	}
	if (playerThread.joinable()) {
		playerThread.join();
	}
    if (chatThread.joinable()) {
        chatThread.join();
    }
    if (displayThread.joinable()) {
        displayThread.join();
    }
}

const bool ChatBot::isFinishedResponding() const {
	return (textAudioPairs.empty() && producerFinished) ;
}
                   

const std::vector<std::shared_ptr<Message>>& ChatBot::getChatHistory() {
    return m_chatHistory;
}

} // namespace Chat
} // namespace XPlaneChatBot