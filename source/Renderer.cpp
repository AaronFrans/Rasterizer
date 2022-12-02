//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"
#include <algorithm>
#include <iostream>

using namespace dae;

//#define TRIANGLE_STRIP
#define OBJ

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f }, static_cast<float>(m_Width) / m_Height);

	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");

	m_CurrentRenderMode = RenderMode::Depth;


	InitMesh();

}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	//const float meshRotationPerSecond{ 50.0f };
	//m_Mesh.worldMatrix = Matrix::CreateRotationY(meshRotationPerSecond * pTimer->GetElapsed() * TO_RADIANS) * m_Mesh.worldMatrix;
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//clear background
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));

	//reset buffer
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, FLT_MAX);

	//Rasterization
	VertexTransformationFunction();

	std::vector<Vector2> screenVertices{};

	screenVertices.reserve(m_Mesh.vertices_out.size());
	for (auto& vertex : m_Mesh.vertices_out)
	{
		screenVertices.push_back({
			(vertex.position.x + 1) * 0.5f * m_Width,
			(1.0f - vertex.position.y) * 0.5f * m_Height
			});
	}

	//RENDER LOGIC


	switch (m_Mesh.primitiveTopology)
	{
	case PrimitiveTopology::TriangleList:

		for (int i{}; i < m_Mesh.indices.size(); i += 3)
		{
			int index0{ i }, index1{ i + 1 }, index2{ i + 2 };

			RenderTraingle(index0, index1, index2, screenVertices);

		}
		break;
	case PrimitiveTopology::TriangleStrip:

		for (int i{}; i < m_Mesh.indices.size() - 2; ++i)
		{

			int index0{ i }, index1{}, index2{};
			// if n&1 is 1, then odd, else even

			bool swapIndeces = i % 2;


			index1 = i + !swapIndeces * 1 + swapIndeces * 2;
			index2 = i + !swapIndeces * 2 + swapIndeces * 1;

			RenderTraingle(index0, index1, index2, screenVertices);
		}
		break;
	}



	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}


void Renderer::VertexTransformationFunction()
{
	m_Mesh.vertices_out.clear();
	m_Mesh.vertices_out.reserve(m_Mesh.vertices.size());

	Matrix worldViewProjectionMatrix{ m_Mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (const Vertex& vertex : m_Mesh.vertices)
	{
		Vertex_Out vertexOut{ {}, vertex.color, vertex.uv, vertex.normal, vertex.normal };
		vertexOut.position = worldViewProjectionMatrix.TransformPoint({ vertex.position, 1.0f });

		vertexOut.position.x /= vertexOut.position.w;
		vertexOut.position.y /= vertexOut.position.w;
		vertexOut.position.z /= vertexOut.position.w;

		m_Mesh.vertices_out.emplace_back(vertexOut);
	}

}

void Renderer::RenderTraingle(int i0, int i1, int i2, std::vector<Vector2>& screenVertices)
{

	if (PositionOutsideFrustrum(m_Mesh.vertices_out[m_Mesh.indices[i0]].position) ||
		PositionOutsideFrustrum(m_Mesh.vertices_out[m_Mesh.indices[i1]].position) ||
		PositionOutsideFrustrum(m_Mesh.vertices_out[m_Mesh.indices[i2]].position))
		return;

	const Vector2& v0{ screenVertices[m_Mesh.indices[i0]] };
	const Vector2& v1{ screenVertices[m_Mesh.indices[i1]] };
	const Vector2& v2{ screenVertices[m_Mesh.indices[i2]] };

	const Vector2 edge0{ v1 - v0 };
	const Vector2 edge1{ v2 - v1 };
	const Vector2 edge2{ v0 - v2 };

	const float triangleArea{ Vector2::Cross(edge0, edge1) };


	const Vector2 minBB{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
	const Vector2 maxBB{ Vector2::Max(v0, Vector2::Max(v1, v2)) };


	const int startX{ std::clamp(static_cast<int>(minBB.x) - 1, 0, m_Width) };
	const int startY{ std::clamp(static_cast<int>(minBB.y) - 1, 0, m_Height) };
	const int endX{ std::clamp(static_cast<int>(maxBB.x) + 1, 0, m_Width) };
	const int endY{ std::clamp(static_cast<int>(maxBB.y) + 1, 0, m_Height) };


	for (int px{ startX }; px < endX; ++px)
	{
		for (int py{ startY }; py < endY; ++py)
		{
			const int pixelIndex{ px + py * m_Width };
			Vector2 currentPixel{ static_cast<float>(px), static_cast<float>(py) };

			if (currentPixel.x < minBB.x || currentPixel.x > maxBB.x ||
				currentPixel.y < minBB.y || currentPixel.y > maxBB.y) continue;

			const Vector2 v0ToPixel{ currentPixel - v0 };
			const Vector2 v1ToPixel{ currentPixel - v1 };
			const Vector2 v2ToPixel{ currentPixel - v2 };

			const float edge0PixelCross{ Vector2::Cross(edge0, v0ToPixel) };
			const float edge1PixelCross{ Vector2::Cross(edge1, v1ToPixel) };
			const float edge2PixelCross{ Vector2::Cross(edge2, v2ToPixel) };


			if (!(edge0PixelCross > 0 && edge1PixelCross > 0 && edge2PixelCross > 0)) continue;


			const float weightV0{ edge1PixelCross / triangleArea };
			const float weightV1{ edge2PixelCross / triangleArea };
			const float weightV2{ edge0PixelCross / triangleArea };


			const float interpolatedZDepth
			{
				1.0f /
					(weightV0 / m_Mesh.vertices_out[m_Mesh.indices[i0]].position.z +
					weightV1 / m_Mesh.vertices_out[m_Mesh.indices[i1]].position.z +
					weightV2 / m_Mesh.vertices_out[m_Mesh.indices[i2]].position.z)
			};


			if (interpolatedZDepth < 0.0f || interpolatedZDepth > 1.0f ||
				m_pDepthBufferPixels[pixelIndex] < interpolatedZDepth)
				continue;

			m_pDepthBufferPixels[pixelIndex] = interpolatedZDepth;


			switch (m_CurrentRenderMode)
			{
			case dae::Renderer::RenderMode::Texture:
			{

				const float wWeight0{ m_Mesh.vertices_out[m_Mesh.indices[i0]].position.w };
				const float wWeight1{ m_Mesh.vertices_out[m_Mesh.indices[i1]].position.w };
				const float wWeight2{ m_Mesh.vertices_out[m_Mesh.indices[i2]].position.w };

				const float interpolatedWDepth
				{
					1.0f /
						(weightV0 / wWeight0 +
						weightV1 / wWeight1 +
						weightV2 / wWeight2)
				};

				Vector2 uvInterpolated0{ weightV0 * (m_Mesh.vertices_out[m_Mesh.indices[i0]].uv / wWeight0) };
				Vector2 uvInterpolated1{ weightV1 * (m_Mesh.vertices_out[m_Mesh.indices[i1]].uv / wWeight1) };
				Vector2 uvInterpolated2{ weightV2 * (m_Mesh.vertices_out[m_Mesh.indices[i2]].uv / wWeight2) };


				Vector2 uvInterpolated{ (uvInterpolated0 + uvInterpolated1 + uvInterpolated2) * interpolatedWDepth };

				ColorRGB finalColor{
					m_pTexture->Sample(uvInterpolated)
				};

				finalColor.MaxToOne();

				m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
			break;
			case dae::Renderer::RenderMode::Depth:
			{
				float depthVal = Remap(interpolatedZDepth, 0.985f, 1.0f);

				ColorRGB finalColor{ depthVal, depthVal, depthVal };

				m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
			break;
			}




		}
	}
}

void dae::Renderer::InitMesh()
{

#ifdef TRIANGLE_STRIP
	m_Mesh = Mesh
	{
		{
			Vertex{
				{-3.f,3.f,-2.f},
				{1},
				{0, 0}},
			Vertex{
				{0.f,3.f,-2.f},
				{1},
				{0.5f, 0}},
			Vertex{
				{3.f,3.f,-2.f},
				{1},
				{1, 0}},
			Vertex{
				{-3.f,0.f,-2.f},
				{1},
				{0, 0.5f}},
			Vertex{
				{0.f,0.f,-2.f},
				{1},
				{0.5f, 0.5f}},
			Vertex{
				{3.f,0.f,-2.f},
				{1},
				{1, 0.5f}},
			Vertex{
				{-3.f,-3.f,-2.f},
				{1},
				{0, 1}},
			Vertex{
				{0.f,-3.f,-2.f},
				{1},
				{0.5f, 1}
},
Vertex{
	{3.f,-3.f,-2.f},
	{1},
	{1,1}}
},
{
	3,0,4,1,5,2,
	2,6,
	6,3,7,4,8,5
},
PrimitiveTopology::TriangleStrip
	};
#elif defined(OBJ)
	Utils::ParseOBJ("Resources/tuktuk.obj", m_Mesh.vertices, m_Mesh.indices);
#else
	m_Mesh = Mesh
	{
		{
			Vertex{
				{-3.f,3.f,-2.f},
				{1},
				{0, 0}
			},
			Vertex{
				{0.f,3.f,-2.f},
				{1},
				{0.5f, 0}},
			Vertex{
				{3.f,3.f,-2.f},
				{1},
				{1, 0}},
			Vertex{
				{-3.f,0.f,-2.f},
				{1},
				{0, 0.5f}},
			Vertex{
				{0.f,0.f,-2.f},
				{1},
				{0.5f, 0.5f}},
			Vertex{
				{3.f,0.f,-2.f},
				{1},
				{1, 0.5f}},
			Vertex{
				{-3.f,-3.f,-2.f},
				{1},
				{0, 1}},
			Vertex{
				{0.f,-3.f,-2.f},
				{1},
				{0.5f, 1}
},
Vertex{
	{3.f,-3.f,-2.f},
	{1},
	{1,1}}
},
{
	3,0,1,	1,4,3,	4,1,2,
	2,5,4,	6,3,4,	4,7,6,
	7,4,5,	5,8,7
},
PrimitiveTopology::TriangleList
	};

#endif // TRIANGLE_STRIP

	const Vector3 position{ m_Camera.origin + Vector3{ 0.0f, -3.0f, 15.0f } };
	const Vector3 rotation{ };
	const Vector3 scale{ Vector3{ 0.5f, 0.5f, 0.5f } };
	m_Mesh.worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);
	}

bool dae::Renderer::PositionOutsideFrustrum(const Vector4& v)
{
	return v.x < -1.0f || v.x > 1.0f || v.y < -1.0f || v.y > 1.0f;;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::ToggleRenderState()
{
	m_CurrentRenderMode = static_cast<RenderMode>((static_cast<int>(m_CurrentRenderMode) + 1) % (static_cast<int>(RenderMode::Depth) + 1));
}
