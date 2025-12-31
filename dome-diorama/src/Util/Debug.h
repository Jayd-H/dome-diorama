#pragma once
#include <iostream>

class Debug final {
 public:
  enum class Category { MAIN, CAMERA, INPUT, RENDERING, VULKAN };

  static void setEnabled(Category category, bool enabled) {
    switch (category) {
      case Category::MAIN:
        enableMain = enabled;
        break;
      case Category::CAMERA:
        enableCamera = enabled;
        break;
      case Category::INPUT:
        enableInput = enabled;
        break;
      case Category::RENDERING:
        enableRendering = enabled;
        break;
      case Category::VULKAN:
        enableVulkan = enabled;
        break;
    }
  }

  static bool isEnabled(Category category) {
    switch (category) {
      case Category::MAIN:
        return enableMain;
      case Category::CAMERA:
        return enableCamera;
      case Category::INPUT:
        return enableInput;
      case Category::RENDERING:
        return enableRendering;
      case Category::VULKAN:
        return enableVulkan;
      default:
        return false;
    }
  }

  template <typename... Args>
  static void log(Category category, Args&&... args) {
    if (isEnabled(category)) {
      std::cout << "[" << categoryName(category) << "] ";
      (std::cout << ... << args) << std::endl;
    }
  }

private:
  static const char* categoryName(Category cat) {
    switch (cat) {
      case Category::MAIN:
        return "MAIN";
      case Category::CAMERA:
        return "CAMERA";
      case Category::INPUT:
        return "INPUT";
      case Category::RENDERING:
        return "RENDERING";
      case Category::VULKAN:
        return "VULKAN";
      default:
        return "UNKNOWN";
    }
  }

  static bool enableMain;
  static bool enableCamera;
  static bool enableInput;
  static bool enableRendering;
  static bool enableVulkan;
};



inline bool Debug::enableMain = false;
inline bool Debug::enableCamera = false;
inline bool Debug::enableInput = false;
inline bool Debug::enableRendering = false;
inline bool Debug::enableVulkan = false;