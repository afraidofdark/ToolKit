#include "stdafx.h"
#include "Directional.h"
#include "Node.h"
#include "DebugNew.h"

namespace ToolKit
{

  Directional::Directional()
  {
  }

  Directional::~Directional()
  {
  }

  void Directional::Pitch(float angle)
  {
    Quaternion q = glm::angleAxis(angle, Vec3(1.0f, 0.0f, 0.0f));
    m_node->Rotate(q, TransformationSpace::TS_LOCAL);
  }

  void Directional::Yaw(float angle)
  {
    Quaternion q = glm::angleAxis(angle, Vec3(0.0f, 1.0f, 0.0f));
    m_node->Rotate(q, TransformationSpace::TS_LOCAL);
  }

  void Directional::Roll(float angle)
  {
    Quaternion q = glm::angleAxis(angle, Vec3(0.0f, 0.0f, 1.0f));
    m_node->Rotate(q, TransformationSpace::TS_LOCAL);
  }

  void Directional::Translate(Vec3 pos)
  {
    m_node->Translate(pos, TransformationSpace::TS_LOCAL);
  }

  void Directional::RotateOnUpVector(float angle)
  {
    m_node->Rotate(glm::angleAxis(angle, Vec3(0.0f, 1.0f, 0.0f)), TransformationSpace::TS_WORLD);
  }

  void Directional::GetLocalAxis(Vec3& dir, Vec3& up, Vec3& right) const
  {
    Mat4 transform = m_node->GetTransform(TransformationSpace::TS_WORLD);
    right = glm::column(transform, 0);
    up = glm::column(transform, 1);
    dir = -glm::column(transform, 2);
  }

  Vec3 Directional::GetDir() const
  {
    Mat4 transform = m_node->GetTransform(TransformationSpace::TS_WORLD);
    return -glm::column(transform, 2);
  }

  Vec3 Directional::GetUp() const
  {
    Mat4 transform = m_node->GetTransform(TransformationSpace::TS_WORLD);
    return glm::column(transform, 1);
  }

  Vec3 Directional::GetRight() const
  {
    Mat4 transform = m_node->GetTransform(TransformationSpace::TS_WORLD);
    return glm::column(transform, 0);
  }

  EntityType Directional::GetType() const
  {
    return EntityType::Entity_Directional;
  }

  Camera::Camera()
  {
    SetLens(glm::radians(90.0f), 640.0f, 480.0f, 0.01f, 1000.0f);
  }

  Camera::~Camera()
  {
  }

  void Camera::SetLens(float fov, float width, float height)
  {
    SetLens(fov, width, height, 0.01f, 1000.0f);
  }

  void Camera::SetLens(float fov, float width, float height, float near, float far)
  {
    m_projection = glm::perspectiveFov(fov, width, height, near, far);
    m_fov = fov;
    m_aspect = width / height;
    m_near = near;
    m_height = height;
    m_ortographic = false;
  }

  void Camera::SetLens(float aspect, float left, float right, float bottom, float top, float near, float far)
  {
    m_projection = glm::ortho(left * aspect, right * aspect, bottom, top, near, far);
    m_fov = 0.0f;
    m_aspect = aspect;
    m_near = near;
    m_height = top - bottom;
    m_ortographic = true;
  }

  Mat4 Camera::GetViewMatrix() const
  {
    Mat4 view = m_node->GetTransform(TransformationSpace::TS_WORLD);
    return glm::inverse(view);
  }

  bool Camera::IsOrtographic() const
  {
    return m_ortographic;
  }

  void Camera::LookAt(Vec3 target)
  {
    Vec3 eye = m_node->GetTranslation(TransformationSpace::TS_WORLD);
    Vec3 tdir = target - eye;
    tdir.y = 0.0f; // project on xz
    tdir = glm::normalize(tdir);
    Vec3 dir = GetDir();
    dir.y = 0.0f; // project on xz
    dir = glm::normalize(dir);
    Vec3 rotAxis = glm::normalize(glm::cross(dir, tdir));
    float yaw = glm::acos(glm::dot(tdir, dir));

    yaw *= glm::sign(glm::dot(Y_AXIS, rotAxis));
    RotateOnUpVector(yaw);

    tdir = target - eye;
    tdir = glm::normalize(tdir);
    dir = glm::normalize(GetDir());
    rotAxis = glm::normalize(glm::cross(dir, tdir));
    float pitch = glm::acos(glm::dot(tdir, dir));
    pitch *= glm::sign(glm::dot(GetRight(), rotAxis));
    Pitch(pitch);

    // Check upside down case
    if (glm::dot(GetUp(), Y_AXIS) < 0.0f)
    {
      Roll(glm::pi<float>());
    }
  }

  Camera::CamData Camera::GetData() const
  {
    CamData data;
    data.dir = GetDir();
    data.pos = m_node->GetTranslation(TransformationSpace::TS_WORLD);
    data.projection = m_projection;
    data.fov = m_fov;
    data.aspect = m_aspect;
    data.nearDist = m_near;
    data.height = m_height;
    data.ortographic = m_ortographic;

    return data;
  }

  EntityType Camera::GetType() const
  {
    return EntityType::Entity_Camera;
  }

  Light::Light()
  {
    m_color = Vec3(1.0f, 1.0f, 1.0f);
    m_intensity = 1.0f;
  }

  Light::~Light()
  {
  }

  Light::LightData Light::GetData() const
  {
    LightData data;
    data.dir = GetDir();
    data.pos = m_node->GetTranslation(TransformationSpace::TS_WORLD);
    data.color = m_color;
    data.intensity = m_intensity;

    return data;
  }

  EntityType Light::GetType() const
  {
    return EntityType::Entity_Light;
  }

}
