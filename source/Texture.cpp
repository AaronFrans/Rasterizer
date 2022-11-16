#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <cassert>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* loadSurface = IMG_Load(path.c_str());

		//if loadloadSurface == null throw assert
		assert(loadSurface && "Image failed to load.");

		Texture* toReturn{ new Texture{ loadSurface } };
		return toReturn;
		return nullptr;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		Uint32 x{ Uint32(uv.x * m_pSurface->w) }, y{ Uint32(uv.y * m_pSurface->h) };
		uint8_t r, g, b;

		SDL_GetRGB(m_pSurfacePixels[static_cast<uint32_t>(x + (y * m_pSurface->w))],
			m_pSurface->format,
			&r,
			&g,
			&b);

		ColorRGB pixelColor{ r / 255.f, g / 255.f, b / 255.f };
		return pixelColor;

		return {};
	}
}