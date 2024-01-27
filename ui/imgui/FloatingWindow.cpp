#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <XPLMGraphics.h>
#include <XPLMDisplay.h>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <memory>
#include <stdexcept>
#include "FloatingWindow.h"
#include "defs.h"
#include "base/logger.h"

namespace XPlaneChatBot {
namespace Ui {

FloatingWindow::FloatingWindow(int winWidth, int winHeight, int winDecoration, bool setRight)
    : width(winWidth)
    , height(winHeight)
    , decoration(winDecoration)
    , setRight(setRight)
{
    vrEnabledRef = XPLMFindDataRef("sim/graphics/VR/enabled");
    modeliewMatrixRef = XPLMFindDataRef("sim/graphics/view/modelview_matrix");
    viewportRef = XPLMFindDataRef("sim/graphics/view/viewport");
    projectionMatrixRef = XPLMFindDataRef("sim/graphics/view/projection_matrix");

    createWindow();
}

void FloatingWindow::createWindow() {
    int winLeft, winTop, winRight, winBot;
    XPLMGetScreenBoundsGlobal(&winLeft, &winTop, &winRight, &winBot);

    XPLMCreateWindow_t params;
    params.structSize = sizeof(params);
    if (setRight) {
        params.left = winRight - 20 - width;
        params.right = winRight - 20;
        params.top = winTop - 50;
        params.bottom = winTop - 50 - height;
    }
    else {
        params.left = winLeft + 100;
        params.right = winLeft + 100 + width;
        params.top = winTop - 100;
        params.bottom = winTop - 100 - height;
    } 
    
    
    params.visible = 1;
    params.refcon = this;
    params.drawWindowFunc = [] (XPLMWindowID id, void *ref) {
        UNUSED(id);
        reinterpret_cast<FloatingWindow *>(ref)->onDraw();
    };
    params.handleMouseClickFunc = [] (XPLMWindowID id, int x, int y, XPLMMouseStatus status, void *ref) -> int {
        UNUSED(id);
        return reinterpret_cast<FloatingWindow *>(ref)->onClick(x, y, status);
    };
    params.handleRightClickFunc = [] (XPLMWindowID id, int x, int y, XPLMMouseStatus status, void *ref) -> int {
        UNUSED(id);
        return reinterpret_cast<FloatingWindow *>(ref)->onRightClick(x, y, status);
    };
    params.handleKeyFunc = [] (XPLMWindowID id, char key, XPLMKeyFlags flags, char vKey, void *ref, int losingFocus) {
        UNUSED(id);
        reinterpret_cast<FloatingWindow *>(ref)->onKey(key, flags, vKey, losingFocus);
    };
    params.handleCursorFunc = [] (XPLMWindowID id, int x, int y, void *ref) -> XPLMCursorStatus {
        UNUSED(id);
        return reinterpret_cast<FloatingWindow *>(ref)->getCursor(x, y);
    };
    params.handleMouseWheelFunc =  [] (XPLMWindowID id, int x, int y, int wheel, int clicks, void *ref) -> int {
        UNUSED(id);
        return reinterpret_cast<FloatingWindow *>(ref)->onMouseWheel(x, y, wheel, clicks);
    };
    params.layer = xplm_WindowLayerFloatingWindows;

    params.decorateAsFloatingWindow = decoration;

    window = XPLMCreateWindowEx(&params);

    if (!window) {
        Base::Logger::log("Couldn't create window");
        return;
    }

    moveFromOrToVR();
}

void FloatingWindow::setDrawCallback(DrawCallback cb) {
    onDrawCB = cb;
}

void FloatingWindow::setClickCallback(ClickCallback cb) {
    onClickCB = cb;
}

void FloatingWindow::setCloseCallback(CloseCallback cb) {
    onCloseCB = cb;
}

void FloatingWindow::setKeyCallback(KeyCallback cb) {
    onKeyCB = cb;
}

void FloatingWindow::setTitle(const char *title) {
    XPLMSetWindowTitle(window, title);
}

void FloatingWindow::moveFromOrToVR() {
    bool vrEnabled = XPLMGetDatai(vrEnabledRef);

    if (vrEnabled && !isInVR) {
        // X-Plane switched to VR but our window isn't in VR
        XPLMSetWindowPositioningMode(window, xplm_WindowVR, -1);
        isInVR = true;
    } else if (!vrEnabled && isInVR) {
        // Our window is still in VR but X-Plane switched to 2D
        XPLMSetWindowPositioningMode(window, xplm_WindowPositionFree, -1);
        isInVR = false;
        
        int winLeft, winTop, winRight, winBot;
        XPLMGetScreenBoundsGlobal(&winLeft, &winTop, &winRight, &winBot);
        XPLMSetWindowGeometry(window, winLeft + 100, winTop - 100, winLeft + 100 + width, winTop - 100 - height);
    }
}

void FloatingWindow::requestInputFocus(bool req) {
    XPLMTakeKeyboardFocus(req ? window : nullptr);
}

bool FloatingWindow::hasInputFocus() {
    return XPLMHasKeyboardFocus(window);
}

void FloatingWindow::updateMatrices() {
    // Get the current modelview matrix, viewport, and projection matrix from X-Plane
    XPLMGetDatavf(modeliewMatrixRef, mModelView, 0, 16);
    XPLMGetDatavf(projectionMatrixRef, mProjection, 0, 16);
    XPLMGetDatavi(viewportRef, mViewport, 0, 4);
}

void FloatingWindow::boxelsToNative(int x, int y, int& outX, int& outY) {
    GLfloat boxelPos[4] = { (GLfloat) x, (GLfloat) y, 0, 1 };
    GLfloat eye[4], ndc[4];

    multMatrixVec4f(eye, mModelView, boxelPos);
    multMatrixVec4f(ndc, mProjection, eye);
    ndc[3] = 1.0f / ndc[3];
    ndc[0] *= ndc[3];
    ndc[1] *= ndc[3];

    outX = static_cast<int>((ndc[0] * 0.5f + 0.5f) * mViewport[2] + mViewport[0]);
    outY = static_cast<int>((ndc[1] * 0.5f + 0.5f) * mViewport[3] + mViewport[1]);
}

void FloatingWindow::onDraw() {
    if (onDrawCB) {
        onDrawCB(*this);
    }
}

bool FloatingWindow::onClick(int x, int y, XPLMMouseStatus status) {
    if (onClickCB) {
        onClickCB(*this, x, y, status);
    }
    return true;
}

void FloatingWindow::reportClose() {
    if (onCloseCB) {
        onCloseCB(*this);
    }
}

bool FloatingWindow::isVisible() const {
    return XPLMGetWindowIsVisible(window);
}

void FloatingWindow::setVisible(bool visible) {
    XPLMSetWindowIsVisible(window, visible);
}

bool FloatingWindow::onRightClick(int x, int y, XPLMMouseStatus status) {
    UNUSED(x);
    UNUSED(y);
    UNUSED(status);
    return true;
}

void FloatingWindow::onKey(char key, XPLMKeyFlags flags, char virtualKey, bool losingFocus) {
    if (losingFocus) {
        return;
    }

    if (onKeyCB) {
        onKeyCB(*this, key, virtualKey, flags);
    }
}

XPLMCursorStatus FloatingWindow::getCursor(int x, int y) {
    UNUSED(x);
    UNUSED(y);
    return xplm_CursorDefault;
}

bool FloatingWindow::onMouseWheel(int x, int y, int wheel, int clicks) {
    UNUSED(x);
    UNUSED(y);
    UNUSED(wheel);
    UNUSED(clicks);
    return true;
}

XPLMWindowID FloatingWindow::getXWindow() {
    return window;
}

FloatingWindow::~FloatingWindow() {
    XPLMDestroyWindow(window);
}

}
}
