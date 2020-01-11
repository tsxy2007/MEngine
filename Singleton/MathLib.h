#pragma once
#include "../Common/d3dUtil.h"

const float Deg2Rad = 0.0174532924;
const float Rad2Deg = 57.29578;
class MathLib final
{

public:
	static DirectX::XMVECTOR GetPlane(
		DirectX::XMVECTOR& normal,
		DirectX::XMVECTOR& inPoint);
	static DirectX::XMVECTOR GetPlane(
		DirectX::XMVECTOR& a,
		DirectX::XMVECTOR& b, 
		DirectX::XMVECTOR& c);
	static bool BoxIntersect(
		DirectX::XMMATRIX& localToWorldMatrix,
		DirectX::XMVECTOR* planes,
		DirectX::XMVECTOR position,
		DirectX::XMVECTOR localExtent);
	static void GetCameraNearPlanePoints(
		DirectX::XMMATRIX& localToWorldMatrix,
		double fov,
		double aspect,
		double distance,
		DirectX::XMVECTOR* corners
	);

	static void GetPerspFrustumPlanes(
		DirectX::XMMATRIX& localToWorldMatrix,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		DirectX::XMFLOAT4* frustumPlanes
	);
	static void GetFrustumBoundingBox(
		DirectX::XMMATRIX& localToWorldMatrix,
		double nearWindowHeight,
		double farWindowHeight,
		double aspect,
		double nearZ,
		double farZ, 
		DirectX::XMVECTOR* minValue,
		DirectX::XMVECTOR* maxValue
	);
};