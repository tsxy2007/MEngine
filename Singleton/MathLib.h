#pragma once
#include "../Common/d3dUtil.h"
struct PerspCam
{
	DirectX::XMVECTOR right;
	DirectX::XMVECTOR up;
	DirectX::XMVECTOR forward;
	DirectX::XMVECTOR position;
	float fov;
	float nearClipPlane;
	float farClipPlane;
	float aspect;
};
struct OrthoCam
{
	DirectX::XMVECTOR right;
	DirectX::XMVECTOR up;
	DirectX::XMVECTOR forward;
	DirectX::XMVECTOR position;
	float size;
	float nearClipPlane;
	float farClipPlane;
};
const float Deg2Rad = 0.0174532924;
const float Rad2Deg = 57.29578;
class MathLib final
{

public:
	static void GetFrustumCorner(PerspCam& perspCam, float distance, DirectX::XMVECTOR* corners);
	static void GetFrustumCorner(OrthoCam& orthoCam, float distance, DirectX::XMVECTOR* corners);
	static void GetFrustumPlanes(PerspCam& perspCam, DirectX::XMVECTOR* planes);
	static DirectX::XMVECTOR GetPlane(DirectX::XMVECTOR normal, DirectX::XMVECTOR inPoint);
	static DirectX::XMVECTOR GetPlane(DirectX::XMVECTOR& a, DirectX::XMVECTOR& b, DirectX::XMVECTOR& c);
	static bool BoxIntersect(DirectX::XMMATRIX& localToWorldMatrix, DirectX::XMVECTOR* planes, DirectX::XMVECTOR position, DirectX::XMVECTOR localExtent);
};