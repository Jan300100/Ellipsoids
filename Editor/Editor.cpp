#include "Editor.h"
#include "FreeCamera.h"
#include "Window.h"
#include "Mouse.h"
#include "Structs.h"
#include "imgui.h"


Editor::~Editor()
{
	for (QuadricGeometry* pGeo : m_Geometry)
	{
		delete pGeo;
	}
}

Editor::Editor(Window* pWindow, Mouse* pMouse)
	:m_pWindow{ pWindow }, m_pCamera{}, m_DX12{ pWindow }, m_ImGuiRenderer{ m_DX12.GetDevice(), pWindow->GetHandle() }
	,m_QRenderer{ m_DX12.GetDevice(), pWindow->GetDimensions().width, pWindow->GetDimensions().height}
{
	m_pCamera = new FreeCamera{pWindow, pMouse };
	m_pWindow->AddListener(&m_ImGuiRenderer);
}

void Editor::Initialize()
{
	m_DX12.GetPipeline()->commandAllocator->Reset();
	m_DX12.GetPipeline()->commandList->Reset(m_DX12.GetPipeline()->commandAllocator.Get(), nullptr);
	m_QRenderer.Initialize(m_DX12.GetPipeline()->commandList.Get());
	m_QRenderer.SetProjectionVariables(m_pCamera->GetFOV(), m_pWindow->AspectRatio(), m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane());

	//initialization
	DirectX::XMFLOAT3 skinColor{ 1.0f,0.67f,0.45f }, tShirtColor{ 1,0,0 }, pantsColor{ 0,0,1 }, shoeColor{ 0.6f,0.4f,0.1f };

	EditQuadric head{};
	head.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	head.color = skinColor;
	head.transform.SetScale({ 1,1,1 });
	head.transform.SetPosition({ 0,2,0 });

	EditQuadric body{};
	body.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	body.color = tShirtColor;
	body.transform.SetScale({ 1,2,1 });
	body.transform.SetPosition({ 0,0,0 });

	EditQuadric upperArmRight{};
	upperArmRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperArmRight.color = tShirtColor;
	upperArmRight.transform.SetScale({ 0.4f,1.0f,0.4f });
	upperArmRight.transform.SetPosition({ 1.25f,0.6f,0 });
	upperArmRight.transform.SetRotation({ 0,0,1 });

	EditQuadric lowerArmRight{};
	lowerArmRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerArmRight.color = skinColor;
	lowerArmRight.transform.SetScale({ 0.3f,1.0f,0.3f });
	lowerArmRight.transform.SetPosition({ 1.85f,-0.6f,0 });

	EditQuadric handRight{};
	handRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	handRight.color = skinColor;
	handRight.transform.SetScale({ 0.3f,0.5f,0.3f });
	handRight.transform.SetPosition({ 1.85f,-1.5f,0 });

	EditQuadric upperArmLeft{};
	upperArmLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperArmLeft.color = tShirtColor;
	upperArmLeft.transform.SetScale({ 0.4f,1.0f,0.4f });
	upperArmLeft.transform.SetPosition({ -1.25f,0.6f,0 });
	upperArmLeft.transform.SetRotation({ 0,0,-1 });

	EditQuadric lowerArmLeft{};
	lowerArmLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerArmLeft.color = skinColor;
	lowerArmLeft.transform.SetScale({ 0.3f,1.0f,0.3f });
	lowerArmLeft.transform.SetPosition({ -1.85f,-0.6f,0 });

	EditQuadric handLeft{};
	handLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	handLeft.color = skinColor;
	handLeft.transform.SetScale({ 0.3f,0.5f,0.3f });
	handLeft.transform.SetPosition({ -1.85f,-1.5f,0 });

	//leg
	EditQuadric upperLegRight{};
	upperLegRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperLegRight.color = pantsColor;
	upperLegRight.transform.SetScale({ 0.5f,1.5f,0.5f });
	upperLegRight.transform.SetPosition({ 0.5f,-2.0f,0 });

	EditQuadric lowerLegRight{};
	lowerLegRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerLegRight.color = pantsColor;
	lowerLegRight.transform.SetScale({ 0.5f,1.5f,0.5f });
	lowerLegRight.transform.SetPosition({ 0.5f,-3.0f,0 });

	EditQuadric shoeRight{};
	shoeRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	shoeRight.color = shoeColor;
	shoeRight.transform.SetScale({ 0.5f,0.3f,0.75f });
	shoeRight.transform.SetPosition({ 0.5f,-4.5f,-0.5f });

	//leg
	EditQuadric upperLegLeft{};
	upperLegLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperLegLeft.color = pantsColor;
	upperLegLeft.transform.SetScale({ 0.5f,1.5f,0.5f });
	upperLegLeft.transform.SetPosition({ -0.5f,-2.0f,0 });

	EditQuadric lowerLegLeft{};
	lowerLegLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerLegLeft.color = pantsColor;
	lowerLegLeft.transform.SetScale({ 0.5f,1.5f,0.5f });
	lowerLegLeft.transform.SetPosition({ -0.5f,-3.0f,0 });

	EditQuadric shoeLeft{};
	shoeLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	shoeLeft.color = shoeColor;
	shoeLeft.transform.SetScale({ 0.5f,0.3f,0.75f });
	shoeLeft.transform.SetPosition({ -0.5f,-4.5f,-0.5f });

	std::vector<Quadric> in{};

	for (size_t i = 0; i < 1; i++)
	{
		in.push_back(head.ToQuadric());
		in.push_back(body.ToQuadric());
		in.push_back(upperArmRight.ToQuadric());
		in.push_back(lowerArmRight.ToQuadric());
		in.push_back(handRight.ToQuadric());
		in.push_back(upperArmLeft.ToQuadric());
		in.push_back(lowerArmLeft.ToQuadric());
		in.push_back(handLeft.ToQuadric());
		in.push_back(upperLegRight.ToQuadric());
		in.push_back(lowerLegRight.ToQuadric());
		in.push_back(shoeRight.ToQuadric());
		in.push_back(upperLegLeft.ToQuadric());
		in.push_back(lowerLegLeft.ToQuadric());
		in.push_back(shoeLeft.ToQuadric());
	}


	UINT count = 10;
	QuadricGeometry* dudeGeometry = new QuadricGeometry{ m_DX12.GetDevice(),m_DX12.GetPipeline()->commandList.Get() , in , count * count };
	for (UINT i = 0; i < count; i++)
	{
		for (UINT j = 0; j < count; j++)
		{
			m_Instances.push_back(dudeGeometry);
			Transform tr{};
			tr.SetPosition({ 5.0f * i ,4.5f ,5.0f * j });
			m_Instances.back().SetTransform(tr);
		}
	}
	m_Geometry.push_back(dudeGeometry);

	EditQuadric ellipsoid{};
	ellipsoid.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };


	EditQuadric world{};
	float range = 10000;
	world.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	world.color = { 75 / 255.0f,168 / 255.0f,59 / 255.0f };
	world.transform.SetScale({ range,range,range });
	world.transform.SetPosition({ 0,-range,0 });

	std::vector<Quadric> groundInput{};
	groundInput.push_back(world.ToQuadric());
	QuadricGeometry* ground = new QuadricGeometry{ m_DX12.GetDevice(),m_DX12.GetPipeline()->commandList.Get() , groundInput };
	m_Geometry.push_back(ground);
	m_Instances.push_back(ground);

	//
	// Done recording commands.
	m_DX12.GetPipeline()->commandList.Get()->Close();
	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_DX12.GetPipeline()->commandList.Get() };
	m_DX12.GetPipeline()->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	m_DX12.GetPipeline()->Flush();

}

void Editor::Update(float dt)
{
	//UPDATE
	m_pCamera->Update(dt);
}

void Editor::Render()
{

	//IMGUI
	m_ImGuiRenderer.NewFrame();
	static bool show = true;
	if (show)
	{
		ImGui::Begin("Hello world!", &show);
		ImGui::Text("Let's create something nice!");
		ImGui::End();
	}

	//QUADRICS
	m_QRenderer.SetViewMatrix(m_pCamera->GetView());
	for (QuadricInstance& inst : m_Instances)
	{
		m_QRenderer.Render(inst.GetGeometry(), inst.GetTransformMatrix());
	}

	//RENDER FINAL IMAGE
	m_DX12.NewFrame();
	{
		m_QRenderer.RenderFrame(m_DX12.GetPipeline()->commandList.Get(), m_DX12.GetPipeline()->GetCurrentRenderTarget());
		m_ImGuiRenderer.RenderUI(m_DX12.GetPipeline()->commandList.Get());
	}
	m_DX12.Present();
}
