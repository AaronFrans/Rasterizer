//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

#define TRIANGLE_STRIP

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_AspectRatio = m_Width / static_cast<float>(m_Height);

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//clear background
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//reset buffer
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, FLT_MAX);

	//Rasterization


#ifdef TRIANGLE_STRIP

	std::vector<Mesh> meshes_world{
		Mesh
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
					{0.5f, 1}},
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
		}
	};
#else
	std::vector<Mesh> meshes_world{
	Mesh
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
				{0.5f, 1}},
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
	}
	};

#endif // TRIANGLE_STRIP


	std::vector<Vertex_Out> rasterVertices{};
	VertexTransformationFunction(meshes_world[0].vertices, rasterVertices);


	//RENDER LOGIC


	std::vector<uint32_t>& meshIndeces = meshes_world[0].indices;
#ifdef TRIANGLE_STRIP
	for (int i{}; i + 2 < meshIndeces.size(); ++i)
	{

		int index0{ i }, index1{}, index2{};
		// if n&1 is 1, then odd, else even
		if (!(i & 1)) //If true = even, else odd.
		{
			index1 = i + 1;
			index2 = i + 2;
		}
		else
		{
			index1 = i + 2;
			index2 = i + 1;

		}
#else
	for (int i{}; i < meshIndeces.size(); i += 3)
	{
		int index0{ i }, index1{ i + 1 }, index2{ i + 2 };
#endif // TRIANGLE_STRIP

		const Vector2 v0{
			rasterVertices[meshIndeces[index0]].position.x,
			rasterVertices[meshIndeces[index0]].position.y
		};
		const Vector2 v1{
			rasterVertices[meshIndeces[index1]].position.x,
			rasterVertices[meshIndeces[index1]].position.y
		};
		const Vector2 v2{
			rasterVertices[meshIndeces[index2]].position.x,
			rasterVertices[meshIndeces[index2]].position.y
		};

		const Vector2 edge0{ v1 - v0 };
		const Vector2 edge1{ v2 - v1 };
		const Vector2 edge2{ v0 - v2 };

		const float triangleArea{ Vector2::Cross(edge0, edge1) };


		const Vector2 minBB{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
		const Vector2 maxBB{ Vector2::Max(v0, Vector2::Max(v1, v2)) };


		if (0 >= minBB.x || 0 >= minBB.y ||
			maxBB.x >= (m_Width - 1) || maxBB.y >= (m_Height - 1)) continue;

		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
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



				float depth0{ rasterVertices[meshIndeces[index0]].position.z };
				float depth1{ rasterVertices[meshIndeces[index1]].position.z };
				float depth2{ rasterVertices[meshIndeces[index2]].position.z };


				float zInterpolatedWeight{ 1 / (
						weightV0 / depth0 +
						weightV1 / depth1 +
						weightV2 / depth2
					) };

				if (m_pDepthBufferPixels[pixelIndex] < zInterpolatedWeight) continue;

				m_pDepthBufferPixels[pixelIndex] = zInterpolatedWeight;


				Vector2 uvInterpolated0{ weightV0 * (rasterVertices[meshIndeces[index0]].uv / depth0) };
				Vector2 uvInterpolated1{ weightV1 * (rasterVertices[meshIndeces[index1]].uv / depth1) };
				Vector2 uvInterpolated2{ weightV2 * (rasterVertices[meshIndeces[index2]].uv / depth2) };

				Vector2 uvInterpolate{ (uvInterpolated0 + uvInterpolated1 + uvInterpolated2) * zInterpolatedWeight };

				ColorRGB finalColor{
					m_pTexture->Sample(uvInterpolate)
				};

				finalColor.MaxToOne();

				m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}


void Renderer::VertexTransformationFunction(const std::vector<Vertex>&vertices_in, std::vector<Vertex_Out>&vertices_out) const
{
	std::vector<Vertex_Out> ndcVertices{};
	ndcVertices.reserve(ndcVertices.size());
	Matrix projectionMatrix{
		Vector3{1 / (m_AspectRatio * m_Camera.fov), 0, 0},
		Vector3{0, 1 / m_Camera.fov, 0},
		Vector3{0,0,1},
		Vector3{0,0,0}
	};

	for (Vertex vertex : vertices_in)
	{
		Vector4 postition;
		vertex.position = m_Camera.invViewMatrix.TransformPoint(vertex.position);

		postition.x = vertex.position.x / (m_AspectRatio * m_Camera.fov) / vertex.position.z;
		postition.y = vertex.position.y / m_Camera.fov / vertex.position.z;
		postition.z = vertex.position.z;
		ndcVertices.emplace_back(Vertex_Out{ postition,vertex.color, vertex.uv, vertex.normal, vertex.normal });
	}


	vertices_out.reserve(ndcVertices.size());

	for (Vertex_Out vertex : ndcVertices)
	{

		vertex.position.x = (vertex.position.x + 1) * 0.5f * m_Width;
		vertex.position.y = (1.0f - vertex.position.y) * 0.5f * m_Height;

		vertices_out.emplace_back(vertex);
	}

}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
