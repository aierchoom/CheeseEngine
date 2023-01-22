#ifndef CORE_CAMERA_H
#define CORE_CAMERA_H
#include <DirectXMath.h>
#include <d3d12.h>
#include "Math/Transform.h"

class Camera
{
 public:
  Camera() = default;
  ~Camera() {}

  DirectX::XMFLOAT3 GetPostion() const;
  DirectX::XMVECTOR GetPositionXM() const;

  void SetPosition(DirectX::XMFLOAT3 position);
  void SetPosition(float x, float y, float z);

  float GetRotationX() const;
  float GetRotationY() const;

  DirectX::XMFLOAT3 GetRightAxis() const;
  DirectX::XMVECTOR GetRightAxisXM() const;

  DirectX::XMFLOAT3 GetUpAxis() const;
  DirectX::XMVECTOR GetUpAxisXM() const;

  DirectX::XMFLOAT3 GetLookAxis() const;
  DirectX::XMVECTOR GetLookAxisXM() const;

  DirectX::XMMATRIX GetLocalToWorldMatrixXM() const;
  DirectX::XMMATRIX GetViewMatrixXM() const;
  DirectX::XMMATRIX GetProjMatrixXM() const;
  DirectX::XMMATRIX GetViewProjMatrixXM() const;

  D3D12_VIEWPORT GetViewPort() const;

  float GetNearZ() const;
  float GetFarZ() const;
  float GetFovY() const;
  float GetAspectRatio() const;

  void SetFrustum(float fovY, float aspect, float nearZ, float farZ);

  void SetViewPort(const D3D12_VIEWPORT& viewPort);
  void SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

  void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up = {0.0f, 1.0f, 0.0f});
  void LookTo(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT3& up);

  void MoveForward(float distance);
  void MoveRight(float distance);

  void AddYaw(float radian);
  void AddPitch(float radian);

 private:
  Transform mTransform = {};

  float mNearZ  = 0.0f;
  float mFarZ   = 0.0f;
  float mAspect = 0.0f;
  float mFovY   = 0.0f;

  D3D12_VIEWPORT mViewPort = {};
};

#endif  // CORE_CAMERA_H
