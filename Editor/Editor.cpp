#include "Editor.h"
#include "FreeCamera.h"
#include "Window.h"
#include "Mouse.h"
#include "Structs.h"
#include "imgui.h"
#include "SceneNode.h"

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

#define USE_IMGUI 0

Editor::~Editor()
{
	m_DX12.GetPipeline()->Flush();
	for (auto  pGeo : m_Geometry)
	{
		delete pGeo.second.pGeometry;
	}
	for (SceneNode* pNode : m_Prefabs)
	{
		delete pNode;
	}
}

Editor::Editor(Window* pWindow, Mouse* pMouse)
	:m_pWindow{ pWindow }, m_pCamera{}, m_DX12{ pWindow }, m_ImGuiRenderer{ m_DX12.GetDevice(), pWindow->GetHandle() }
	,m_QRenderer{ m_DX12.GetDevice(), pWindow->GetDimensions().width, pWindow->GetDimensions().height}
	, m_pCurrentScene{ nullptr }, m_Prefabs{}, m_pGeometryEditor{ new SceneNode{"Editor"} }, m_pEditResult{}
{
	m_pCamera = new FreeCamera{pWindow, pMouse };
	
	m_pCamera->Offset({ 0.0f,4.5f, -5.f });

	m_pWindow->AddListener(&m_ImGuiRenderer);
}

void Editor::Initialize()
{
	m_DX12.GetPipeline()->commandAllocator[m_DX12.GetPipeline()->currentRT]->Reset();
	m_DX12.GetPipeline()->commandList->Reset(m_DX12.GetPipeline()->commandAllocator[m_DX12.GetPipeline()->currentRT].Get(), nullptr);
	m_QRenderer.Initialize(m_DX12.GetPipeline()->commandList.Get());
	m_QRenderer.SetProjectionVariables(m_pCamera->GetFOV(), m_pWindow->AspectRatio(), m_pCamera->GetNearPlane(), 200.0f);
	m_QRenderer.SetRendererSettings(m_DX12.GetPipeline()->commandList.Get(), 1024, Dimensions<unsigned int>{64,64}, 128);
	m_QRenderer.ShowTiles(true);
	m_QRenderer.ReverseDepth(true);

	//initialization
	DirectX::XMFLOAT3 skinColor{ 1.0f,0.67f,0.45f }, tShirtColor{ 1,0,0 }, pantsColor{ 0,0,1 }, shoeColor{ 0.6f,0.4f,0.1f };

	EditableGeometry person{ {}, new QuadricGeometry{"Person"}, 5000 };

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

	SceneNode* pScene = new SceneNode{"ROOT"};
	SceneNode* army = new SceneNode{};
	pScene->AddNode(army);
	m_Prefabs.push_back(pScene);
	m_pCurrentScene = pScene;

	UINT count = 40;
	for (UINT i = 0; i < count; i++)
	{
		for (UINT j = 0; j < count; j++)
		{
			QuadricInstance* pInstance = new QuadricInstance{ person.pGeometry };
			pInstance->GetTransform().SetPosition({ 5.0f * i ,4.5f ,5.0f * j });
			army->AddElement(pInstance);
		}
	}

	EditableGeometry ground{ {}, new QuadricGeometry{"World"}, 2 };

	EditQuadric world{};
	float range = 10'000;
	world.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	world.color = { 75 / 255.0f,168.0f / 255.0f,59 / 255.0f };
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

void Editor::Frame(float dt)
{
	m_DX12.NewFrame(); //resets cmdlist
	Update(dt); //fills cmdlists
	Render(); //fills cmdlist
	m_DX12.Present(); //executes cmdlist
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
#if USE_IMGUI
	m_ImGuiRenderer.NewFrame();

	static bool showTiles = false;
	static bool reverseDepth = true;
	static int tileDim[2] = { 128,128 };
	static int numRasterizers = 256, quadricsPerRasterizer = 64;
	ImGui::Begin("Renderer Settings");
	ImGui::Checkbox("Show Tiles", &showTiles);
	ImGui::Checkbox("Reverse DepthBuffer", &reverseDepth);

	bool changed = (ImGui::InputInt2("TileDimensions", tileDim));
	changed |= ImGui::InputInt("# Rasterizers", &numRasterizers);
	changed |= ImGui::InputInt("Quadrics / Rasterizer", &quadricsPerRasterizer);

	ImGui::Text(("FPS: " + std::to_string(fps) + "\t" + std::to_string(dt * 1000).substr(0 , 5) + " ms").c_str());
	ImGui::End();
	
	m_QRenderer.ShowTiles(showTiles);
	m_QRenderer.ReverseDepth(reverseDepth);

	if (tileDim[0] >= 32 && tileDim[0] <= 512 && tileDim[1] >= 32 && tileDim[1] <= 512 && numRasterizers <= 1000 && quadricsPerRasterizer <= 512)
	{
		
		m_QRenderer.SetRendererSettings(m_DX12.GetPipeline()->commandList.Get(), numRasterizers, Dimensions<unsigned int>{(UINT)tileDim[0], (UINT)tileDim[1]}, quadricsPerRasterizer);

	}

	//scene graph
	ImGui::Begin("Scene");
	ImGui::BeginChild("Graph", ImVec2(0, 300));
	SceneNode* pNode = nullptr;
	QuadricInstance* pInst = nullptr;
	static SceneNode* pSelectedNode = nullptr;
	static QuadricInstance* pSelectedInst = nullptr;
	m_pCurrentScene->RenderImGui(&pNode, &pInst);
	ImGui::EndChild();
	//EDITOR
	if (pInst)
	{
		pSelectedInst = pInst;
		pSelectedNode = nullptr;
	}
	else if (pNode)
	{
		pSelectedInst = nullptr;
		pSelectedNode = pNode;
	}
	

	if (pSelectedInst && pSelectedInst != m_pEditResult)
	{
		ImGui::BeginChild("Edit");
		pSelectedInst->RenderEditImGui();
		if (m_pCurrentScene != m_pGeometryEditor)
		{
			ImGui::Button("Edit Geometry");

			if (pSelectedInst->GetParent())
			{
				if (ImGui::Button("Remove"))
				{
					pSelectedInst->GetParent()->RemoveElement(pSelectedInst, true);
					pSelectedInst = nullptr;
				}
				else if (ImGui::Button("Duplicate"))
				{
					pSelectedInst->GetParent()->AddElement(new QuadricInstance{ (*pSelectedInst) });
				}
			}
		}
		ImGui::EndChild();
	}
	else if (pSelectedNode)
	{
		ImGui::BeginChild("Edit");
		pSelectedNode->RenderEditImGui();
		if (m_pCurrentScene != m_pGeometryEditor)
		{
			if (ImGui::Button("Add Child Node"))
			{
				pSelectedNode->AddNode(new SceneNode{ pSelectedNode->GetName() + "_CHILD" });
			}

			if (ImGui::Button("Save as Prefab"))
			{
				m_Prefabs.push_back(new SceneNode{ *pSelectedNode });
			}
			if (pSelectedNode->GetParent())
			{
				if (m_pCurrentScene != pSelectedNode)
				{
					if (ImGui::Button("Isolate"))
					{
						m_pCurrentScene = pSelectedNode;
						pSelectedNode = nullptr;
					}
					else if (ImGui::Button("Remove Node"))
					{
						pSelectedNode->GetParent()->RemoveNode(pSelectedNode, true);
						pSelectedNode = nullptr;
					}
					else if (ImGui::Button("Duplicate Node"))
					{
						pSelectedNode->GetParent()->AddNode(new SceneNode{ (*pSelectedNode) });
					}
				}
			}
		}
		ImGui::EndChild();

	}
	
	ImGui::End();

	ImGui::Begin("Data");

	//prefabs
	static UINT selectedIdx = UINT_MAX;
	ImGui::BeginChild("Prefabs", ImVec2(0, 200));
	ImGui::BeginChild("PrefabList", ImVec2{ 0, 20.0f * min(m_Geometry.size(), 10) });
	for (UINT i = 0 ; i <  m_Prefabs.size(); i++)
	{
		if (ImGui::Selectable((m_Prefabs[i]->GetName()).c_str(), i == selectedIdx))
		{
			selectedIdx = i;
		}
	}
	ImGui::EndChild();
	ImGui::NewLine();
	if (selectedIdx < m_Prefabs.size())
	{
		char buf[64]{};
		strcpy_s(buf, m_Prefabs[selectedIdx]->GetName().c_str());


		if (ImGui::InputText("Name", buf, 64))
		{
			m_Prefabs[selectedIdx]->SetName(buf);
		}
		ImGui::NewLine();

		if (m_pCurrentScene != m_Prefabs[selectedIdx])
		{
			if (ImGui::Button("Set as root"))
			{
				m_pCurrentScene = m_Prefabs[selectedIdx];
			}
			if (m_pCurrentScene != m_pGeometryEditor)
			{
				if (pSelectedNode)
				{
					if (ImGui::Button("Instantiate in current node"))
					{
						auto newNode = new SceneNode{ *m_Prefabs[selectedIdx] };
						pSelectedNode->AddNode(newNode);
						pSelectedNode = newNode;
					}
				}
			}
			if (ImGui::Button("Delete Prefab"))
			{
				delete m_Prefabs[selectedIdx];
				m_Prefabs[selectedIdx] = m_Prefabs.back();
				m_Prefabs.pop_back();
				selectedIdx = UINT_MAX;
			}
		}
	}
	ImGui::EndChild();

	//geometry
	static std::string selected = "";
	ImGui::BeginChild("Geometry");
	ImGui::BeginChild("GeometryList", ImVec2{ 0, 20.0f * min(m_Geometry.size(), 10)});
	for (auto pair : m_Geometry)
	{
		if (ImGui::Selectable(pair.first.c_str(), pair.first.c_str() == selected))
		{
			selected = pair.first.c_str();
		}
	}
	ImGui::EndChild();
	ImGui::NewLine();

	if (m_Geometry.find(selected) != m_Geometry.cend())
	{
		EditableGeometry info = m_Geometry[selected];
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
		ImGui::NewLine();

		ImGui::Text((std::to_string(info.quadrics.size()) + " Ellipsoids").c_str());
		int instances = info.pGeometry->GetMaxInstances();
		if (ImGui::InputInt("Max Instances", &instances))
		{
			instances = max(instances, (int)info.pGeometry->GetMaxInstances());
			if ((UINT)instances > info.pGeometry->GetMaxInstances())
			{
				info.maxInstances = (UINT)instances;
				info.UpdateGeometry(m_DX12.GetDevice(), m_DX12.GetPipeline()->commandList.Get());
			}
		}
		if (pSelectedNode)
		{
			if (ImGui::Button("Add Instance to current node"))
			{
				pSelectedInst = new QuadricInstance{ info.pGeometry };
				pSelectedNode->AddElement(pSelectedInst);
				pSelectedNode = nullptr;
			}
		}
		if (ImGui::Button("Edit Geometry"))
		{
			m_pCurrentScene = m_pGeometryEditor;
			m_pGeometryEditor->Clear();
			m_pEditResult = new QuadricInstance{ info.pGeometry };
			m_pGeometryEditor->AddElement(m_pEditResult);
		}
	}
	ImGui::EndChild();
	ImGui::End();
#endif
}

void Editor::Render()
{

	//QUADRICS
	m_QRenderer.SetViewMatrix(m_pCamera->GetView());
	m_pCurrentScene->Render(&m_QRenderer);

	m_QRenderer.RenderFrame(m_DX12.GetPipeline()->commandList.Get(), m_DX12.GetPipeline()->GetCurrentRenderTarget());
#if USE_IMGUI
	m_ImGuiRenderer.RenderUI(m_DX12.GetPipeline()->commandList.Get());
#endif
}
