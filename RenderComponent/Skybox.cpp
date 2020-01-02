#include "Skybox.h"
#include "Shader.h"
#include <mutex>
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "MeshRenderer.h"
#include "../Common/Camera.h"
#include "../Singleton/PSOContainer.h"
std::unique_ptr<Mesh> Skybox::fullScreenMesh = nullptr;
void Skybox::Draw(
	int targetPass,
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	ConstBufferElement* cameraBuffer,
	FrameResource* currentResource,
	PSOContainer* container
)
{
	PSODescriptor desc;
	desc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	desc.shaderPtr = skyboxMat->GetShader();
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	skyboxMat->BindShaderResource(commandList);
	skyboxMat->GetShader()->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraBuffer->buffer.operator->(), cameraBuffer->element);
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
}

Skybox::~Skybox()
{
	fullScreenMesh = nullptr;
	skyboxTex = nullptr;
	skyboxMat = nullptr;
	texDescHeap = nullptr;
}

Skybox::Skybox(
	ObjectPtr<Texture> tex,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList
) : MObject(),
skyboxTex(tex)
{
	texDescHeap = new DescriptorHeap();
	texDescHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
	tex->BindToDescriptorHeap(texDescHeap.operator->(), 0, device);
	ObjectPtr<UploadBuffer> noProperty = nullptr;
	skyboxMat = new Material(ShaderCompiler::GetShader("Skybox"), noProperty, 0, texDescHeap);
	skyboxMat->SetBindlessResource(ShaderID::PropertyToID("cubemap"), 0);
	if (fullScreenMesh == nullptr) {
		std::array<DirectX::XMFLOAT3, 3> vertex;
		vertex[0] = { -3, -1, 1 };
		vertex[1] = { 1, 3, 1 };
		vertex[2] = { 1, -1, 1 };
		std::array<INT16, 3> indices{ 0, 1, 2 };
		fullScreenMesh = std::make_unique<Mesh>(
			3,
			vertex.data(),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			device,
			commandList,
			DXGI_FORMAT_R16_UINT,
			3,
			indices.data()
			);
	}
}