#include "stdafx.h"
#include "BigBackground.h"
#include "GameInfo.h"
#include "CardResourceManager.h"

using namespace Card;

float BigBackground::OFFSET = 0;

void BigBackground::LoadSmallTexture(rapidxml::xml_node<>* rectXml, IPoint offset, Render::Texture *texture) 
{
	IRect screenRect(IPoint(rectXml), Xml::GetIntAttribute(rectXml, "w"), Xml::GetIntAttribute(rectXml, "h"));
	IRect bitmapRect = texture->getBitmapRect();
	//screenRect.y = height - (screenRect.y + screenRect.height);
	screenRect.MoveBy(offset);
	float cx = static_cast<float>(Xml::GetIntAttribute(rectXml, "cx"));
	float cy = static_cast<float>(Xml::GetIntAttribute(rectXml, "cy"));
	FRect uv(cx / bitmapRect.width, (cx + screenRect.width) / bitmapRect.width,
		1.f - (cy + screenRect.height) / bitmapRect.height, 1.f - cy / bitmapRect.height
		);
	parts.push_back(PartSheet(texture, screenRect, uv));
}


void BigBackground::init(rapidxml::xml_node<>* info) {
	parts.clear();
	drawParts.clear();
	rapidxml::xml_node<>* xml_backgrounds = info->first_node("background");
	rapidxml::xml_node<>* xml_big = xml_backgrounds->first_node("bigtexture");
	height = 0;
	bool ipad_device = BigBackground::OFFSET <= 0.f;
	while(xml_big)
	{
		std::string path = Xml::GetStringAttribute(xml_big, "path");
		bool ipad_only = Xml::GetBoolAttributeOrDef(xml_big, "ipad", false);
		if(ipad_only)
		{
			//Если экран узкий то не загружаем
			if(ipad_device)
			{
				xml_big = xml_big->next_sibling("bigtexture");
				continue;
			}
		}
		if(Core::fileSystem.FileExists(path))
		{
			Xml::RapidXmlDocument description(path);
			rapidxml::xml_node<>* root_xml = description.first_node();
			//Проверка на наличие ресурса в принципе
			IPoint offset = xml_big + IPoint(-64, 0);
			width = Xml::GetIntAttribute(root_xml, "width");
			int h2 = Xml::GetIntAttribute(root_xml, "height");
			height = math::max(height, h2 + offset.y);
			rapidxml::xml_node<>* textureXml = root_xml->first_node("texture");
			while (textureXml) {
				Render::Texture* texture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(textureXml, "id"));
				//IRect bitmapRect = texture->getBitmapRect();
				rapidxml::xml_node<>* rectXml = textureXml->first_node("rect");
				while (rectXml) {
					LoadSmallTexture(rectXml, offset, texture);
					rectXml = rectXml->next_sibling("rect");
				}
				textureXml = textureXml->next_sibling("texture");
			}
		}
		xml_big = xml_big->next_sibling("bigtexture");
	}

	//Загрузка обычных текстур
	rapidxml::xml_node<>* xml_texture = xml_backgrounds->first_node("smaltexture");
	while(xml_texture)
	{
		IPoint offset(xml_texture);
		Render::Texture *texture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(xml_texture, "id"));
		rapidxml::xml_node<>* xml_rect = xml_texture->first_node("rect");
		IRect screenRect(offset + IPoint(xml_rect), Xml::GetIntAttribute(xml_rect, "w"), Xml::GetIntAttribute(xml_rect, "h"));
		parts.push_back(PartSheet(texture, screenRect, FRect(0.f, 1.f, 0.f, 1.f)));
		xml_texture = xml_texture->next_sibling("smaltexture");
		height = math::max(height, screenRect.LeftTop().y);
	}
	height += 180;
	_drawRect = IRect(0,0, 100,100);
}

void BigBackground::Draw() {
	//Render::device.PushMatrix();
	//Render::device.MatrixTranslate(math::Vector3(0.f-BigBackground::OFFSET, 0.f, 0.f));
	//bool draw_lost = false;
	for (size_t i = 0; i < drawParts.size(); ++i) {
		PartSheet* p = drawParts[i];		
		p->Draw();
	}
	//Render::device.PopMatrix();
	//if(draw_lost)
	//{
	//	Core::guiManager.getLayer("CardLayer")->getWidget("CardWidget")->AcceptMessage(Message("PausedScrolling"));
	//}
}

void BigBackground::setDrawRect(const IRect& drawRect) 
{
	_drawRect = drawRect;
	drawParts.clear();
	for (size_t i = 0; i < parts.size(); ++i) {
		if(drawRect.Intersects(parts[i].screenRect))
		{
			drawParts.push_back(&parts[i]);
		}
	}
}