/**
 * @file imwindow.cpp
 * @author lc
 * @brief 
 * imwindow is a gui tool taken right off of Xplane's website. It is used to create a window that can be used to display stuff
 * @version 0.1
 * @date 2023-09-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <XPLMGraphics.h>
#include <XPLMUtilities.h>
#include <XPLMPlugin.h>
#include <cstdint>
#include <cctype>
#include "ImWindow.h"
#include "base/logger.h"
#include "defs.h"

namespace XPlaneChatBot {
namespace Ui {

ImWindow::ImWindow(int width, int height, int decoration, bool menu, bool setRight, bool chatWindow, bool boldFont)
    : FloatingWindow(width, height, decoration, setRight)
    , wantsMenu(menu)
{
    namespace fs = std::filesystem;

    Base::Logger::log("ImWindow constructor called", Base::DEBUG, __FUNCTION__);
    imGuiContext = ImGui::CreateContext();
    Base::Logger::log("ImGui context created", Base::DEBUG, __FUNCTION__);

    ImGui::SetCurrentContext(imGuiContext);
    Base::Logger::log("Set current ImGui context", Base::DEBUG, __FUNCTION__);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0;
    Base::Logger::log("ImGui style configured", Base::DEBUG, __FUNCTION__);

    auto& io = ImGui::GetIO();
    io.RenderDrawListsFn = nullptr; // [OBSOLETE since 1.60+] 
    io.IniFilename = nullptr;
    io.ConfigMacOSXBehaviors = false;
    io.ConfigFlags = ImGuiConfigFlags_NavNoCaptureKeyboard;
    Base::Logger::log("ImGui IO configured", Base::DEBUG, __FUNCTION__);

    // Setting up key mappings
    Base::Logger::log("Setting up key mappings", Base::DEBUG, __FUNCTION__);
    io.KeyMap[ImGuiKey_Tab] = XPLM_VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = XPLM_VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = XPLM_VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = XPLM_VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = XPLM_VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = XPLM_VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = XPLM_VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = XPLM_VK_HOME;
    io.KeyMap[ImGuiKey_End] = XPLM_VK_END;
    io.KeyMap[ImGuiKey_Insert] = XPLM_VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = XPLM_VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = XPLM_VK_BACK;
    io.KeyMap[ImGuiKey_Space] = XPLM_VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = XPLM_VK_ENTER;
    io.KeyMap[ImGuiKey_Escape] = XPLM_VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = XPLM_VK_A;
    io.KeyMap[ImGuiKey_C] = XPLM_VK_C;
    io.KeyMap[ImGuiKey_V] = XPLM_VK_V;
    io.KeyMap[ImGuiKey_X] = XPLM_VK_X;
    io.KeyMap[ImGuiKey_Y] = XPLM_VK_Y;
    io.KeyMap[ImGuiKey_Z] = XPLM_VK_Z;
    Base::Logger::log("Key mappings set up complete", Base::DEBUG, __FUNCTION__);

    // Font setup
    Base::Logger::log("Setting up fonts", Base::DEBUG, __FUNCTION__);
    unsigned char* pixels{nullptr};
    int fontTexWidth{ 0 }, fontTexHeight{ 0 };

    // Determine the plugin's directory
    std::string pluginDir = get_plugin_path(); // Assuming this returns the directory path

    // Construct the path to the fonts directory
    fs::path fontDir = fs::path(pluginDir).parent_path().parent_path().parent_path() / "fonts";

    std::string relativeFontPath = boldFont ? "Roboto-Bold.ttf" : "DejaVuSans.ttf";
    fs::path fontPath = fontDir / relativeFontPath;

    if (!fs::exists(fontPath)) {
        Base::Logger::log("Font file does not exist: " + fontPath.generic_string(), Base::ERR, __FUNCTION__);
    }
    else { // Load the font
        Base::Logger::log("Font path: " + fontPath.generic_string(), Base::DEBUG, __FUNCTION__);
        io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 18);
    }
    Base::Logger::log("Font added", Base::DEBUG, __FUNCTION__);

    {
        io.Fonts->GetTexDataAsAlpha8(&pixels, &fontTexWidth, &fontTexHeight);
        if (pixels == nullptr || fontTexWidth == 0 || fontTexHeight == 0) {
            Base::Logger::log("Failed to get font texture data", Base::ERR, __FUNCTION__);
        }
        else {
            Base::Logger::log("Font texture data retrieved successfully", Base::DEBUG, __FUNCTION__);
        }
    }

    // Texture setup
    int textureId;
    XPLMGenerateTextureNumbers(&textureId, 1);
    fontTextureId = (GLuint)textureId;
    Base::Logger::log("Texture ID generated", Base::DEBUG, __FUNCTION__);

    XPLMBindTexture2d(fontTextureId, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, fontTexWidth, fontTexHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    io.Fonts->TexID = (void*)(intptr_t)(fontTextureId);
    Base::Logger::log("Font texture set up", Base::DEBUG, __FUNCTION__);

    ImGui::StyleColorsLight();
    if (chatWindow) {
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f); // Semi-transparent dark background
        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White text
        Base::Logger::log("Chat window style set", Base::DEBUG, __FUNCTION__);
    }
    else {
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.91f, 0.91f, 0.91f, 1.00f);
        Base::Logger::log("Default window style set", Base::DEBUG, __FUNCTION__);
    }

    Base::Logger::log("ImWindow constructor completed", Base::DEBUG, __FUNCTION__);

}

void ImWindow::onDraw() {
    try {
        updateMatrices();
        buildGUI();
        showGUI();

        ImGui::SetCurrentContext(imGuiContext);
        auto& io = ImGui::GetIO();
        bool hasKeyboardFocus = hasInputFocus();
        if (io.WantTextInput && !hasKeyboardFocus) {
            requestInputFocus(true);
        }
        else if (!io.WantTextInput && hasKeyboardFocus) {
            requestInputFocus(false);
            // reset keysdown otherwise we'll think any keys used to defocus the keyboard are still down!
            std::fill(std::begin(io.KeysDown), std::end(io.KeysDown), false);
            io.KeyShift = false;
            io.KeyAlt = false;
            io.KeyCtrl = false;
            io.KeySuper = false;
        }

        FloatingWindow::onDraw();
    }
    catch (const std::exception& e) {
		Base::Logger::log(std::string("Exception in GUI: ") + e.what(), Base::ERR, __FUNCTION__);
        throw;
	}
}

void ImWindow::buildGUI() {
    ImGui::SetCurrentContext(imGuiContext);
    auto &io = ImGui::GetIO();

    // transfer the window geometry to ImGui
    XPLMGetWindowGeometry(getXWindow(), &mLeft, &mTop, &mRight, &mBottom);

    float win_width = static_cast<float>(mRight - mLeft);
    float win_height = static_cast<float>(mTop - mBottom);

    io.DisplaySize = ImVec2(win_width, win_height);
    // in boxels, we're always scale 1, 1.
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2((float) 0.0, (float) 0.0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(win_width, win_height), ImGuiCond_Always);

    // and construct the window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 5.0);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 5.0);
    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (wantsMenu) {
        flags |= ImGuiWindowFlags_MenuBar;
    }
    ImGui::Begin("XPlaneChatBot", nullptr, flags);
    try {
        doBuild();
    } catch (const std::exception &e) {
        Base::Logger::log(std::string("Exception in GUI: ") + e.what(), Base::ERR, __FUNCTION__);
        throw;
    }

    ImGui::End();
    ImGui::PopStyleVar(5);
    ImGui::Render();
}

void ImWindow::showGUI() {
    ImGui::SetCurrentContext(imGuiContext);
    auto &io = ImGui::GetIO();

    ImDrawData *drawData = ImGui::GetDrawData();

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    drawData->ScaleClipRects(io.DisplayFramebufferScale);

    // We are using the OpenGL fixed pipeline because messing with the
    // shader-state in X-Plane is not very well documented, but using the fixed
    // function pipeline is.

    // 1TU + Alpha settings, no depth, no fog.
    XPLMSetGraphicsState(0, 1, 0, 1, 1, 0, 0);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glScalef(1.0f, -1.0f, 1.0f);
    glTranslatef(static_cast<GLfloat>(mLeft), static_cast<GLfloat>(-mTop), 0.0f);

    // Render command lists
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);

                // Scissors work in viewport space - must translate the coordinates from ImGui -> Boxels, then Boxels -> Native.
                //FIXME: it must be possible to apply the scale+transform manually to the projection matrix so we don't need to doublestep.
                int bTop, bLeft, bRight, bBottom;
                translateImguiToBoxel(pcmd->ClipRect.x, pcmd->ClipRect.y, bLeft, bTop);
                translateImguiToBoxel(pcmd->ClipRect.z, pcmd->ClipRect.w, bRight, bBottom);
                int nTop, nLeft, nRight, nBottom;
                boxelsToNative(bLeft, bTop, nLeft, nTop);
                boxelsToNative(bRight, bBottom, nRight, nBottom);
                glScissor(nLeft, nBottom, nRight-nLeft, nTop-nBottom);
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    // Restore modified state
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();
    glPopClientAttrib();
}

bool ImWindow::onClick(int x, int y, XPLMMouseStatus status) {
    ImGui::SetCurrentContext(imGuiContext);
    ImGuiIO& io = ImGui::GetIO();

    float outX, outY;
    translateToImguiSpace(x, y, outX, outY);
    io.MousePos = ImVec2(outX, outY);

    switch (status) {
    case xplm_MouseDown:
    case xplm_MouseDrag:
        io.MouseDown[0] = true;
        break;
    case xplm_MouseUp:
        io.MouseDown[0] = false;
        break;
    }

    return FloatingWindow::onClick(x, y, status);
}

bool ImWindow::onMouseWheel(int x, int y, int wheel, int clicks) {
    ImGui::SetCurrentContext(imGuiContext);
    ImGuiIO& io = ImGui::GetIO();

    float outX, outY;
    translateToImguiSpace(x, y, outX, outY);

    io.MousePos = ImVec2(outX, outY);
    switch (wheel) {
    case 0:
        io.MouseWheel = static_cast<float>(clicks);
        break;
    case 1:
        io.MouseWheelH = static_cast<float>(clicks);
        break;
    }

    return FloatingWindow::onMouseWheel(x, y, wheel, clicks);
}

XPLMCursorStatus ImWindow::getCursor(int x, int y) {
    ImGui::SetCurrentContext(imGuiContext);
    ImGuiIO& io = ImGui::GetIO();

    float outX, outY;
    translateToImguiSpace(x, y, outX, outY);
    io.MousePos = ImVec2(outX, outY);

    return xplm_CursorDefault;
}

void ImWindow::onKey(char key, XPLMKeyFlags flags, char virtualKey, bool losingFocus) {
    if (losingFocus) {
        return;
    }

    ImGui::SetCurrentContext(imGuiContext);
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        // If you press and hold a key, the flags will actually be down, 0, 0, ..., up
        // So the key always has to be considered as pressed unless the up flag is set
        auto vk = static_cast<unsigned char>(virtualKey);
        io.KeysDown[vk] = (flags & xplm_UpFlag) != xplm_UpFlag;
        io.KeyShift = (flags & xplm_ShiftFlag) != 0;
        io.KeyAlt = (flags & xplm_OptionAltFlag) != 0;
        io.KeyCtrl = (flags & xplm_ControlFlag) != 0;

        if ((flags & xplm_UpFlag) != xplm_UpFlag
            && !io.KeyCtrl
            && !io.KeyAlt
            && std::isprint(key)) {
            char smallStr[] = { key, 0 };
            io.AddInputCharactersUTF8(smallStr);
        }
    }

    buildGUI();

    FloatingWindow::onKey(key, flags, virtualKey, losingFocus);
}

void ImWindow::translateImguiToBoxel(float inX, float inY, int& outX, int& outY) {
    outX = (int)(mLeft + inX);
    outY = (int)(mTop - inY);
}

void ImWindow::translateToImguiSpace(int inX, int inY, float& outX, float& outY) {
    outX = static_cast<float>(inX - mLeft);
    if (outX < 0.0f || outX > (float)(mRight - mLeft)) {
        outX = -FLT_MAX;
        outY = -FLT_MAX;
        return;
    }
    outY = static_cast<float>(mTop - inY);
    if (outY < 0.0f || outY > (float)(mTop - mBottom)) {
        outX = -FLT_MAX;
        outY = -FLT_MAX;
        return;
    }
}

ImWindow::~ImWindow() {
    ImGui::DestroyContext(imGuiContext);
    glDeleteTextures(1, &fontTextureId);
}

}
}
