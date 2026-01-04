#pragma once
#include <GLFW/glfw3.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

#include "Debug.h"
#include "Input.h"

enum class CameraMode { ORBIT, FPS };

class Camera final {
 public:
  Camera()
      : orbitPivot(0.0f, 0.0f, 0.0f),
        fpsPosition(2.0f, 2.0f, 2.0f),
        lastOrbitPosition(0.0f),
        orbitRadius(350.0f),
        orbitTheta(0.0f),
        orbitPhi(0.5f),
        fpsYaw(-135.0f),
        fpsPitch(-35.0f),
        fpsSpeed(10.0f),
        mode(CameraMode::ORBIT) {}

  inline void update(const Input& input, float deltaTime) {
    if (input.wasKeyJustPressed(GLFW_KEY_ENTER)) {
      Debug::log(Debug::Category::CAMERA, "Enter detected! Switching modes...");
      if (mode == CameraMode::ORBIT) {
        switchToFPS();
      } else {
        switchToOrbit();
      }
    }

    if (mode == CameraMode::ORBIT) {
      updateOrbitMode(input, deltaTime);
    } else {
      updateFPSMode(input, deltaTime);
    }
  }

  inline glm::mat4 getViewMatrix() const {
    if (mode == CameraMode::ORBIT) {
      return glm::lookAt(lastOrbitPosition, orbitPivot,
                         glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 forward{};
    forward.x = cos(glm::radians(fpsYaw)) * cos(glm::radians(fpsPitch));
    forward.y = sin(glm::radians(fpsPitch));
    forward.z = sin(glm::radians(fpsYaw)) * cos(glm::radians(fpsPitch));
    forward = glm::normalize(forward);

    return glm::lookAt(fpsPosition, fpsPosition + forward,
                       glm::vec3(0.0f, 1.0f, 0.0f));
  }

  inline CameraMode getMode() const { return mode; }

  inline void setCursorMode(GLFWwindow* window) const {
    if (mode == CameraMode::FPS) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }

  inline glm::vec3 getPosition() const {
    if (mode == CameraMode::ORBIT) {
      return lastOrbitPosition;
    }
    return fpsPosition;
  }

 private:
  inline void updateOrbitMode(const Input& input, float deltaTime) {
    static_cast<void>(deltaTime);

    double dx = 0.0;
    double dy = 0.0;
    input.getMouseDelta(dx, dy);

    if (input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
      orbitTheta -= static_cast<float>(dx) * ORBIT_SENSITIVITY;
      orbitPhi += static_cast<float>(dy) * ORBIT_SENSITIVITY;
      orbitPhi = std::clamp(orbitPhi, 0.1f, glm::pi<float>() - 0.1f);
    }

    const double scroll = input.getScrollDelta();
    if (std::abs(scroll) > std::numeric_limits<double>::epsilon()) {
      Debug::log(Debug::Category::CAMERA, "Scroll: ", scroll,
                 ", radius: ", orbitRadius);
      orbitRadius -= static_cast<float>(scroll) * ZOOM_SENSITIVITY;
      Debug::log(Debug::Category::CAMERA, "New radius: ", orbitRadius);
    }

    const float x = orbitRadius * sin(orbitPhi) * cos(orbitTheta);
    const float y = orbitRadius * cos(orbitPhi);
    const float z = orbitRadius * sin(orbitPhi) * sin(orbitTheta);

    lastOrbitPosition = orbitPivot + glm::vec3(x, y, z);
  }

  inline void updateFPSMode(const Input& input, float deltaTime) {
    double dx = 0.0;
    double dy = 0.0;
    input.getMouseDelta(dx, dy);

    fpsYaw += static_cast<float>(dx) * FPS_MOUSE_SENSITIVITY;
    fpsPitch -= static_cast<float>(dy) * FPS_MOUSE_SENSITIVITY;
    fpsPitch = std::clamp(fpsPitch, -89.0f, 89.0f);

    const double scroll = input.getScrollDelta();
    if (std::abs(scroll) > std::numeric_limits<double>::epsilon()) {
      fpsSpeed += static_cast<float>(scroll) * SPEED_CHANGE_RATE;
      fpsSpeed = std::max(1.0f, fpsSpeed);
      Debug::log(Debug::Category::CAMERA, "FPS speed: ", fpsSpeed);
    }

    glm::vec3 forward{};
    forward.x = cos(glm::radians(fpsYaw)) * cos(glm::radians(fpsPitch));
    forward.y = sin(glm::radians(fpsPitch));
    forward.z = sin(glm::radians(fpsYaw)) * cos(glm::radians(fpsPitch));
    forward = glm::normalize(forward);

    const glm::vec3 right =
        glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    const float speed = fpsSpeed * deltaTime;

    if (input.isKeyPressed(GLFW_KEY_W)) {
      fpsPosition += forward * speed;
    }
    if (input.isKeyPressed(GLFW_KEY_S)) {
      fpsPosition -= forward * speed;
    }
    if (input.isKeyPressed(GLFW_KEY_A)) {
      fpsPosition -= right * speed;
    }
    if (input.isKeyPressed(GLFW_KEY_D)) {
      fpsPosition += right * speed;
    }
    if (input.isKeyPressed(GLFW_KEY_SPACE)) {
      fpsPosition.y += speed;
    }
    if (input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
        input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
      fpsPosition.y -= speed;
    }
  }

  inline void switchToFPS() {
    mode = CameraMode::FPS;
    fpsPosition = lastOrbitPosition;

    const glm::vec3 direction = glm::normalize(orbitPivot - lastOrbitPosition);
    fpsYaw = glm::degrees(atan2(direction.z, direction.x));
    fpsPitch = glm::degrees(asin(direction.y));

    Debug::log(Debug::Category::CAMERA, "Switched to FPS mode");
  }

  inline void switchToOrbit() {
    mode = CameraMode::ORBIT;

    const glm::vec3 offset = fpsPosition - orbitPivot;
    orbitRadius = glm::length(offset);

    if (orbitRadius > 0.001f) {
      const glm::vec3 normalized = offset / orbitRadius;
      orbitPhi = acos(std::clamp(normalized.y, -1.0f, 1.0f));
      orbitTheta = atan2(normalized.z, normalized.x);
    }

    Debug::log(Debug::Category::CAMERA, "Switched to ORBIT mode");
  }

  glm::vec3 orbitPivot;
  glm::vec3 fpsPosition;
  glm::vec3 lastOrbitPosition;
  float orbitRadius;
  float orbitTheta;
  float orbitPhi;
  float fpsYaw;
  float fpsPitch;
  float fpsSpeed;
  CameraMode mode;

  static constexpr float ORBIT_SENSITIVITY = 0.005f;
  static constexpr float ZOOM_SENSITIVITY = 2.0f;
  static constexpr float FPS_MOUSE_SENSITIVITY = 0.1f;
  static constexpr float SPEED_CHANGE_RATE = 2.0f;
};