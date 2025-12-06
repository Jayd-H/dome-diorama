#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

#include "Material.h"

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

  Object();

  glm::mat4 getModelMatrix() const;

  void setPosition(const glm::vec3& pos);
  void setPosition(float x, float y, float z);

  void setRotation(const glm::quat& rot);
  void setRotationEuler(float pitch, float yaw, float roll);
  void setRotationEuler(const glm::vec3& euler);

  void setScale(const glm::vec3& s);
  void setScale(float x, float y, float z);
  void setScale(float uniform);

  void setMesh(MeshID mesh);
  void setMaterial(MaterialID material);
};

class ObjectBuilder {
 public:
  ObjectBuilder();

  ObjectBuilder& name(const std::string& name);

  ObjectBuilder& position(const glm::vec3& pos);
  ObjectBuilder& position(float x, float y, float z);

  ObjectBuilder& rotation(const glm::quat& rot);
  ObjectBuilder& rotationEuler(float pitch, float yaw, float roll);
  ObjectBuilder& rotationEuler(const glm::vec3& euler);

  ObjectBuilder& scale(const glm::vec3& s);
  ObjectBuilder& scale(float x, float y, float z);
  ObjectBuilder& scale(float uniform);

  ObjectBuilder& mesh(MeshID meshID);
  ObjectBuilder& material(MaterialID materialID);

  ObjectBuilder& visible(bool isVisible);

  Object build();

 private:
  Object object;
};