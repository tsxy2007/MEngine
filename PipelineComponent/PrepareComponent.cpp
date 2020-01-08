#include "PrepareComponent.h"
#include "../Common/Camera.h"
#include "../RenderComponent/UploadBuffer.h"
#include "CameraData/CameraTransformData.h"
#include "../Common/Random.h"
#include "../Singleton/MathLib.h"
using namespace DirectX;
UINT sampleIndex;
const UINT k_SampleCount = 8;
double GetHalton(int index, int radix)
{
	double result = 0;
	double fraction = 1.0 / (double)radix;

	while (index > 0)
	{
		result += (double)(index % radix) * fraction;

		index /= radix;
		fraction /= (double)radix;
	}

	return result;
}
XMVECTOR GenerateRandomOffset()
{
	XMVECTOR offset = {
		GetHalton((sampleIndex & 1023) + 1, 2) - 0.5f,
		GetHalton((sampleIndex & 1023) + 1, 3) - 0.5f,
		0, 0
	};

	if (++sampleIndex >= k_SampleCount)
		sampleIndex = 0;

	return offset;
}

void GetJitteredPerspectiveProjectionMatrix(
	double nearClipPlane,
	double farClipPlane,
	double fieldOfView,
	double aspect,
	UINT pixelWidth, UINT pixelHeight,
	XMFLOAT2 offset, XMMATRIX* projectionMatrix)
{
	double nearPlane = nearClipPlane;
	double vertical = tan(0.5 * Deg2Rad * fieldOfView) * nearPlane;
	double horizontal = vertical * aspect;

	offset.x *= horizontal / (0.5f * pixelWidth);
	offset.y *= vertical / (0.5f * pixelHeight);

	XMFLOAT4* firstRow = (XMFLOAT4*)&projectionMatrix->r[0];
	XMFLOAT4* secondRow = (XMFLOAT4*)&projectionMatrix->r[1];
	firstRow->z += offset.x / horizontal;
	secondRow->z -= offset.y / vertical;		//TODD: dont know difference between ogl & dx
}
void GetJitteredProjectionMatrix(
	double nearClipPlane,
	double farClipPlane,
	double fieldOfView,
	double aspect,
	double jitterOffset,
	UINT pixelWidth, UINT pixelHeight,
	XMMATRIX& projectionMatrix,
	XMVECTOR& jitter)
{
	jitter = GenerateRandomOffset();
	jitter *= jitterOffset;
	XMFLOAT2* jitterPtr = (XMFLOAT2*)&jitter;
	GetJitteredPerspectiveProjectionMatrix(
		nearClipPlane, farClipPlane,
		fieldOfView,
		aspect,
		pixelWidth, pixelHeight,
		*jitterPtr,
		&projectionMatrix
	);
	*jitterPtr = { jitterPtr->x / pixelWidth, jitterPtr->y / pixelHeight };
}
void ConfigureJitteredProjectionMatrix(Camera* camera, UINT height, UINT width, double jitterOffset, CameraTransformData* data, XMVECTOR& jitter)
{
	data->nonJitteredMatrix = camera->GetProj();
	XMMATRIX jitteredMat = data->nonJitteredMatrix;
	GetJitteredProjectionMatrix(
		camera->GetNearZ(),
		camera->GetFarZ(),
		camera->GetFovY(),
		camera->GetAspect(),
		jitterOffset,
		width, height, 
		jitteredMat,
		jitter
	);
}

struct PrepareRunnable
{
	PrepareComponent* ths;
	ThreadCommand* threadCommand;
	FrameResource* resource;
	Camera* camera;
	void operator()()
	{
		CameraTransformData* transData = (CameraTransformData*)camera->GetResource(ths, []()->CameraTransformData*
		{
			return new CameraTransformData;
		});
		
		camera->UploadCameraBuffer(ths->passConstants);
		ConstBufferElement ele = resource->cameraCBs[camera->GetInstanceID()];
		ele.buffer->CopyData(ele.element, &ths->passConstants);
	}
};

void PrepareComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		this,
		commandList,
		data.resource,
		data.camera
	};
	ScheduleJob(runnable);
}