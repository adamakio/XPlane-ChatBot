/**
 * @file ChatBot.h
 * @author zah
 * @brief Header for ChatBot class that handles transcription using Transcriber 
 *  
 * This header file defines the SpeechToText class, which is responsible for handling transcription using Python and AssemblyAI.
 * It provides methods for starting and stopping transcription, as well as checking if transcription is currently running.
 *  
 * @version 0.1
 * @date 2023-10-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef XPROTECTION_CHAT_CHATBOT_H
#define XPROTECTION_CHAT_CHATBOT_H

#include "defs.h"
#include "base/logger.h"
#include "chatbot/IXTranscriber.h"
#include "chatbot/ChatStructures.hpp"
#include "chatbot/openai.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <XPLMUtilities.h>
#include <XPLMProcessing.h>

#include <imgui.h>
#include <Windows.h>

namespace XPlaneChatBot {
	namespace Chat {
		/// @brief Class that handles chatting functionality
		class ChatBot : public std::enable_shared_from_this<ChatBot> {
		public:
			/**
			 * @brief Constructor for ChatBot class: initializes member variables and starts connections with OpenAI
			 * + Initializes the pointers to the transcript handler and stream response
			 * + Initializes isListening to false 
			 * + Initializes map from filename to list of words
			 * 
			 */
			ChatBot();

			

			/**
			 * @brief Destructor for ChatBot class: stops listening to free the Transcriber
			 */
			~ChatBot();
			

			/**
			 * @brief Initializes the Transcriber and starts transcription
			 * + Sets m_isListening to true
			 * + Popultes the m_transcriptHandler with partial and final transcripts
			 * 
			 * @param message_type The type of message to listen for (see Message.h)
			 */
			void startListening(const MessageType& message_type = MessageType::UserTranscription);

			/**
			 * @brief Stops transcription and frees the Transcriber (automatically since it is a unique_ptr)
			 * + Sets m_isListening to false
			 * + Sets m_transcriptHandler->activity.level to PAUSE
			 */
			void stopListening(); 

			/**
			 * @brief Getter for listening status
			 * @return bool True if the chatbot is listening to audio
			 */
			bool isListening() const; 

			/**
			 * @brief Respond to a message using OpenAI
			 * @param question The question to respond to
			 * @param context The context to respond in (if any)
			 */
			void respond(const std::string& question, const std::string& context = "");

			/**
			 * @brief Joins the response threads 
			 */
			void joinResponseThreads();

			/**
			 * @brief Check if the chatbot is finished responding
			 * @return true if chatbot is finished responding
			 */
			const bool isFinishedResponding() const;

			/**
			 * @brief Getter for the chat history
			 * @return const std::vector<Message>& The chat history
			 */
			const std::vector<std::shared_ptr<Message>>& getChatHistory();

		private:

			// Transcription related
			IXTranscriber m_transcriber; ///< Transcriber for transcribing audio in real time
			std::atomic<bool> m_isListening; ///< True if the chatbot is listening to user

			// Response related
			std::thread chatThread; ///< Thread for responding to messages
			std::vector<openai::TextAudioPair> textAudioPairs; ///< Vector of text-audio pairs to be played
			std::mutex textAudioPairsMutex; ///< Mutex for textAudioPairs to prevent concurrent access
			std::atomic<bool> producerFinished{ false }; ///< True if the producer thread has finished
			std::thread producerThread; ///< Thread for producing audio
			std::thread playerThread; ///< Thread for playing audio
			std::thread displayThread; ///< Thread for displaying messages

			// ChatBots "memory"
			std::vector<std::shared_ptr<Message>> m_chatHistory; ///< Custom data structure for storing chat history (see CircularBuffer.h)
		};
		
	} // namespace Chat
} // namespace XPlaneChatBot

#endif // XPROTECTION_CHAT_CHATBOT_H
