/**
 * @file chatview.h
 * @author zah
 * @brief Implementation of ChatView class
 * @version 0.1
 * @date 2023-10-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef XPROTECTION_UI_CHATVIEW_H
#define XPROTECTION_UI_CHATVIEW_H

#include <chrono>
#include <string>
#include <cstring>
#include <memory>
#include <XPLMUtilities.h>

#include "defs.h"
#include "ui/imgui/imwindow.h"
#include "chatbot/ChatBot.h"

namespace XPlaneChatBot {
namespace Ui {

/// @brief This class provides a GUI for the chat output
class ChatView : public ImWindow
{
public:
    ChatView(std::shared_ptr<Chat::ChatBot> chatBot); ///< Constructor for ChatView class: initializes member variables
    void doBuild() override; ///< Builds the GUI
private:

    /**
     * @brief Determines the status message for the chat based on the type of the latest message
     * @param type Type of message
     * @return std::string Message to display
     */
    const std::string getStatusMessage(Chat::MessageType type);

    /**
     * @brief Determines the status color for the chat based on the type of the latest message
     * @param type Type of message
     * @return ImVec4 Color for the status indicator
     */
    const ImVec4 getStatusColor(Chat::MessageType type);

    /**
     * @brief Determines the text/status color for the chat based on the type of the latest message
     * @param type Type of message
     * @return ImVec4 Color for the text
     */
    const ImVec4 getTextColor(Chat::MessageType type);



    std::shared_ptr<Chat::ChatBot> m_chatBot; ///< ChatBot pointer
    std::string m_errorMessage; ///< Error message (if any)
    bool m_active; ///< Flag to indicate whether the chat is active or not

    // UI specs
    ImVec4 m_green; ///< For when the chatbot is listening and IDEAL and SUCCESS messages
    ImVec4 m_red; ///< For when the chatbot is not listening and FATAL messages
    ImVec4 m_orange; ///< For warnings
    ImVec4 m_white; ///< For all other text
    float m_fontSize; ///< Font size

};

} // namespace Ui
} // namespace XPlaneChatBot

#endif // XPROTECTION_UI_ChatBotVIEW_H
