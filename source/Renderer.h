#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
		enum class RenderMode
		{
			Texture,
			Depth,
		};
		enum class ColorMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			FinalColor,
		};

	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();


		bool SaveBufferToImage() const;

		void ToggleRenderMode();
		void ToggleColorMode();
		void ToggleNormals();
		void ToggleRotation();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Texture* m_pDiffuseMap{ nullptr };
		Texture* m_pNormalMap{ nullptr };
		Texture* m_pSpecularMap{ nullptr };
		Texture* m_pGlossMap{ nullptr };

		Camera m_Camera{};

		Mesh m_Mesh{};

		int m_Width{};
		int m_Height{};

		bool m_UseNormalMap{ true };
		bool m_IsRotating{ true };

		RenderMode m_CurrentRenderMode;
		ColorMode m_CurrentColorMode;

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(); //W1 Version

		void RenderTraingle(int i0, int i1, int i2, std::vector<Vector2>& screenVertices);
		void InitMesh();
		bool PositionOutsideFrustrum(const Vector4& v);

		ColorRGB PixelShading(const Vertex_Out& v);

	};
}
