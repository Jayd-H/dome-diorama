#pragma once
#include <GLFW/glfw3.h>

class Input {
 public:
  Input();

  void update();
  void endFrame();

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

  void onKey(int key, int scancode, int action, int mods);
  void onMouseMove(double xpos, double ypos);
  void onMouseButton(int button, int action, int mods);
  void onScroll(double xoffset, double yoffset);

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