#include "cameraFps.h"
#include <math.h>
#include <iostream>

using namespace DirectX;

CameraFps::CameraFps()
{
	position.x = 0.0f;
	position.y = 1.0f;
	position.z = 0.0f;

	pitch = 0;
	yaw = 0;

	sensitivity = 0.00005;
	movementSpeed = 0.1;
}

CameraFps::~CameraFps()
{
}

XMFLOAT3 CameraFps::GetPosition()
{
	return position;
}

void CameraFps::MoveForward(float speedBoost) {
	position.x += direction.x * movementSpeed * speedBoost;
	position.y += direction.y * movementSpeed * speedBoost;
	position.z += direction.z * movementSpeed * speedBoost;

}

void CameraFps::MoveBackward(float speedBoost) {
	position.x -= direction.x * movementSpeed * speedBoost;
	position.y -= direction.y * movementSpeed * speedBoost;
	position.z -= direction.z * movementSpeed * speedBoost;
}

void CameraFps::MoveLeft(float speedBoost) {
	position.x -= right.x * movementSpeed * speedBoost;
	position.y -= right.y * movementSpeed * speedBoost;
	position.z -= right.z * movementSpeed * speedBoost;
}

void CameraFps::MoveRight(float speedBoost) {
	position.x += right.x * movementSpeed * speedBoost;
	position.y += right.y * movementSpeed * speedBoost;
	position.z += right.z * movementSpeed * speedBoost;
}

void CameraFps::MoveUp(float speedBoost) {
	position.x += up.x * movementSpeed * speedBoost;
	position.y += up.y * movementSpeed * speedBoost;
	position.z += up.z * movementSpeed * speedBoost;
}

void CameraFps::MoveDown(float speedBoost) {
	position.x -= up.x * movementSpeed * speedBoost;
	position.y -= up.y * movementSpeed * speedBoost;
	position.z -= up.z * movementSpeed * speedBoost;
}

void CameraFps::Rotate(float dYaw, float dPitch) {
	pitch += dPitch * sensitivity;
	if (pitch < -XM_PIDIV2) {
		pitch = -XM_PIDIV2;
	}
	if (pitch > XM_PIDIV2) {
		pitch = XM_PIDIV2;
	}

	yaw += dYaw * sensitivity;
	if (yaw > XM_PI)
		yaw = -XM_PI + (yaw - XM_PI);
	else if (yaw < -XM_PI)
		yaw = XM_PI - (-XM_PI + yaw);
}

void CameraFps::SetSensitivity(float val) {
	this -> sensitivity = val;
}

float CameraFps::GetSensitivity() {
	return sensitivity;
}

const XMMATRIX& CameraFps::GetViewMatrix() const {
	return this->viewMtx;
}

void CameraFps::Update() {
	XMVECTOR positionVec = XMLoadFloat3(&position);

	// Calculate the rotation matrix
	XMMATRIX mtxRotation = XMMatrixRotationRollPitchYaw(pitch, yaw, 0);

	// Calculate the new target
	XMVECTOR forwardVec = XMVector3Transform(XMVectorSet(0, 0, 1, 0), mtxRotation);
	XMVECTOR upVec = XMVector3Transform(XMVectorSet(0, 1, 0, 0), mtxRotation);
	XMVECTOR rightVec = XMVector3Transform(XMVectorSet(1, 0, 0, 0), mtxRotation);
	XMVECTOR vecTarget = positionVec + forwardVec;

	XMStoreFloat3(&position, positionVec);
	XMStoreFloat3(&direction, forwardVec);
	XMStoreFloat3(&up, upVec);
	XMStoreFloat3(&right, rightVec);

	// View matrix
	viewMtx = XMMatrixLookAtLH(positionVec, vecTarget, upVec);
}