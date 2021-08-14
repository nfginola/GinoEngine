#include "pch.h"
#include "Engine.h"
#include "Graphics/ResourceTypes.h"
#include "Graphics/DXDevice.h"
#include "Graphics/CentralRenderer.h"
#include "AssimpLoader.h"
#include "Input.h"
#include "FPCamera.h"

// Temp
#include "Timer.h"

namespace Gino
{
	Engine::Engine(Settings& settings)
	{
		m_input = std::make_unique<Input>(settings.hwnd);
		m_dxDev = std::make_unique<DXDevice>(settings.hwnd, settings.resolutionWidth, settings.resolutionHeight);
		m_centralRenderer = std::make_unique<CentralRenderer>(m_dxDev.get(), settings.vsync);
		
		m_fpCam = std::make_unique<FPCamera>((float)settings.resolutionWidth / settings.resolutionHeight, 80.f);

		m_centralRenderer->SetRenderCamera(m_fpCam.get());

		// DXState state
		/*
			this should PRIMARILY be used inside CentralRenderer and other Render modules
			state->Set(...)
			state->FinalizeBinds();		--> All API calls in one go
		*/

		// We should do something like: LoadModel("sponza", filePath)
		// and keep an unordered map with models with string identifiers
		// then when we have an Entity --> Entity.AddModel("sponza");
		m_sponzaModel = LoadModel("../assets/Models/Sponza_new/sponza.obj");
	}

	Engine::~Engine()
	{

	}

	static float dt = 0.f;
	void Engine::SimulateAndRender()
	{
		Timer timer;
		m_input->Update();

		// Move camera
		if (m_input->KeyIsDown(Keys::W))				m_fpCam->Move(MoveDirection::Forward);
		if (m_input->KeyIsDown(Keys::A))				m_fpCam->Move(MoveDirection::Left);
		if (m_input->KeyIsDown(Keys::S))				m_fpCam->Move(MoveDirection::Backward);
		if (m_input->KeyIsDown(Keys::D))				m_fpCam->Move(MoveDirection::Right);
		if (m_input->KeyIsDown(Keys::Space))			m_fpCam->Move(MoveDirection::Up);
		if (m_input->KeyIsDown(Keys::LeftShift))		m_fpCam->Move(MoveDirection::Down);

		if (m_input->KeyIsPressed(Keys::LeftControl))	m_fpCam->SetMoveSpeed(MoveSpeed::Slow);
		if (m_input->KeyIsReleased(Keys::LeftControl))	m_fpCam->SetMoveSpeed(MoveSpeed::Normal);

		// Hook camera rotate to raw input messages when RMB is pressed
		if (m_input->RMBIsPressed())
		{
			//m_input->SetMouseRawDeltaFunc([this](int dx, int dy) { this->m_fpCam->RotateCamera({ dx, dy }, 0.14f); });	// Sensitivity last arg
			m_input->SetMouseMode(MouseMode::Relative);
		}
		// Unhook camera from raw input messages when RMB is released
		if (m_input->RMBIsReleased())
		{
			//m_input->SetMouseRawDeltaFunc({});
			m_input->SetMouseMode(MouseMode::Absolute);
		}

		if (m_input->RMBIsDown())
		{
			m_fpCam->RotateCamera(m_input->GetMouseDelta(), 0.14f);	// No need for delta since we consume the WMs generated for this frame
																	// We instead supply some multiplier per WM 

			std::cout << "dx: " << m_input->GetMouseDelta().first << "|| dy: " << m_input->GetMouseDelta().second << std::endl;
			std::cout << "xPos: " << m_input->GetScreenPosition().first << "|| yPos: " << m_input->GetScreenPosition().second << std::endl;
		}


		if (m_input->MMBIsDown())
		{
			std::cout << "dx: " << m_input->GetMouseDelta().first << "|| dy: " << m_input->GetMouseDelta().second << std::endl;
			std::cout << "xPos: " << m_input->GetScreenPosition().first << "|| yPos: " << m_input->GetScreenPosition().second << std::endl;
		}

		// Finalize camera changes for this frame
		m_fpCam->Update(dt);







		/*
		
		UpdateObjects(dt)

		culler->cull(scene)
		
		for each non-culled geometry in scene:
			cr->SubmitOpaqueModel(mesh, material);
			cr->SubmitTransparentModel(mesh, material);
		
		*/

		m_centralRenderer->Render(m_sponzaModel.get());

		m_input->Reset();

		dt = timer.TimeElapsed();
		//std::cout << dt << std::endl;
	}

	Input* Engine::GetInput()
	{
		return m_input.get();
	}

	std::function<void(HWND, UINT, WPARAM, LPARAM)> Engine::GetImGuiHook() const
	{
		if (m_centralRenderer && m_centralRenderer->GetImGui())
		{
			return m_centralRenderer->GetImGui()->GetWin32Hook();
		}
		else
		{
			return[](HWND, UINT, WPARAM, LPARAM) {};
		}
	}


	Texture* Engine::LoadTexture(const std::string& filePath)
	{
		if (m_loadedTextures.find(filePath) == m_loadedTextures.end())
		{
			auto text = std::make_unique<Texture>();
			text->InitializeFromFile(m_dxDev->GetDevice(), m_dxDev->GetContext(), filePath);
			auto pair = m_loadedTextures.insert({ filePath, std::move(text) });
			
			return pair.first->second.get();
		}
		else
		{
			std::cout << "Skipping loading texture..: " << filePath << std::endl;
			return nullptr;
		}
	}

	std::unique_ptr<Model> Engine::LoadModel(const std::filesystem::path& filePath)
	{
		static std::string defaultDiffuseFilePath = "../assets/Textures/Default/defaultdiffuse.jpg";
		static std::string defaultSpecularFilePath = "../assets/Textures/Default/defaultspecular.jpg";
		static std::string defaultOpacityFilePath = "../assets/Textures/Default/defaultopacity.jpg";
		static std::string defaultNormalFilePath = "../assets/Textures/Default/defaultnormal.jpg";

		AssimpLoader loader(filePath);

		auto verts = loader.GetVertices();
		auto indices = loader.GetIndices();
		auto subsets = loader.GetSubsets();
		auto mats = loader.GetMaterials();

		//const std::string directory = filePath.parent_path().string() + "/";

		// Load textures
		for (auto& mat : mats)
		{
			Texture* diffuse = nullptr;
			Texture* specular = nullptr;
			Texture* opacity = nullptr;
			Texture* normal = nullptr;

			if (mat.diffuseFilePath.has_value())
			{
				diffuse = LoadTexture(mat.diffuseFilePath.value());
			}
			else
			{
				diffuse = LoadTexture(defaultDiffuseFilePath);
			}

			if (mat.normalFilePath.has_value())
			{
				normal = LoadTexture(mat.normalFilePath.value());
			}
			else
			{
				normal = LoadTexture(defaultNormalFilePath);
			}

			if (mat.opacityFilePath.has_value())
			{
				opacity = LoadTexture(mat.opacityFilePath.value());
			}
			else
			{
				opacity = LoadTexture(defaultOpacityFilePath);
			}

			if (mat.specularFilePath.has_value())
			{
				specular = LoadTexture(mat.specularFilePath.value());
			}
			else
			{
				specular = LoadTexture(defaultSpecularFilePath);
			}
			
		}


		// Transform verts data into our specified input layout
		std::vector<Vertex_POS_UV_NORMAL> vertsIn;
		vertsIn.reserve(verts.size());
		for (const auto& vert : verts)
		{
			Vertex_POS_UV_NORMAL vertex;
			vertex.pos.x = vert.position.x;
			vertex.pos.y = vert.position.y;
			vertex.pos.z = vert.position.z;

			vertex.uv.x = vert.uv.x;
			vertex.uv.y = vert.uv.y;

			vertex.normal.x = vert.normal.x;
			vertex.normal.y = vert.normal.y;
			vertex.normal.z = vert.normal.z;
			vertsIn.push_back(vertex);
		}

		Buffer vb;
		Buffer ib;
		vb.Initialize(m_dxDev->GetDevice(), VertexBufferDesc<Vertex_POS_UV_NORMAL>{ .data = vertsIn });
		ib.Initialize(m_dxDev->GetDevice(), IndexBufferDesc{ .data = indices });

		// Setup meshes
		std::vector<std::pair<Mesh, Material>> materialsAndMeshes;
		materialsAndMeshes.reserve(subsets.size());
		for (const auto& subset : subsets)
		{
			Mesh mesh
			{
				.numIndices = subset.indexCount,
				.indicesFirstIndex = subset.indexStart,
				.vertexOffset = subset.vertexStart
			};

			std::string diffLook = subset.diffuseFilePath.has_value() ? subset.diffuseFilePath.value() : defaultDiffuseFilePath;
			std::string specLook = subset.specularFilePath.has_value() ? subset.specularFilePath.value() : defaultSpecularFilePath;
			std::string opacityLook = subset.opacityFilePath.has_value() ? subset.opacityFilePath.value() : defaultOpacityFilePath;
			std::string normalLook = subset.normalFilePath.has_value() ? subset.normalFilePath.value() : defaultNormalFilePath;

			// Create material for this submesh to use
			Material mat;
			mat.Initialize(PhongMaterialData
				{
					.diffuse = m_loadedTextures.find(diffLook)->second.get(),
					.specular = m_loadedTextures.find(specLook)->second.get(),
					.opacity = m_loadedTextures.find(opacityLook)->second.get(),
					.normal = m_loadedTextures.find(normalLook)->second.get()
				});
			
			materialsAndMeshes.push_back({ mesh, mat });
		}

		auto model = std::make_unique<Model>();
		model->Initialize(vb, ib, materialsAndMeshes);
		return model;
	}

}


