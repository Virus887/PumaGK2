#pragma once

#include <DirectXMath.h>

# define M_PI_2		1.57079632679489661923

class CameraFps
{
private:
	DirectX::XMFLOAT3 position;

	DirectX::XMFLOAT3 direction;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;

	float pitch;
	float yaw;
	float sensitivity;
	float movementSpeed;

	DirectX::XMMATRIX viewMtx;
public:
	CameraFps();
	~CameraFps();

	void MoveForward(float speedBoost = 1);
	void MoveBackward(float speedBoost = 1);

	void MoveLeft(float speedBoost = 1);
	void MoveRight(float speedBoost = 1);

	void MoveUp(float speedBoost = 1);
	void MoveDown(float speedBoost = 1);

	void Rotate(float dx, float dy);

	DirectX::XMFLOAT3 GetPosition();

	void SetSensitivity(float sens);
	float GetSensitivity();

	const DirectX::XMMATRIX& GetViewMatrix() const;

	void Update();
};