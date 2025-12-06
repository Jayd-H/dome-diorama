#include "Object.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Debug.h"

Object::Object()
    : name("Unnamed Object"),
      position(0.0f, 0.0f, 0.0f),
      rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      scale(1.0f, 1.0f, 1.0f),
      meshID(INVALID_MESH_ID),
      materialID(INVALID_MATERIAL_ID),
      visible(true) {}

glm::mat4 Object::getModelMatrix() const {
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  model = model * glm::mat4_cast(rotation);
  model = glm::scale(model, scale);
  return model;
}

void Object::setPosition(const glm::vec3& pos) { position = pos; }

void Object::setPosition(float x, float y, float z) {
  position = glm::vec3(x, y, z);
}

void Object::setRotation(const glm::quat& rot) { rotation = rot; }

void Object::setRotationEuler(float pitch, float yaw, float roll) {
  rotation = glm::quat(
      glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
}

void Object::setRotationEuler(const glm::vec3& euler) {
  rotation = glm::quat(glm::radians(euler));
}

void Object::setScale(const glm::vec3& s) { scale = s; }

void Object::setScale(float x, float y, float z) { scale = glm::vec3(x, y, z); }

void Object::setScale(float uniform) { scale = glm::vec3(uniform); }

void Object::setMesh(MeshID mesh) { meshID = mesh; }

void Object::setMaterial(MaterialID material) { materialID = material; }

ObjectBuilder::ObjectBuilder() { object = Object(); }

ObjectBuilder& ObjectBuilder::name(const std::string& name) {
  object.name = name;
  return *this;
}

ObjectBuilder& ObjectBuilder::position(const glm::vec3& pos) {
  object.position = pos;
  return *this;
}

ObjectBuilder& ObjectBuilder::position(float x, float y, float z) {
  object.position = glm::vec3(x, y, z);
  return *this;
}

ObjectBuilder& ObjectBuilder::rotation(const glm::quat& rot) {
  object.rotation = rot;
  return *this;
}

ObjectBuilder& ObjectBuilder::rotationEuler(float pitch, float yaw,
                                            float roll) {
  object.rotation = glm::quat(
      glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
  return *this;
}

ObjectBuilder& ObjectBuilder::rotationEuler(const glm::vec3& euler) {
  object.rotation = glm::quat(glm::radians(euler));
  return *this;
}

ObjectBuilder& ObjectBuilder::scale(const glm::vec3& s) {
  object.scale = s;
  return *this;
}

ObjectBuilder& ObjectBuilder::scale(float x, float y, float z) {
  object.scale = glm::vec3(x, y, z);
  return *this;
}

ObjectBuilder& ObjectBuilder::scale(float uniform) {
  object.scale = glm::vec3(uniform);
  return *this;
}

ObjectBuilder& ObjectBuilder::mesh(MeshID meshID) {
  object.meshID = meshID;
  return *this;
}

ObjectBuilder& ObjectBuilder::material(MaterialID materialID) {
  object.materialID = materialID;
  return *this;
}

ObjectBuilder& ObjectBuilder::visible(bool isVisible) {
  object.visible = isVisible;
  return *this;
}

Object ObjectBuilder::build() {
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

  return object;
}