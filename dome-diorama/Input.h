#pragma once
#include <GLFW/glfw3.h>

#include <cstring>

#include "Debug.h"

class Input {
 public:
  Input()
      : mouseX(0.0),
        mouseY(0.0),
        lastMouseX(0.0),
        lastMouseY(0.0),
        mouseDeltaX(0.0),
        mouseDeltaY(0.0),
        scrollDelta(0.0),
        firstMouse(true) {
    std::memset(currentKeys, 0, sizeof(currentKeys));
    std::memset(previousKeys, 0, sizeof(previousKeys));
    std::memset(currentMouseButtons, 0, sizeof(currentMouseButtons));
    std::memset(previousMouseButtons, 0, sizeof(previousMouseButtons));
  }

  inline void update() {
    mouseDeltaX = mouseX - lastMouseX;
    mouseDeltaY = mouseY - lastMouseY;
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    if (firstMouse) {
      mouseDeltaX = 0.0;
      mouseDeltaY = 0.0;
      firstMouse = false;
    }
  }

  inline void endFrame() {
    std::memcpy(previousKeys, currentKeys, sizeof(currentKeys));
    std::memcpy(previousMouseButtons, currentMouseButtons,
                sizeof(currentMouseButtons));
    scrollDelta = 0.0;
  }

  inline bool isKeyPressed(int key) const {
    if ((key < 0) || (key > GLFW_KEY_LAST)) {
      return false;
    }
    return currentKeys[key];
  }

  inline bool wasKeyJustPressed(int key) const {
    if ((key < 0) || (key > GLFW_KEY_LAST)) {
      return false;
    }
    return currentKeys[key] && !previousKeys[key];
  }

  inline bool isMouseButtonPressed(int button) const {
    if ((button < 0) || (button > GLFW_MOUSE_BUTTON_LAST)) {
      return false;
    }
    return currentMouseButtons[button];
  }

  inline void getMouseDelta(double& dx, double& dy) const {
    dx = mouseDeltaX;
    dy = mouseDeltaY;
  }

  inline double getScrollDelta() const { return scrollDelta; }

  inline void onKey(int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if ((key < 0) || (key > GLFW_KEY_LAST)) {
      return;
    }

    if (key == GLFW_KEY_ENTER) {
      Debug::log(Debug::Category::INPUT, "Enter key, action: ", action);
    }

    if (action == GLFW_PRESS) {
      currentKeys[key] = true;
    } else if (action == GLFW_RELEASE) {
      currentKeys[key] = false;
    }
  }

  inline void onMouseMove(double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
  }

  inline void onMouseButton(int button, int action, int mods) {
    (void)mods;

    if ((button < 0) || (button > GLFW_MOUSE_BUTTON_LAST)) {
      return;
    }

    if (action == GLFW_PRESS) {
      currentMouseButtons[button] = true;
    } else if (action == GLFW_RELEASE) {
      currentMouseButtons[button] = false;
    }
  }

  inline void onScroll(double xoffset, double yoffset) {
    (void)xoffset;
    Debug::log(Debug::Category::INPUT, "Scroll received: ", yoffset);
    scrollDelta = yoffset;
  }

 private:
  bool currentKeys[GLFW_KEY_LAST + 1];
  bool previousKeys[GLFW_KEY_LAST + 1];
  bool currentMouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
  bool previousMouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
  double mouseX;
  double mouseY;
  double lastMouseX;
  double lastMouseY;
  double mouseDeltaX;
  double mouseDeltaY;
  double scrollDelta;
  bool firstMouse;
};