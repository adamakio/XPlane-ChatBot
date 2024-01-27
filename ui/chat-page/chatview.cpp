/**
 * @file chatview.cpp
 * @author zah
 * @brief Implementation file of ChatView class
 * @version 0.1
 * @date 2023-10-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "chatview.h"


namespace XPlaneChatBot {
    namespace Ui {
        using namespace std::chrono_literals;

        ChatView::ChatView(std::shared_ptr<Chat::ChatBot> chatBot)
            : ImWindow(350, 400, xplm_WindowDecorationRoundRectangle, false, true, true, true)
            , m_chatBot(chatBot)
            , m_errorMessage("")
            , m_fontSize(ImGui::GetFontSize())
            , m_active(false)
        {
            setTitle(std::string("XPlaneChatBot - Chat").c_str());
        }

        void ChatView::doBuild()
        {
            using namespace Chat;

            try {
                if (!m_errorMessage.empty()) {
                    ImGui::Text("The following error occurred during chat: ");
                    ImGui::TextWrapped(m_errorMessage.c_str());
                    ImGui::Separator();
                    return;
                }

                const auto& chat_history = m_chatBot->getChatHistory();

                // Show visual indicator of whether the chatbot is listening or speaking
                ImVec2 windowSize = ImGui::GetWindowSize();
                if (!chat_history.empty()) {
                    const auto& latest_message = chat_history.back();

                    // Status Text and Indicator
                    auto status_message = getStatusMessage(latest_message->getType());
                    auto status_color = getStatusColor(latest_message->getType());

                    ImGui::Text(status_message.c_str());
                    ImGui::SameLine(windowSize.x - m_fontSize - ImGui::GetStyle().ItemSpacing.x);
                    ImGui::ColorButton("##status", status_color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(m_fontSize, m_fontSize));
                    ImGui::Separator();

                    // Handling message response and listening logic
                    if (latest_message->getType() == MessageType::AIGeneratedResponse && m_chatBot->isFinishedResponding() && !m_chatBot->isListening()) {
                        m_chatBot->joinResponseThreads();
                        m_chatBot->startListening(MessageType::UserTranscription);
                    }
                    else if (latest_message->getType() == MessageType::UserTranscription && !latest_message->isUpdating() && m_chatBot->isListening()) {
                        m_chatBot->stopListening();
                        m_chatBot->respond(latest_message->getText());
                    }

                    ImGui::Columns(2, "MessageColumns");
                    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.15f);

                    for (const auto& message : chat_history) {
                        std::string text = message->getText();
                        Chat::MessageType type = message->getType();
                        if (text.empty()) { continue; }

                        ImGui::PushStyleColor(ImGuiCol_Text, getTextColor(type)); // Push must be accompanied by Pop
                        ImGui::Text(isUser(type) ? "YOU: " : "VFI: ");
                        ImGui::NextColumn();
                        ImGui::TextWrapped(text.c_str());
                        ImGui::NextColumn();
                        ImGui::PopStyleColor(); // Pop color !!
                    }

                    ImGui::Columns(1);
                    ImGui::Separator();
                }

                ImGui::Separator();

                // Start/Stop Chat Button
                if (!m_active) {
                    if (ImGui::Button("Start Chat")) {
                        m_chatBot->respond("Introduce Yourself",
                            R"(You are a helpful certified flight instructor providing practical feedback and instruction to a student pilot.
You are sitting in the aircraft with the student as they practice various flight exercises and maneuvers.
When you have control of the airplane you chat with the student pilot about technique and answer specific questions.
Finish answers with follow-up questions to check for understanding of the student.)"
);
                        m_active = true;
                    }
                }
                if (m_chatBot->isListening()) {
                    if (ImGui::Button("Stop Chat")) {
                        m_chatBot->stopListening();
                        m_active = false;
                    }
                }
            }
            catch (const std::exception& e) {
                Base::Logger::log(std::string("Exception in GUI: ") + e.what(), Base::ERR, __FUNCTION__);
            }
            catch (...) {
                Base::Logger::log("Unknown exception in GUI", Base::ERR, __FUNCTION__);
            }

        }


        const std::string ChatView::getStatusMessage(Chat::MessageType type)
        {
            using Chat::MessageType;

            switch (type) {
            case MessageType::UserTranscription: return "Listening";
            case MessageType::AIGeneratedResponse: return "Speaking";
            case MessageType::studentAssertingControl: return "Listening for I have control";
            case MessageType::studentRelinquishingControl: return "Listening for You have control";
            case MessageType::CachedSelect: return "Maneuver selected";
            case MessageType::CachedBegin: return "Beginning Maneuver";
            case MessageType::CachedWarn: return "Warning";
            case MessageType::CachedFatal: return "Fatal";
            case MessageType::CachedSuccess: return "Success";
            default:
                return "Unknown Message Type";
            }
        }

        const ImVec4 ChatView::getStatusColor(Chat::MessageType type)
        {
            if (Chat::isUser(type))
                return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green

            switch (type) {
            case Chat::MessageType::CachedSuccess: return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            case Chat::MessageType::CachedWarn: return ImVec4(1.0f, 0.55f, 0.0f, 1.0f); // Orange
            default: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
            }
        }

        const ImVec4 ChatView::getTextColor(Chat::MessageType type)
        {
            switch (type) {
            case Chat::MessageType::CachedWarn: return ImVec4(1.0f, 0.55f, 0.0f, 1.0f); // Orange
            case Chat::MessageType::CachedFatal: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
            case Chat::MessageType::CachedSuccess: return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
            }
        }






    } // namespace Ui
} // namespace XPlaneChatBot