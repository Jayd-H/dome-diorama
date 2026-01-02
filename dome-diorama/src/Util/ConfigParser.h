#pragma once
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Util/Debug.h"

class ConfigParser final {
 public:
  inline ConfigParser() = default;

  inline bool load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      Debug::log(Debug::Category::MAIN,
                 "Failed to open config file: ", filename);
      return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
      line = trim(line);

      if (line.empty() || line[0] == ';' || line[0] == '#') {
        continue;
      }

      if (line[0] == '[' && line[line.length() - 1] == ']') {
        currentSection = line.substr(1, line.length() - 2);
        continue;
      }

      size_t delimiterPos = line.find('=');
      if (delimiterPos != std::string::npos) {
        std::string key = trim(line.substr(0, delimiterPos));
        std::string value = trim(line.substr(delimiterPos + 1));

        if (!currentSection.empty()) {
          key = currentSection + "." + key;
        }

        data[key] = value;
      }
    }

    Debug::log(Debug::Category::MAIN, "Loaded ", data.size(),
               " config values from ", filename);
    return true;
  }

  inline std::string getString(const std::string& key,
                               const std::string& defaultValue = "") const {
    auto it = data.find(key);
    return it != data.end() ? it->second : defaultValue;
  }

  inline int getInt(const std::string& key, int defaultValue = 0) const {
    auto it = data.find(key);
    if (it != data.end()) {
      try {
        return std::stoi(it->second);
      } catch (...) {
        return defaultValue;
      }
    }
    return defaultValue;
  }

  inline float getFloat(const std::string& key,
                        float defaultValue = 0.0f) const {
    auto it = data.find(key);
    if (it != data.end()) {
      try {
        return std::stof(it->second);
      } catch (...) {
        return defaultValue;
      }
    }
    return defaultValue;
  }

  inline bool getBool(const std::string& key, bool defaultValue = false) const {
    auto it = data.find(key);
    if (it != data.end()) {
      std::string value = it->second;
      std::transform(value.begin(), value.end(), value.begin(), ::tolower);
      return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    return defaultValue;
  }

 private:
  std::unordered_map<std::string, std::string> data;

  inline std::string trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
  }
};