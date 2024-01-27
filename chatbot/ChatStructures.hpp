#ifndef XPROTECTION_CHAT_STRUCTURES_H
#define XPROTECTION_CHAT_STRUCTURES_H

#include <string>
#include <chrono>
#include <vector>
#include <atomic>

#include "base/logger.h"

#include <nlohmann/json.hpp>
#include <XPLMProcessing.h>

namespace XPlaneChatBot {
	namespace Chat {
		using Json = nlohmann::json;

		/// @brief Struct that contains a word and its start time for the cached response
		struct Word {
			std::string text; ///< Text of the word
			long long start; ///< Start time of the word

			/// @brief Constructor for the Word struct
			/// @param t Text of the word
			/// @param s Start time of the word
			Word(std::string t, long long s) : text(t), start(s) {}
		};

		/// @brief Enum for the type of message
		enum class MessageType {
			CachedSelect, ///< Cached message upon selecting a maneuver  (followed by "studentAssertingControl" message)
			CachedBegin, ///< Cached message after student asserts control (followed by "CachedWarn" or "CachedFatal" or "CachedSuccess" message)
			CachedWarn, ///< Cached message for warning the student (followed by "CachedWarn" or "CachedFatal" or "CachedSuccess" message)
			CachedFatal, ///< Cached message for fatal error  (followed by "studentRelinquishingControl" message)
			CachedSuccess, ///< Cached message for success (followed by "studentRelinquishingControl" message)
			Cached, ///< Default cached message (also separates cached from non-cached messages) 

			AIGeneratedResponse, ///< AI generated response (followed by "UserTranscription")
			None, ///< Default message type


			UserTranscription, ///< User transcript -> (followed by "AIGeneratedResponse" message)
			studentRelinquishingControl, ///< Student relinquished control (followed by "AIGeneratedResponse" message)
			studentAssertingControl, ///< Student asserted control (followed by "CachedBegin" message)
		};

		static bool isCached(const MessageType& message_type) {
			return message_type <= MessageType::Cached;
		}

		static bool isUser(const MessageType& message_type) {
			return message_type >= MessageType::UserTranscription;
		}

		static bool isAI(const MessageType& message_type) {
			return message_type == MessageType::AIGeneratedResponse;
		}

		static const std::string messageTypeToString(const MessageType& type) {
			switch (type) {
			case MessageType::UserTranscription: return "UserTranscription";
			case MessageType::AIGeneratedResponse: return "AIGeneratedResponse";
			case MessageType::studentAssertingControl: return "studentAssertingControl";
			case MessageType::studentRelinquishingControl: return "studentRelinquishingControl";
			case MessageType::CachedWarn: return "CachedWarn";
			case MessageType::CachedBegin: return "CachedBegin";
			case MessageType::CachedSelect: return "CachedSelect";
			case MessageType::CachedFatal: return "CachedFatal";
			case MessageType::CachedSuccess: return "CachedSuccess";
			case MessageType::Cached: return "Cached";
			default: return "Unknown";
			}
		}

		class Message {
		public:
			// Constructors
			Message(MessageType type) : m_type(type), m_lastUpdated(std::chrono::system_clock::now()) {}

			Message(MessageType type, const std::vector<Word>& words)
				: m_type(type), m_lastUpdated(std::chrono::system_clock::now()), m_words(words)
			{
				if (isCached(type)) {
					m_wordsStartTime = std::chrono::steady_clock::now();
					m_lastProcessedWordIndex = 0; // Reset the index as we are starting afresh

					// Start the callback loop for updating words
					if (m_flightLoopID != nullptr) {
						Base::Logger::log("Flight loop already exists.", Base::ERR, __FUNCTION__);
						return;
					}
					XPLMCreateFlightLoop_t params;
					params.structSize = sizeof(params);
					params.phase = xplm_FlightLoop_Phase_AfterFlightModel;
					params.callbackFunc = &Message::updateWordsCallback;
					params.refcon = this;

					m_flightLoopID = XPLMCreateFlightLoop(&params);

					XPLMScheduleFlightLoop(m_flightLoopID, 0.01f, true);
					Base::Logger::log("Message type is cached, updating words", Base::DEBUG, __FUNCTION__);

					m_isUpdating = true;
				}
				else {
					Base::Logger::log("Message type is not cached.", Base::ERR, __FUNCTION__);
				}
			}

			// Transcription methods

			void setPartialTranscript(const std::string& transcript) {
				m_partialTranscript = transcript;
				m_text = m_finalTranscript + m_partialTranscript;
				m_lastUpdated = std::chrono::system_clock::now();
			}

			void setFinalTranscript(const std::string& transcript) {
				m_finalTranscript += transcript;
				m_partialTranscript = "";
				m_text = m_finalTranscript;
				m_lastUpdated = std::chrono::system_clock::now();
				if (m_type == MessageType::UserTranscription) { return; }
				if (studentAssertedControl(transcript)) {
					Base::Logger::log("Student asserted control", Base::DEBUG, __FUNCTION__);
					m_isUpdating = false;
				}
				if (studentRelinquishedControl(transcript)) {
					Base::Logger::log("Student relinquished control", Base::DEBUG, __FUNCTION__);
					m_isUpdating = false;
				}
			}

			bool receivedFinal() const noexcept {
				return !m_finalTranscript.empty();
			}

			bool studentAssertedControl(const std::string& transcript) {
				return (
					m_type == MessageType::studentAssertingControl
					&&
					transcript.find("i have control") != std::string::npos
					|| transcript.find("i have the control") != std::string::npos
					|| transcript.find("i have the flight controls") != std::string::npos
					|| transcript.find("i have the flight control") != std::string::npos
					|| transcript.find("i have the controls") != std::string::npos
					);
			}

			bool studentRelinquishedControl(const std::string& transcript) {
				return (
					m_type == MessageType::studentRelinquishingControl
					&&
					transcript.find("you have control") != std::string::npos
					|| transcript.find("you have the control") != std::string::npos
					|| transcript.find("you have the flight controls") != std::string::npos
					|| transcript.find("you have the flight control") != std::string::npos
					|| transcript.find("you have the controls") != std::string::npos
					);
			}

			// Cached message methods
			static float updateWordsCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
			{
				Message* message = static_cast<Message*>(inRefcon);
				Base::Logger::log("Updating words.", Base::DEBUG, __FUNCTION__);
				return message->updateWords();
			}

			float updateWords() {
				if (m_words.empty()) {
					Base::Logger::log("Words list is empty", Base::WARN, __FUNCTION__);
					stopUpdating();
					return 0.0f; // Stop the callback if words list is empty or null
				}

				if (m_lastProcessedWordIndex >= m_words.size()) {
					stopUpdating();
					return 0.0f; // Stop the callback when all words are processed
				}

				long long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - m_wordsStartTime
				).count();

				while (m_lastProcessedWordIndex < m_words.size()) {
					const auto& word = m_words.at(m_lastProcessedWordIndex);
					if (elapsedTime < word.start) {
						// Calculate time until the next word needs to be processed
						long long timeUntilNextWord = word.start - elapsedTime;
						return static_cast<float>(timeUntilNextWord) / 1000.0f; // Convert milliseconds to seconds for the callback interval
					}
					m_text.append(word.text + " ");
					m_lastProcessedWordIndex++;
				}

				return -1.0f; // Continue the callback, adjust the time interval as needed
			}

			/// @brief Set the response from the API
			void setAIResponse(const std::string& data) {
				if (!isAI(this->m_type)) {
					Base::Logger::log("Message type is not AI generated response: " + messageTypeToString(this->m_type), Base::ERR, __FUNCTION__);
					return;
				}
				Base::Logger::log(">> response: " + data + "\n", Base::INFO, __FUNCTION__);

				// Append new data to the buffer
				m_buffer += data;

				// Try to find a complete JSON object
				size_t startPos = m_buffer.find("data: ");
				while (startPos != std::string::npos) {
					size_t endPos = m_buffer.find("\n\n", startPos); // Assuming "\n\n" is the delimiter between events
					if (endPos == std::string::npos) {
						// If we don't have the complete JSON object, break and wait for more data
						break;
					}

					// Extract the JSON object
					std::string json_data = m_buffer.substr(startPos + 6, endPos - (startPos + 6)); // Remove "data: " prefix
					m_buffer.erase(0, endPos + 2); // Remove the processed object from the buffer

					// Parse the JSON data
					Json parsed;
					try {
						parsed = Json::parse(json_data);
					}
					catch (std::exception&) {
						// If parsing fails, it's not a valid JSON, so we continue to the next object
						startPos = m_buffer.find("data: ", endPos);
						continue;
					}

					// Process the parsed JSON as before
					for (const auto& choice : parsed["choices"]) {
						if (choice.contains("delta") && choice["delta"].contains("content")) {
							std::string chunk = choice["delta"]["content"].get<std::string>();
							Base::Logger::log(">> chunk: " + chunk, Base::INFO, __FUNCTION__);
							// TODO: lock
							m_textToDisplay += chunk;
						}
						if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
							m_isUpdating = false;
						}
					}
					if (!m_isUpdating) {
						Base::Logger::log("Finished updating: " + messageTypeToString(m_type), Base::DEBUG, __FUNCTION__);
						break;
					}

					// Look for the next JSON object
					startPos = m_buffer.find("data: ");
				}
			}

			void stopUpdating() {
				if (!m_isUpdating) {
					Base::Logger::log("Transcript not updating.", Base::DEBUG, __FUNCTION__);
					return; // If the thread is not running, nothing to do
				}
				m_isUpdating = false;
				Base::Logger::log("Stopped updating: " + messageTypeToString(m_type), Base::DEBUG, __FUNCTION__);
				if (m_flightLoopID != nullptr) {
					XPLMDestroyFlightLoop(m_flightLoopID);
					m_flightLoopID = nullptr;
				}
			}

			// Getters
			MessageType getType() const { return m_type; }
			std::string getUndisplayedText() const { 
				// TODO: lock
				return m_textToDisplay; 
			}
			std::string getText() const { return m_text; }
			std::chrono::system_clock::time_point getLastUpdated() const { return m_lastUpdated; }
			bool isUpdating() const { return m_isUpdating; }

			// Setters
			void addWordToText(const std::string& text) { m_text += text; } // Only for AI generated response

		private:
			// General fields
			MessageType m_type{ MessageType::None }; ///< Type of message
			std::string m_text{ "" }; ///< Text of the message
			std::string m_textToDisplay{ "" }; ///< Text of the message to display
			std::mutex m_textMutex; ///< Mutex for the text
			std::chrono::system_clock::time_point m_lastUpdated; ///< Latest timestamp of the message
			std::atomic<bool> m_isUpdating{ true }; ///< Flag to indicate whether the message is being updated

			// Fields specific to transcription
			std::string m_partialTranscript{ "" };
			std::string m_finalTranscript{ "" };

			// Fields specific to cached messages
			std::vector<Word> m_words{}; ///< Vector of words in the cached message
			std::chrono::steady_clock::time_point m_wordsStartTime{}; ///< Start time of the words
			size_t m_lastProcessedWordIndex{ 0 }; ///< Index of the last processed word

			// Fields specific to AI generated response
			std::string m_buffer{ "" }; ///< Buffer to hold incomplete JSON data

			// For callback loops
			XPLMFlightLoopID m_flightLoopID{ nullptr }; ///< Flight loop for updating AI Generated Response

		};

	} // namespace Chat
} // namespace XPlaneChatBot
#endif // XPROTECTION_CHAT_STRUCTURES_H
