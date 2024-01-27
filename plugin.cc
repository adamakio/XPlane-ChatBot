/*****************************************************************//**
 * \file   plugin.cc
 * \brief  This is a test for the XPlaneChatBot inside the XPlaneChatBot plugin.
 * 
 * This plugin simulates a chat window that can be used to transcribe speech to text.
 * The window can be opened by clicking on the XPlaneChatBot menu item in the Plugins menu.
 * Then click on the Chat menu item to open the chat window.
 * 
 * \author 16478
 * \date   December 2023
 *********************************************************************/

#include <XPLMDefs.h>
#include <XPLMMenus.h>
#include "defs.h"
#include "base/logger.h"
#include "xplane-chatbot.h"

 // Menu declarations
static int g_menu_container_idx; ///< The index of our menu item in the Plugins menu 
static XPLMMenuID g_menu_id; ///< The menu container we'll append all our menu items to
void menu_handler(void*, void*);

DeclareMenuID_(MENU_CHAT_WINDOW);
DeclareMenuID_(MENU_INFO_WINDOW); // shpeel about the plugin

using namespace XPlaneChatBot;

static Plugin* plugin = nullptr;

void CleanUp();

PLUGIN_API int XPluginStart(
    char* outName,
    char* outSig,
    char* outDesc)
{
    ALLOCATE_MEMORY(plugin, XPlaneChatBot::Plugin)
    
    Base::Logger::log("Starting RealTimeTranscriber Plugin before strcpy_s", Base::INFO, __FUNCTION__);

    strcpy(outName, plugin->pluginName().c_str()); 
    strcpy(outSig, plugin->pluginSignature().c_str()); 
    strcpy(outDesc, plugin->pluginDescription().c_str()); 

    Base::Logger::log("Starting RealTimeTranscriber Plugin", Base::INFO, __FUNCTION__);

    return true;
}


PLUGIN_API int  XPluginEnable(void)
{
    Base::Logger::log("Enabling RealTimeTranscriber Plugin", Base::INFO, __FUNCTION__);

    // Start creating menus
    g_menu_container_idx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "RealTimeTranscriber", nullptr, 0);
    g_menu_id = XPLMCreateMenu("RealTimeTranscriber", XPLMFindPluginsMenu(), g_menu_container_idx, menu_handler, nullptr);

    XPLMAppendMenuItem(g_menu_id, "Chat", CreateMenuID_(MENU_CHAT_WINDOW), 1);

    plugin->enable();

    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    Base::Logger::log("Disabling XPlaneChatBot Plugin", Base::INFO, __FUNCTION__);

    plugin->disable();
}

PLUGIN_API void	XPluginStop(void)
{
    Base::Logger::log("Stoping XPlaneChatBot Plugin", Base::INFO, __FUNCTION__);
    CleanUp();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int, void*)
{
    return;
}

void CleanUp()
{
    FREE_MEMORY(plugin)
}

void menu_handler(void* in_menu_ref, void* in_item_ref) // This is the function that is called when a user clicks on a menu item. It is responsible for calling the appropriate function in the plugin.
{
    UNUSED(in_menu_ref);
    const char* item_name = static_cast<const char*>(in_item_ref);
    if (!strcmp(item_name, MENU_CHAT_WINDOW))
    {
        SAFE_FUNC_CALL(plugin, showChatWindow());
    }
}

#if IBM
BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
)
{
    UNUSED(hModule);
    UNUSED(lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif


