/**
 * @file StreamPy.h
 * @author lc (edited by zah)
 * @brief This file defines the ImWindow class for creating ImGui-based windows.
 *
 * @version 0.1
 * @date 2023-10-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef IMWINDOW_H_
#define IMWINDOW_H_

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <memory>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include "FloatingWindow.h"

namespace XPlaneChatBot {
    namespace Ui {

        /**
         * @class ImWindow
         *
         * @brief Base class for creating ImGui-based windows.
         */
        class ImWindow : public FloatingWindow {
        public:
            /**
             * @brief Construct a new ImWindow object.
             *
             * @param width Width of the window.
             * @param height Height of the window.
             * @param decoration Decoration style of the window.
             * @param menu Flag to include a menu.
             * @param setRight Optional flag to set the window on the right side.
             * @param chatWindow Optional flag to use a chat window style (changes colors of background and text).
             * @param boldFont Optional flag to use a bold font.
             */
            ImWindow(int width, int height, int decoration, bool menu, bool setRight = false, bool chatWindow = false, bool boldFont = false);

            /**
             * @brief Virtual destructor for the ImWindow class.
             */
            virtual ~ImWindow();

        protected:
            /**
             * @brief Must be implemented by derived classes to build the ImGui user interface for the window.
             */
            virtual void doBuild() = 0;

            /**
             * @brief Overridden function for rendering the window content.
             */
            void onDraw() override;

            /**
             * @brief Overridden function for handling mouse clicks.
             *
             * @param x X-coordinate of the click.
             * @param y Y-coordinate of the click.
             * @param status The mouse status.
             */
            bool onClick(int x, int y, XPLMMouseStatus status) override;

            /**
             * @brief Overridden function for handling mouse wheel events.
             *
             * @param x X-coordinate of the mouse wheel event.
             * @param y Y-coordinate of the mouse wheel event.
             * @param wheel The mouse wheel value.
             * @param clicks The number of wheel clicks.
             */
            bool onMouseWheel(int x, int y, int wheel, int clicks) override;

            /**
             * @brief Overridden function for determining the cursor status at a given position.
             *
             * @param x X-coordinate of the cursor position.
             * @param y Y-coordinate of the cursor position.
             */
            XPLMCursorStatus getCursor(int x, int y) override;

            /**
             * @brief Overridden function for handling keyboard input.
             *
             * @param key The pressed key.
             * @param flags Flags associated with the key press.
             * @param virtualKey The virtual key code.
             * @param losingFocus Flag indicating if the window is losing focus.
             */
            void onKey(char key, XPLMKeyFlags flags, char virtualKey, bool losingFocus) override;

        private:
            GLuint fontTextureId{}; ///< Texture ID for fonts.
            ImGuiContext* imGuiContext{}; ///< ImGui context.
            int mLeft{}, mTop{}, mRight{}, mBottom{}; ///< Window dimensions.
            bool wantsMenu = false; ///< Flag to include a menu.

            /**
             * @brief Builds the ImGui user interface for the window.
             */
            void buildGUI();

            /**
             * @brief Displays the ImGui user interface.
             */
            void showGUI();

            /**
             * @brief Translates coordinates from ImGui space to window space.
             *
             * @param inX The input X coordinate in ImGui space.
             * @param inY The input Y coordinate in ImGui space.
             * @param outX The output X coordinate in window space.
             * @param outY The output Y coordinate in window space.
             */
            void translateImguiToBoxel(float inX, float inY, int& outX, int& outY);

            /**
             * @brief Translates coordinates from window space to ImGui space.
             *
             * @param inX The input X coordinate in window space.
             * @param inY The input Y coordinate in window space.
             * @param outX The output X coordinate in ImGui space.
             * @param outY The output Y coordinate in ImGui space.
             */
            void translateToImguiSpace(int inX, int inY, float& outX, float& outY);
        };

    }
}

#endif /* IMWINDOW_H_ */
