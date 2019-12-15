#pragma once
#include "Shader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include <mutex>
#include "../Common/Camera.h"
class Skybox : public MObject
{
private:
	static std::unique_ptr<Mesh> fullScreenMesh;
	ObjectPtr<Texture> skyboxTex;
	ObjectPtr<Material> skyboxMat;
	ObjectPtr<DescriptorHeap> texDescHeap;
public:
	virtual ~Skybox();
	Skybox(
		ObjectPtr<Texture> tex,
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
