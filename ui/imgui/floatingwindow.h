 /**
  * @file StreamPy.h
  * @author lc (edited by zah)
  * @brief Defines the FloatingWindow class for creating X-Plane floating windows.
  *
  * @version 0.1
  * @date 2023-10-20
  *
  * @copyright Copyright (c) 2023
  *
  */

#ifndef _FLOATINGWINDOW_H_
#define _FLOATINGWINDOW_H_

#include <XPLMGraphics.h>
#include <XPLMDisplay.h>
#include <XPLMDataAccess.h>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace XPlaneChatBot {
    namespace Ui {

        /**
         * @class FloatingWindow
         *
         * @brief A class for creating floating windows within X-Plane.
         */
        class FloatingWindow {
        public:
            /**
             * @brief Function type for drawing the window.
             */
            using DrawCallback = std::function<void(FloatingWindow&)>;

            /**
             * @brief Function type for handling mouse click events.
             */
            using ClickCallback = std::function<void(FloatingWindow&, int, int, XPLMMouseStatus)>;

            /**
             * @brief Function type for handling window close events.
             */
            using CloseCallback = std::function<void(FloatingWindow&)>;

            /**
             * @brief Function type for handling keyboard input events.
             */
            using KeyCallback = std::function<void(FloatingWindow&, char, char, XPLMKeyFlags)>;

            /**
             * @brief Construct a new FloatingWindow object.
             *
             * @param winWidth The width of the window.
             * @param winHeight The height of the window.
             * @param winDecoration The decoration style of the window (0 = no decoration, 1 = standard decoration, 2 = self-decorated).
             * @param setLeft Optional flag to set the window on the left side.
             */
            FloatingWindow(int winWidth, int winHeight, int winDecoration, bool setRight = false);

            /**
             * @brief Set the function to be called for drawing the window.
             */
            void setDrawCallback(DrawCallback cb);

            /**
             * @brief Set the function to be called for handling mouse click events.
             */
            void setClickCallback(ClickCallback cb);

            /**
             * @brief Set the function to be called when the window is closed.
             */
            void setCloseCallback(CloseCallback cb);

            /**
             * @brief Set the function to be called for handling keyboard input events.
             */
            void setKeyCallback(KeyCallback cb);

            /**
             * @brief Set the title of the window.
             *
             * @param title The window title.
             */
            void setTitle(const char* title);

            /**
             * @brief Check if the window is currently visible.
             *
             * @return `true` if the window is visible, `false` otherwise.
             */
            bool isVisible() const;

            /**
             * @brief Set the visibility of the window.
             *
             * @param visible `true` to make the window visible, `false` to hide it.
             */
            void setVisible(bool visible);

            /**
             * @brief Move the window between the regular display and VR display.
             */
            void moveFromOrToVR();

            /**
             * @brief Request or release input focus for the window.
             *
             * @param req `true` to request input focus, `false` to release it.
             */
            void requestInputFocus(bool req);

            /**
             * @brief Check if the window currently has input focus.
             *
             * @return `true` if the window has input focus, `false` otherwise.
             */
            bool hasInputFocus();

            /**
             * @brief Notify the system that the window is being closed.
             */
            void reportClose();

            /**
             * @brief Convert window coordinates from Boxels to the native coordinate system.
             *
             * @param x X-coordinate in Boxels.
             * @param y Y-coordinate in Boxels.
             * @param outX Output X-coordinate in the native coordinate system.
             * @param outY Output Y-coordinate in the native coordinate system.
             */
            void boxelsToNative(int x, int y, int& outX, int& outY);

            /**
             * @brief Get the X-Plane window identifier associated with this window.
             *
             * @return The X-Plane window identifier.
             */
            XPLMWindowID getXWindow();

            /**
             * @brief Virtual destructor for the FloatingWindow class.
             */
            virtual ~FloatingWindow();

        protected:
            /**
             * @brief Update the transformation matrices for the window.
             */
            void updateMatrices();

            /**
             * @brief Override this function to provide custom drawing for the window.
             */
            virtual void onDraw();

            /**
             * @brief Override this function to handle mouse click events.
             */
            virtual bool onClick(int x, int y, XPLMMouseStatus status);

            /**
             * @brief Override this function to handle right-click events.
             */
            virtual bool onRightClick(int x, int y, XPLMMouseStatus status);

            /**
             * @brief Override this function to handle keyboard input events.
             */
            virtual void onKey(char key, XPLMKeyFlags flags, char virtualKey, bool losingFocus);

            /**
             * @brief Override this function to specify the cursor status at a given position.
             */
            virtual XPLMCursorStatus getCursor(int x, int y);

            /**
             * @brief Override this function to handle mouse wheel events.
             */
            virtual bool onMouseWheel(int x, int y, int wheel, int clicks);

        private:
            XPLMWindowID window{}; ///< The X-Plane window identifier.
            int width, height, decoration; ///< Window dimensions and decoration style.
            bool setRight; ///< Flag to set the window on the right side.
            bool isInVR = false; ///< Flag indicating if the window is in VR mode.

            XPLMDataRef vrEnabledRef{}; ///< Data reference for VR mode.
            XPLMDataRef modeliewMatrixRef{}; ///< Data reference for modelview matrix.
            XPLMDataRef viewportRef{}; ///< Data reference for viewport.
            XPLMDataRef projectionMatrixRef{}; ///< Data reference for projection matrix.

            float mModelView[16]{}; ///< Modelview matrix.
            float mProjection[16]{}; ///< Projection matrix.
            int mViewport[4]{}; ///< Viewport dimensions.

            DrawCallback onDrawCB; ///< Function to be called for drawing.
            ClickCallback onClickCB; ///< Function to be called for handling mouse click events.
            CloseCallback onCloseCB; ///< Function to be called when the window is closed.
            KeyCallback onKeyCB; ///< Function to be called for handling keyboard input events.

            /**
             * @brief Create the X-Plane window.
             */
            void createWindow();
        };

    }
}

#endif // _FLOATINGWINDOW_H_
