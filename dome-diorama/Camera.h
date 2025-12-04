#pragma once
#include <glm/glm.hpp>

class Input;

enum class CameraMode { ORBIT, FPS };

class Camera {
 public:
  Camera();

  void update(const Input& input, float deltaTime);
  glm::mat4 getViewMatrix() const;

  inline CameraMode getMode() const { return mode; }
  void setCursorMode(GLFWwindow* window) const;

 private:
  void updateOrbitMode(const Input& input, float deltaTime);
  void updateFPSMode(const Input& input, float deltaTime);
  void switchToFPS();
  void switchToOrbit();

  CameraMode mode;

  float orbitRadius;
  float orbitTheta;
  float orbitPhi;
  glm::vec3 orbitPivot;

  glm::vec3 fpsPosition;
  float fpsYaw;
  float fpsPitch;

  glm::vec3 lastOrbitPosition;

  static constexpr float ORBIT_SENSITIVITY = 0.005f;
  static constexpr float ZOOM_SENSITIVITY = 2.0f;
  static constexpr float MIN_RADIUS = 5.0f;
  static constexpr float MAX_RADIUS = 100.0f;

  static constexpr float FPS_MOUSE_SENSITIVITY = 0.1f;
  static constexpr float FPS_MOVE_SPEED = 10.0f;
};