#include "Editor.h"
#include "FreeCamera.h"
#include "Window.h"
#include "Mouse.h"
#include "Structs.h"
#include "imgui.h"
#include "SceneNode.h"

Editor::~Editor()
{
	for (auto  pGeo : m_Geometry)
	{
		delete pGeo.second.pGeometry;
	}
	for (SceneNode* pNode : m_pScenes)
	{
		delete pNode;
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

	EditableGeometry person{ {}, new QuadricGeometry{100, "Person"}, false };
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

	person.quadrics.push_back(head);
	person.quadrics.push_back(body);
	person.quadrics.push_back(upperArmRight);
	person.quadrics.push_back(lowerArmRight);
	person.quadrics.push_back(handRight);
	person.quadrics.push_back(upperArmLeft);
	person.quadrics.push_back(lowerArmLeft);
	person.quadrics.push_back(handLeft);
	person.quadrics.push_back(upperLegRight);
	person.quadrics.push_back(lowerLegRight);
	person.quadrics.push_back(shoeRight);
	person.quadrics.push_back(upperLegLeft);
	person.quadrics.push_back(lowerLegLeft);
	person.quadrics.push_back(shoeLeft);
	
	m_Geometry.emplace("Person", person);

	SceneNode* pScene = new SceneNode{};
	m_pScenes.push_back(pScene);
	m_pCurrentScene = pScene;

	UINT count = 10;
	for (UINT i = 0; i < count; i++)
	{
		for (UINT j = 0; j < count; j++)
		{
			QuadricInstance* pInstance = new QuadricInstance{ person.pGeometry };
			pInstance->GetTransform().SetPosition({ 5.0f * i ,4.5f ,5.0f * j });
			pScene->AddElement(pInstance);
		}
	}

	EditableGeometry ground{ {}, new QuadricGeometry{1, "World"}, false };

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

	ground.quadrics.push_back(world);

	m_Geometry.emplace(ground.pGeometry->GetName(), ground);
	QuadricInstance* pInstance = new QuadricInstance{ ground.pGeometry };
	pScene->AddElement(pInstance);

	ground.UpdateGeometry(m_DX12.GetDevice(), m_DX12.GetPipeline()->commandList.Get());
	person.UpdateGeometry(m_DX12.GetDevice(), m_DX12.GetPipeline()->commandList.Get());
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

	static UINT frameCtr = 0, fps = UINT(1 / dt);
	static float ctr = 0.0f;
	ctr += dt;
	frameCtr++;
	if (ctr > 1.0f)
	{
		fps = frameCtr;
		ctr--;
		frameCtr = 0;
	}

	//IMGUI
	m_ImGuiRenderer.NewFrame();

	static bool showTiles = false;
	static bool reverseDepth = true;
	static int tileDim[2] = { 128,128 };
	static int numRasterizers = 256, quadricsPerRasterizer = 64;
	ImGui::Begin("Renderer Settings");
	ImGui::Checkbox("Show Tiles", &showTiles);
	ImGui::Checkbox("Reverse DepthBuffer", &reverseDepth);
	ImGui::InputInt2("TileDimensions", tileDim);
	ImGui::InputInt("# Rasterizers", &numRasterizers);
	ImGui::InputInt("Quadrics / Rasterizer", &quadricsPerRasterizer);

	ImGui::Text(("FPS: " + std::to_string(fps) + "\t" + std::to_string(dt * 1000).substr(0 , 5) + " ms").c_str());
	ImGui::End();
	
	m_QRenderer.ShowTiles(showTiles);
	m_QRenderer.ReverseDepth(reverseDepth);

	if (tileDim[0] >= 32 && tileDim[0] <= 512 && tileDim[1] >= 32 && tileDim[1] <= 512 && numRasterizers <= 1000 && quadricsPerRasterizer <= 512)
	{
		ThrowIfFailed(m_DX12.GetPipeline()->commandAllocator->Reset());
		ThrowIfFailed(m_DX12.GetPipeline()->commandList->Reset(m_DX12.GetPipeline()->commandAllocator.Get(), nullptr));
		m_QRenderer.SetRasterizerSettings(m_DX12.GetPipeline()->commandList.Get(), numRasterizers, Dimensions<unsigned int>{(UINT)tileDim[0], (UINT)tileDim[1]}, quadricsPerRasterizer);
		ThrowIfFailed(m_DX12.GetPipeline()->commandList->Close());
		ID3D12CommandList* cmdsLists[] = { m_DX12.GetPipeline()->commandList.Get() };
		m_DX12.GetPipeline()->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		m_DX12.GetPipeline()->Flush();
	}

	//SCene graph

	ImGui::ShowDemoWindow();

	ImGui::Begin("SceneGraph");
	m_pCurrentScene->RenderImGui();
	ImGui::End();
	//

	static std::string selected = "";
	static bool show = false;
	ImGui::Begin("Geometry");
	for (auto pair : m_Geometry)
	{
		if (ImGui::Selectable(pair.first.c_str(), pair.first.c_str() == selected))
		{
			selected = pair.first.c_str();

		}
	}
	if (m_Geometry.find(selected) != m_Geometry.cend())
	{
		if (ImGui::Button("Show Info"))
		{
			show = true;
		}
	}
	ImGui::End();


	if (show)
	{
		EditableGeometry info = m_Geometry[selected];
		ImGui::Begin("Geometry Info", &show);
		char buf[64]{};
		strcpy_s(buf, selected.c_str());
		if (ImGui::InputText("Name", buf, 64))
		{
			m_Geometry.erase(selected);
			auto newName = std::string{ buf };
			m_Geometry.emplace(newName, info);
			selected = newName;
			info.pGeometry->SetName(newName);
		}
		ImGui::Text((std::to_string(info.quadrics.size()) + " Ellipsoids").c_str());
		if (ImGui::Button("Edit"))
		{

		}
		ImGui::End();
	}

}

void Editor::Render()
{
	//QUADRICS
	m_QRenderer.SetViewMatrix(m_pCamera->GetView());

	m_pCurrentScene->Render(&m_QRenderer);

	//RENDER FINAL IMAGE
	m_DX12.NewFrame();
	{
		m_QRenderer.RenderFrame(m_DX12.GetPipeline()->commandList.Get(), m_DX12.GetPipeline()->GetCurrentRenderTarget());
		m_ImGuiRenderer.RenderUI(m_DX12.GetPipeline()->commandList.Get());
	}
	m_DX12.Present();
}
