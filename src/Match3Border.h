#ifndef _MATCH3_BORDER_H_
#define _MATCH3_BORDER_H_

#include "Place2D.h"

namespace Gadgets
{
	class LevelBorder
	{
		VertexBuffer _vb;
		Render::Texture *_texture;
		Place2D::BorderInfo *_map;
		std::vector<Place2D::ScopePieceForDraw> _levelScope; // внешная граница поля
		void AddBorder(const IPoint &pos, const BYTE &mask);
	public:
		void Init(const std::string &name_border_map);
		void LoadLevel();
		void Calculate();
		void Draw();
		void Clear();
	};
}

#endif //_MATCH3_BORDER_H_