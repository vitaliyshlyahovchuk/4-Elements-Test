#include "stdafx.h"
#include "SpriteCardItem.h"
#include "CardResourceManager.h"

namespace Card
{


	/*******************/
	// PartSheet
	/*******************/
	PartSheet::PartSheet()
		: texture(NULL)
	{
	}

	PartSheet::PartSheet(Render::Texture *texture, IRect screenRect, FRect textureUV)
		: texture(texture)
		, screenRect(screenRect)
		, textureUV(textureUV)
	{
		float high_level = screenRect.y + screenRect.height;
		float low_level = screenRect.y;
		Card::cardResourceManager.Register(texture, low_level, high_level);
	}

	void PartSheet::Draw()
	{
		if(texture->IsLoaded())
		{
			texture->Draw(screenRect, textureUV);
		}
	}

	/*******************/
	// SpriteCardItem
	/*******************/

	SpriteCardItem::SpriteCardItem()
		: MapItem()	
	{

	}

	void SpriteCardItem::init(Container* parent, rapidxml::xml_node<>* xml_item)
	{
		MapItem::init(parent, xml_item);

		rapidxml::xml_node<>* xml_info = xml_item->first_node("info");	
		IPoint offset(xml_info);
		Render::Texture *texture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(xml_info, "id"));
		rapidxml::xml_node<>* xml_rect = xml_info->first_node("rect");
		IRect screenRect(offset + IPoint(xml_rect), Xml::GetIntAttribute(xml_rect, "w"), Xml::GetIntAttribute(xml_rect, "h"));
		part = PartSheet(texture, screenRect, FRect(0.f, 1.f, 0.f, 1.f));
	}

	void SpriteCardItem::Draw(const FPoint &shift)
	{
		if(part.texture->IsLoaded())
		{
			part.texture->Draw(part.screenRect.MovedBy(shift.Rounded()), part.textureUV);
		}
	}

	void SpriteCardItem::Update(float dt)
	{
	
	}

}//namespace Card
