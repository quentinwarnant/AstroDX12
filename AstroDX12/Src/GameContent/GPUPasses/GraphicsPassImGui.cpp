#include <GameContent/GPUPasses/GraphicsPassImGui.h>
#include <Rendering/Common/RendererContext.h>
#include <Rendering/Common/DescriptorHeap.h>
#include <DemoManager.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

void GraphicsPassImGui::Init(HWND hwnd, RendererContext& rendererContext, int numFramesInFlight, DemoManager* demoManager)
{
	auto srvHeap = rendererContext.GlobalCBVSRVUAVDescriptorHeap.lock();
	DX::astro_assert(srvHeap != nullptr, "SRV heap expired");

	// Reserve a descriptor slot for ImGui's font texture
	const int32_t imguiSrvIndex = srvHeap->GetCurrentDescriptorHeapHandle();
	srvHeap->IncreaseCurrentDescriptorHeapHandle();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);

	ImGui_ImplDX12_InitInfo initInfo;
	initInfo.Device = rendererContext.Device.Get();
	initInfo.CommandQueue = rendererContext.CommandQueue.Get();
	initInfo.NumFramesInFlight = numFramesInFlight;
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	initInfo.SrvDescriptorHeap = srvHeap->GetHeapPtr();
	initInfo.LegacySingleSrvCpuDescriptor = srvHeap->GetCPUDescriptorHandleByIndex(imguiSrvIndex);
	initInfo.LegacySingleSrvGpuDescriptor = srvHeap->GetGPUDescriptorHandleByIndex(imguiSrvIndex);
	ImGui_ImplDX12_Init(&initInfo);

	m_demoManager = demoManager;
}

void GraphicsPassImGui::Update(const GPUPassUpdateData& /*updateData*/)
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DrawDemoManagerUI();
}

void GraphicsPassImGui::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float /*deltaTime*/, const FrameResource& /*frameResources*/) const
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());
}

void GraphicsPassImGui::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GraphicsPassImGui::DrawDemoManagerUI()
{
	if (!m_demoManager)
		return;

	ImGui::Begin("Demos");

	for (auto& demo : m_demoManager->GetDemos())
	{
		bool enabled = demo.enabled;
		if (ImGui::Checkbox(demo.name.c_str(), &enabled))
		{
			m_demoManager->SetDemoEnabled(demo.name, enabled);
		}

		if (!demo.dependencies.empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(deps: %s)", [&]() {
				std::string deps;
				for (size_t i = 0; i < demo.dependencies.size(); ++i)
				{
					if (i > 0) deps += ", ";
					deps += demo.dependencies[i];
				}
				return deps;
			}().c_str());
		}
	}

	ImGui::End();
}
