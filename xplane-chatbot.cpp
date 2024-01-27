/**
 * @file xprotection.cpp
 * @author lc
 * @brief Implementation file of main class of plugin called Plugin. It is responsible for loading and unloading the plugin. 
 * + Flight Manager, Sound Manager, Trigger Manager are three distinct roles throughout the plugin. 
 * + The flight manager is responsible for getting the current state of the aircraft. 
 * + The sound manager is responsible for playing sounds. 
 * + The trigger manager is responsible for monitoring the aircraft state and playing sounds when the aircraft is in a dangerous state. 
 * + The training data is responsible for loading the training data from the XML file.
 * @version 0.1
 * @date 2023-09-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "xplane-chatbot.h"

namespace XPlaneChatBot {

/// @brief Constructs a new Plugin Object.
Plugin::Plugin()
    : m_pluginName(PLUGIN_NAME)
    , m_pluginSignature(PLUGIN_SIGNATURE)
    , m_pluginDescription(PLUGIN_DESCRIPTION)
{
    Base::Logger::log("Plugin constructor called", Base::LogLevel::INFO, __FUNCTION__);
    std::string logs_path = get_plugin_path() + "Logs" + XPLMGetDirectorySeparator();
    Base::Logger::openLogFile(logs_path);

    m_errorCallback = [](const char* inMessage) {
		Base::Logger::log("Error callback called: " + std::string(inMessage), Base::LogLevel::ERR, __FUNCTION__);
	};
    XPLMSetErrorCallback(m_errorCallback);
}

/// @brief Destructs Plugin object and frees trigger and sound manager pointers.
Plugin::~Plugin()
{
    Base::Logger::log("Plugin destructor called", Base::LogLevel::INFO, __FUNCTION__);
    Base::Logger::closeLogFile();
}

/// @brief Getter for m_pluginName
/// @return Name of plugin
const std::string &Plugin::pluginName() const
{
    return m_pluginName;
}

/// @brief Getter for m_pluginSignature
/// @return Signature of plugin
const std::string &Plugin::pluginSignature() const
{
    return m_pluginSignature;
}

/// @brief Getter for m_pluginDescription
/// @return Plugin Description
const std::string &Plugin::pluginDescription() const
{
    return m_pluginDescription;
}

/// @brief Load/Enable/Setup the plugin. Initializes sound, flight, and trigger managers and adds XML data to the trigger manager.
/// @return False if XML couldn't be loaded, True otherwise.
bool Plugin::enable() 
{
    m_chatBot = std::make_shared<Chat::ChatBot>();
    
    /////////////////////////////////// Let this be a template for communicating with the website ///////////////////////////// 
    /*Prepare data to send
    std::string postData = "RealTimeTranscriber was enabled on Adam's Computer";

    
    CURL* curl = curl_easy_init(); // Initialize curl
    if (curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, "https://fly-with-ai-web-d348dcf01217.herokuapp.com/update");

        // Set the POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

        // Perform the request, and get a response code
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    */

    return true;
} // N.B XP manages all of the memory on the machine and we are just allocating space through XPLM.

/// @brief Unloads/Disables/Tears down the plugin. Resets maneuver window, stops monitoring triggers, frees pointers to managers and data.  
/// @return True
bool Plugin::disable()
{
    return true;
}

/// @brief Shows maneuvers window and chat window if arguments have been initialized
void Plugin::showChatWindow() {
    try {
        if (!m_chatBot) {
            Base::Logger::log("Chat Bot not initialized", Base::LogLevel::ERR, __FUNCTION__);
            return;
        }
        m_chatView = std::make_shared<Ui::ChatView>(m_chatBot);
    }
    catch (const std::exception& e) {
		Base::Logger::log("Exception caught: " + std::string(e.what()), Base::LogLevel::ERR, __FUNCTION__);
        throw;
	}
}

} // namespace XPlaneChatBot
