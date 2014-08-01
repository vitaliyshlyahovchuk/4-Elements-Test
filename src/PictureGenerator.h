#pragma once

namespace Utils2D
{
	class PictureCellGenerator
	{
	public:
		static Render::Texture *collectionTexture;
		static std::map<int, FRect> sq_mask_to_frect;
		PictureCellGenerator();
		static void LoadSettings();
	};
}