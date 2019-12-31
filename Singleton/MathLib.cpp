#include "MathLib.h"
using namespace DirectX;
#define GetVec(name, v) XMVECTOR name = XMLoadFloat3(&##v);
#define StoreVec(ptr, v) XMStoreFloat3(ptr, v);
XMVECTOR mul(XMMATRIX& mat, XMVECTOR& vec)
{
	return XMVector4Transform(vec, XMMatrixTranspose(mat));
}
XMVECTOR mul(XMVECTOR& vec, XMMATRIX& mat)
{
	return XMVector4Transform(vec, mat);
}
DirectX::XMVECTOR MathLib::GetPlane(XMVECTOR& a, XMVECTOR& b, XMVECTOR& c)
{
	XMVECTOR normal = XMVector3Normalize(XMVector3Cross(b - a, c - a));
	XMVECTOR disVec = XMVector3Dot(normal, a);
	XMFLOAT4 result;
	XMStoreFloat((float*)&result.w, disVec);
	XMStoreFloat3((XMFLOAT3*)&result, normal);
	return XMLoadFloat4(&result);
}



XMVECTOR MathLib::GetPlane(XMVECTOR normal, XMVECTOR inPoint)
{
	XMVECTOR dt = -XMVector3Dot(normal, inPoint);
	XMFLOAT4 result;
	XMStoreFloat3((XMFLOAT3*)&result, normal);
	XMStoreFloat((float*)&result.w, dt);
	return XMLoadFloat4(&result);;
}

void MathLib::GetFrustumCorner(PerspCam& perspCam, float distance, XMVECTOR* corners)
{
	float fov = Deg2Rad * perspCam.fov * 0.5f;
	float upLength = distance * tan(fov);
	float rightLength = upLength * perspCam.aspect;
	XMVECTOR farPoint = perspCam.position + distance * perspCam.forward;
	XMVECTOR upVec = upLength * perspCam.up;
	XMVECTOR rightVec = rightLength * perspCam.forward;
	corners[0] = farPoint - upVec - rightVec;
	corners[1] = farPoint - upVec + rightVec;
	corners[2] = farPoint + upVec - rightVec;
	corners[3] = farPoint + upVec + rightVec;
}

void MathLib::GetFrustumCorner(OrthoCam& orthoCam, float distance, XMVECTOR* corners)
{
	XMVECTOR farPoint = orthoCam.position + distance * orthoCam.forward;
	XMVECTOR upPosVec = orthoCam.size * orthoCam.up;
	XMVECTOR rightPosVec = orthoCam.size * orthoCam.right;
	corners[0] = farPoint - upPosVec - rightPosVec;
	corners[1] = farPoint - upPosVec + rightPosVec;
	corners[2] = farPoint + upPosVec - rightPosVec;
	corners[3] = farPoint + upPosVec + rightPosVec;
}

void MathLib::GetFrustumPlanes(PerspCam& perspCam, XMVECTOR* planes)
{
	XMVECTOR corners[4];
	GetFrustumCorner(perspCam, perspCam.farClipPlane, corners);
	planes[0] = GetPlane(corners[1], corners[0], perspCam.position);
	planes[1] = GetPlane(corners[2], corners[3], perspCam.position);
	planes[2] = GetPlane(corners[0], corners[2], perspCam.position);
	planes[3] = GetPlane(corners[3], corners[1], perspCam.position);
	XMVECTOR farPointVec = perspCam.position + perspCam.forward * perspCam.farClipPlane;
	XMVECTOR nearPointVec = perspCam.position + perspCam.forward * perspCam.nearClipPlane;
	planes[4] = GetPlane(perspCam.forward, farPointVec);
	planes[5] = GetPlane(-perspCam.forward, nearPointVec);
}

bool MathLib::BoxIntersect(DirectX::XMMATRIX& localToWorldMatrix, DirectX::XMVECTOR* planes, DirectX::XMVECTOR position, XMVECTOR localExtent)
{
	XMMATRIX matrixTranspose = XMMatrixTranspose(localToWorldMatrix);
	XMVECTOR pos = XMVector3TransformCoord(position, matrixTranspose);
	for (UINT i = 0; i < 6; ++i)
	{
		XMVECTOR plane = planes[i];
		XMVECTOR absNormal = XMVectorAbs(XMVector3TransformNormal(plane, matrixTranspose));
		XMVECTOR result = XMVector3Dot(pos, plane) - XMVector3Dot(absNormal, localExtent);
		float dist = 0; XMFLOAT4 planeF;
		XMStoreFloat(&dist, result);
		XMStoreFloat4(&planeF, plane);
		if (dist > -planeF.w) return false;
	}
	return true;
}