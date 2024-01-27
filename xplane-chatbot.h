/**
 * @file xprotection.h
 * @author lc
 * @brief Header file for main class of plugin called Plugin. It is responsible for loading and unloading the plugin.
 * @version 0.1
 * @date 2023-09-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CHATBOT_PLUGIN_H // Prevent header from being included more than once
#define CHATBOT_PLUGIN_H

#include <string>
#include "curl/curl.h"

#include "defs.h"
#include "base/logger.h"

#include "ui/chat-page/chatview.h"

#include "chatbot/ChatBot.h"


namespace XPlaneChatBot {

/// @brief This class is the main class of the plugin. It is responsible for loading and unloading the plugin.
class Plugin
{
public:
    Plugin(); 
    ~Plugin(); 

public:
    const std::string &pluginName() const; 
    const std::string &pluginSignature() const; 
    const std::string &pluginDescription() const; 

    bool enable(); 
    bool disable(); 

    void showChatWindow();
    void showInfoWindow(); 

private:
    std::string m_pluginName; ///< Name of plugin
    std::string m_pluginSignature; ///< Signature of plugin
    std::string m_pluginDescription; ///< Description of plugin

    std::shared_ptr<Chat::ChatBot> m_chatBot; ///< Chatbot pointer

    std::shared_ptr<Ui::ChatView> m_chatView; ///< Shared pointer to chat window

    XPLMError_f m_errorCallback; ///< Error callback function
};

} // namespace XPlaneChatBot 

#endif // XPROTECTION_PLUGIN_H

// The idea of namespace protection is to prevent the plugin from being loaded more than once.