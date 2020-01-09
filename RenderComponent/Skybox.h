#pragma once
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
class PSOContainer;
class FrameResource;
class ConstBufferElement;
class Skybox : public MObject
{
private:
	static std::unique_ptr<Mesh> fullScreenMesh;
	ObjectPtr<Texture> skyboxTex;
	ObjectPtr<Material> skyboxMat;
	ObjectPtr<DescriptorHeap> texDescHeap;
	UINT SkyboxCBufferID;
public:
	virtual ~Skybox();
	Skybox(
		ObjectPtr<Texture>& tex,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList
	);

	void Draw(
		int targetPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		ConstBufferElement* cameraBuffer,
		FrameResource* currentResource,
		PSOContainer* container
	);
};
