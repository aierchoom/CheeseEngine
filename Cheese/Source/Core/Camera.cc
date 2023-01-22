#include "Camera.h"

using namespace DirectX;

XMFLOAT3 Camera::GetPostion() const { return mTransform.GetPosition(); }
XMVECTOR Camera::GetPositionXM() const { return mTransform.GetPositionXM(); }

void Camera::SetPosition(XMFLOAT3 position) { mTransform.SetPosition(position); }
void Camera::SetPosition(float x, float y, float z)
{
  XMFLOAT3 position = {x, y, z};
  SetPosition(position);
}

float Camera::GetRotationX() const { return mTransform.GetRotation().x; }

float Camera::GetRotationY() const { return mTransform.GetRotation().y; }

XMFLOAT3 Camera::GetRightAxis() const { return mTransform.GetRightAxis(); }

XMVECTOR Camera::GetRightAxisXM() const { return mTransform.GetRightAxisXM(); }

XMFLOAT3 Camera::GetUpAxis() const { return mTransform.GetUpAxis(); }

XMVECTOR Camera::GetUpAxisXM() const { return mTransform.GetUpAxisXM(); }

XMFLOAT3 Camera::GetLookAxis() const { return mTransform.GetForwardAxis(); }

XMVECTOR Camera::GetLookAxisXM() const { return mTransform.GetForwardAxisXM(); }

XMMATRIX Camera::GetLocalToWorldMatrixXM() const { return mTransform.GetLocalToWorldMatrixXM(); }

XMMATRIX Camera::GetViewMatrixXM() const { return mTransform.GetWorldToLocalMatrixXM(); }

XMMATRIX Camera::GetProjMatrixXM() const { return XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ); }

XMMATRIX Camera::GetViewProjMatrixXM() const { return GetViewMatrixXM() * GetProjMatrixXM(); }

D3D12_VIEWPORT Camera::GetViewPort() const { return mViewPort; }

float Camera::GetNearZ() const { return mNearZ; }

float Camera::GetFarZ() const { return mFarZ; }

float Camera::GetFovY() const { return mFovY; }

float Camera::GetAspectRatio() const { return mAspect; }

void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
  mFovY   = fovY;
  mAspect = aspect;
  mNearZ  = nearZ;
  mFarZ   = farZ;
}

void Camera::SetViewPort(const D3D12_VIEWPORT& viewPort) { mViewPort = viewPort; }

void Camera::SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
{
  mViewPort.TopLeftX = topLeftX;
  mViewPort.TopLeftY = topLeftY;
  mViewPort.Width    = width;
  mViewPort.Height   = height;
  mViewPort.MinDepth = minDepth;
  mViewPort.MaxDepth = maxDepth;
}

void Camera::LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
{
  mTransform.SetPosition(pos);
  mTransform.LookAt(target, up);
}

void Camera::LookTo(const XMFLOAT3& pos, const XMFLOAT3& to, const XMFLOAT3& up)
{
  mTransform.SetPosition(pos);
  mTransform.LookTo(to, up);
}

void Camera::MoveForward(float distance) { mTransform.Translate(mTransform.GetForwardAxis(), distance); }

void Camera::MoveRight(float distance) { mTransform.Translate(mTransform.GetRightAxis(), distance); }

void Camera::AddYaw(float radian)
{
  XMFLOAT3 rotation = mTransform.GetRotation();
  rotation.y        = XMScalarModAngle(rotation.y + radian);
  mTransform.SetRotation(rotation);
}

void Camera::AddPitch(float radian)
{
  XMFLOAT3 rotation = mTransform.GetRotation();
  rotation.x += radian;
  mTransform.SetRotation(rotation);
}
