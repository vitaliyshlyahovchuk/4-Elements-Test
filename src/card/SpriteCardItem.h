#ifndef CARD_ITEM_RES_H
#define CARD_ITEM_RES_H

#include "MapItem.h"

namespace Card
{
	struct PartSheet {
	public:
		Render::Texture *texture;
		IRect screenRect;
		FRect textureUV;
		PartSheet();
		PartSheet(Render::Texture *texture, IRect screenRect, FRect textureUV);
		void Draw();
	};

	class SpriteCardItem
		: public MapItem
	{
	protected:
		PartSheet part;
	public:
		SpriteCardItem();
		virtual void init(Container* parent, rapidxml::xml_node<>* info);
		virtual void Draw(const FPoint &shift);
		virtual void Update(float dt);
	};

}//namespace Card

#endif //CARD_ITEM_RES_H