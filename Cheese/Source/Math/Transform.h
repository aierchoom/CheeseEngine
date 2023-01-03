#ifndef MATH_TRANSFORM_H
#define MATH_TRANSFORM_H

#include <DirectXMath.h>

class Transform
{
 public:
  Transform() = default;
  Transform(const DirectX::XMFLOAT3& scale, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& position)
      : mScale(scale), mPosition(position)
  {
    SetRotation(rotation);
  }

  ~Transform() = default;

  Transform(const Transform&)            = default;
  Transform& operator=(const Transform&) = default;

  Transform(Transform&&)            = default;
  Transform& operator=(Transform&&) = default;

  DirectX::XMFLOAT3 GetScale() const { return mScale; }
  DirectX::XMVECTOR GetScaleXM() const { return DirectX::XMLoadFloat3(&mScale); }

  DirectX::XMFLOAT3 GetRotation() const
  {
    float sinX     = 2 * (mRotation.w * mRotation.x - mRotation.y * mRotation.z);
    float sinYCosX = 2 * (mRotation.w * mRotation.y + mRotation.x * mRotation.z);
    float cosYCosX = 1 - 2 * (mRotation.x * mRotation.x + mRotation.y * mRotation.y);
    float sinZCosX = 2 * (mRotation.w * mRotation.z + mRotation.x * mRotation.y);
    float cosZCosX = 1 - 2 * (mRotation.x * mRotation.x + mRotation.z * mRotation.z);

    DirectX::XMFLOAT3 rotation;
    if (fabs(sinX) >= 1.0f)
      rotation.x = copysignf(DirectX::XM_PI / 2, sinX);
    else
      rotation.x = asinf(sinX);
    rotation.y = atan2f(sinYCosX, cosYCosX);
    rotation.z = atan2f(sinZCosX, cosZCosX);

    return rotation;
  }

  DirectX::XMVECTOR GetRotationXM() const
  {
    auto rotation = GetRotation();
    return DirectX::XMLoadFloat3(&rotation);
  }

  DirectX::XMFLOAT3 GetPosition() const { return mPosition; }
  DirectX::XMVECTOR GetPositionXM() const { return DirectX::XMLoadFloat3(&mPosition); }

  DirectX::XMFLOAT3 GetRightAxis() const
  {
    using namespace DirectX;
    XMMATRIX R = XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&mRotation));
    XMFLOAT3 right;
    XMStoreFloat3(&right, R.r[0]);
    return right;
  }

  DirectX::XMVECTOR GetRightAxisXM() const
  {
    DirectX::XMFLOAT3 right = GetRightAxis();
    return DirectX::XMLoadFloat3(&right);
  }

  DirectX::XMFLOAT3 GetUpAxis() const
  {
    using namespace DirectX;
    XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&mRotation));
    XMFLOAT3 up;
    XMStoreFloat3(&up, R.r[1]);
    return up;
  }

  DirectX::XMVECTOR GetUpAxisXM() const
  {
    DirectX::XMFLOAT3 up = GetUpAxis();
    return DirectX::XMLoadFloat3(&up);
  }

  DirectX::XMFLOAT3 GetForwardAxis() const
  {
    using namespace DirectX;
    XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&mRotation));
    XMFLOAT3 forward;
    XMStoreFloat3(&forward, R.r[2]);
    return forward;
  }

  DirectX::XMVECTOR GetForwardAxisXM() const
  {
    DirectX::XMFLOAT3 forward = GetForwardAxis();
    return DirectX::XMLoadFloat3(&forward);
  }

  DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const
  {
    DirectX::XMFLOAT4X4 res;
    DirectX::XMStoreFloat4x4(&res, GetLocalToWorldMatrixXM());
    return res;
  }

  DirectX::XMMATRIX GetLocalToWorldMatrixXM() const
  {
    using namespace DirectX;
    DirectX::XMVECTOR scale      = XMLoadFloat3(&mScale);
    DirectX::XMVECTOR quaternion = XMLoadFloat4(&mRotation);
    DirectX::XMVECTOR position   = XMLoadFloat3(&mPosition);
    DirectX::XMMATRIX world      = XMMatrixAffineTransformation(scale, g_XMZero, quaternion, position);
    return world;
  }

  DirectX::XMFLOAT4X4 GetWorldToLocalMatrix() const
  {
    DirectX::XMFLOAT4X4 result;
    DirectX::XMStoreFloat4x4(&result, GetWorldToLocalMatrixXM());
  }

  DirectX::XMMATRIX GetWorldToLocalMatrixXM() const
  {
    DirectX::XMMATRIX inverseWorld = DirectX::XMMatrixInverse(nullptr, GetLocalToWorldMatrixXM());
    return inverseWorld;
  }

  void SetScale(const DirectX::XMFLOAT3& scale) { mScale = scale; }
  void SetScale(float x, float y, float z) { mScale = DirectX::XMFLOAT3(x, y, z); }

  void SetRotation(const DirectX::XMFLOAT3& eulerAnglesInRadian)
  {
    auto quaternion = DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&eulerAnglesInRadian));
    DirectX::XMStoreFloat4(&mRotation, quaternion);
  }

  void SetRotation(float x, float y, float z)
  {
    auto quaternion = DirectX::XMQuaternionRotationRollPitchYaw(x, y, z);
    DirectX::XMStoreFloat4(&mRotation, quaternion);
  }

  void SetPosition(const DirectX::XMFLOAT3& position) { mPosition = position; }
  void SetPosition(float x, float y, float z) { mPosition = DirectX::XMFLOAT3(x, y, z); }

  void Rotate(const DirectX::XMFLOAT3& eulerAnglesInRadian)
  {
    using namespace DirectX;
    auto newQuaternion = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&eulerAnglesInRadian));
    auto quaternion    = XMLoadFloat4(&mRotation);
    XMStoreFloat4(&mRotation, XMQuaternionMultiply(quaternion, newQuaternion));
  }

  void RotateAxis(const DirectX::XMFLOAT3& axis, float radian)
  {
    using namespace DirectX;
    auto newQuaternion = XMQuaternionRotationAxis(XMLoadFloat3(&axis), radian);
    auto quaternion    = XMLoadFloat4(&mRotation);
    XMStoreFloat4(&mRotation, XMQuaternionMultiply(quaternion, newQuaternion));
  }

  void RotateAround(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& axis, float radian)
  {
    using namespace DirectX;
    XMVECTOR quaternion = XMLoadFloat4(&mRotation);
    XMVECTOR position   = XMLoadFloat3(&mPosition);
    XMVECTOR center     = XMLoadFloat3(&point);

    XMMATRIX RT = XMMatrixRotationQuaternion(quaternion) * XMMatrixTranslationFromVector(position - center);
    RT *= XMMatrixRotationAxis(XMLoadFloat3(&axis), radian);
    RT *= XMMatrixTranslationFromVector(center);
    XMStoreFloat4(&mRotation, XMQuaternionRotationMatrix(RT));
    XMStoreFloat3(&mPosition, RT.r[3]);
  }

  void Translate(const DirectX::XMFLOAT3& direction, float magnitude)
  {
    using namespace DirectX;
    XMVECTOR directionVec = XMVector3Normalize(XMLoadFloat3(&direction));
    XMVECTOR newPosition  = XMVectorMultiplyAdd(XMVectorReplicate(magnitude), directionVec, XMLoadFloat3(&mPosition));
    XMStoreFloat3(&mPosition, newPosition);
  }

  void LookAt(const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up = {0.0f, 1.0f, 0.0f})
  {
    using namespace DirectX;
    XMMATRIX view    = XMMatrixLookAtLH(XMLoadFloat3(&mPosition), XMLoadFloat3(&target), XMLoadFloat3(&up));
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMStoreFloat4(&mRotation, XMQuaternionRotationMatrix(invView));
  }

  void LookTo(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& up = {0.0f, 1.0f, 0.0f})
  {
    using namespace DirectX;
    XMMATRIX view    = XMMatrixLookToLH(XMLoadFloat3(&mPosition), XMLoadFloat3(&direction), XMLoadFloat3(&up));
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMStoreFloat4(&mRotation, XMQuaternionRotationMatrix(invView));
  }

 private:
  DirectX::XMFLOAT3 mScale    = {1.0, 1.0f, 1.0f};
  DirectX::XMFLOAT4 mRotation = {0.0f, 0.0f, 0.0f, 1.0f};
  DirectX::XMFLOAT3 mPosition = {0.0f, 0.0f, 0.0f};
};
#endif  // MATH_TRANSFORM_H