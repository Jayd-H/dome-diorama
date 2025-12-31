#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <utility>

#include "Util/Debug.h"
#include "Resources/Material.h"

using MeshID = uint32_t;
constexpr MeshID INVALID_MESH_ID = 0;

class Object {
 public:
  std::string name;
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;
  MeshID meshID;
  MaterialID materialID;
  bool visible;

  Object()
      : name("Unnamed Object"),
        position(0.0f, 0.0f, 0.0f),
        rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
        scale(1.0f, 1.0f, 1.0f),
        meshID(INVALID_MESH_ID),
        materialID(INVALID_MATERIAL_ID),
        visible(true) {}

  glm::mat4 getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = model * glm::mat4_cast(rotation);
    model = glm::scale(model, scale);
    return model;
  }

  void setPosition(const glm::vec3& pos) { position = pos; }

  void setPosition(float x, float y, float z) { position = glm::vec3(x, y, z); }

  void setRotation(const glm::quat& rot) { rotation = rot; }

  void setRotationEuler(float pitch, float yaw, float roll) {
    rotation = glm::quat(
        glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
  }

  void setRotationEuler(const glm::vec3& euler) {
    rotation = glm::quat(glm::radians(euler));
  }

  void setScale(const glm::vec3& s) { scale = s; }

  void setScale(float x, float y, float z) { scale = glm::vec3(x, y, z); }

  void setScale(float uniform) { scale = glm::vec3(uniform); }

  void setMesh(MeshID mesh) { meshID = mesh; }

  void setMaterial(MaterialID material) { materialID = material; }
};

class ObjectBuilder {
 public:
  ObjectBuilder() : object() {}

  ObjectBuilder& name(const std::string& n) {
    object.name = n;
    return *this;
  }

  ObjectBuilder& position(const glm::vec3& pos) {
    object.position = pos;
    return *this;
  }

  ObjectBuilder& position(float x, float y, float z) {
    object.position = glm::vec3(x, y, z);
    return *this;
  }

  ObjectBuilder& rotation(const glm::quat& rot) {
    object.rotation = rot;
    return *this;
  }

  ObjectBuilder& rotationEuler(float pitch, float yaw, float roll) {
    object.rotation = glm::quat(
        glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
    return *this;
  }

  ObjectBuilder& rotationEuler(const glm::vec3& euler) {
    object.rotation = glm::quat(glm::radians(euler));
    return *this;
  }

  ObjectBuilder& scale(const glm::vec3& s) {
    object.scale = s;
    return *this;
  }

  ObjectBuilder& scale(float x, float y, float z) {
    object.scale = glm::vec3(x, y, z);
    return *this;
  }

  ObjectBuilder& scale(float uniform) {
    object.scale = glm::vec3(uniform);
    return *this;
  }

  ObjectBuilder& mesh(MeshID meshID) {
    object.meshID = meshID;
    return *this;
  }

  ObjectBuilder& material(MaterialID materialID) {
    object.materialID = materialID;
    return *this;
  }

  ObjectBuilder& visible(bool isVisible) {
    object.visible = isVisible;
    return *this;
  }

  Object build() {
    if (object.meshID == INVALID_MESH_ID) {
      Debug::log(Debug::Category::RENDERING,
                 "ObjectBuilder: Warning - building object '", object.name,
                 "' with invalid mesh ID");
    }
    if (object.materialID == INVALID_MATERIAL_ID) {
      Debug::log(Debug::Category::RENDERING,
                 "ObjectBuilder: Warning - building object '", object.name,
                 "' with invalid material ID");
    }
    Debug::log(Debug::Category::RENDERING, "ObjectBuilder: Built object '",
               object.name, "' (Mesh: ", object.meshID,
               ", Material: ", object.materialID, ")");
    return std::move(object);
  }

 private:
  Object object;
};