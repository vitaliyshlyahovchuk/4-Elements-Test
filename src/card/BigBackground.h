#pragma once

#include "SpriteCardItem.h"
#include "../rapidxml/rapidxml.hpp"

namespace Card {

	// Задник карты
	class BigBackground {
	public:
		void init(rapidxml::xml_node<>* info);
		void Draw();
		int getWidth() { return width; };
		int getHeight() { return height; };
		void setDrawRect(const IRect& drawRect);
	private:
		void LoadSmallTexture(rapidxml::xml_node<>* rectXml, IPoint offset, Render::Texture *texture);
	public:
		static float OFFSET;
	private:

		std::vector<PartSheet> parts;
		std::vector<PartSheet*> drawParts;
		int width;
		int height;
		IRect _drawRect;
	};

} // namespace