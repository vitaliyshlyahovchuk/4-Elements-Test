#include "stdafx.h"
#include "ParalaxCloud.h"
#include "MyApplication.h"
#include "BigBackground.h"

namespace Card
{
	/**************************/
	// ParalaxCloud
	/**************************/

	float ParalaxCloud::SCROLL_POS = 0.f;

	ParalaxCloud::ParalaxCloud()
		: MapItem()
		, _right(false)
	{
	}

	void ParalaxCloud::init(Container* parent, rapidxml::xml_node<>* info)
	{
		_right = Xml::GetBoolAttributeOrDef(info, "subtype", false);
		_alpha = Xml::GetFloatAttributeOrDef(info, "alpha", 1.f);
		MapItem::init(parent, info);
		_cloudTexture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(info, "id"));
		_cloudTexture->setFilteringType(Render::Texture::BILINEAR);
		_cloudTexture->setAddressType(Render::Texture::REPEAT);
		_time = _timeOffset = math::random(0.f, 10.f);
		_timeScale = math::random(1.f, 1.3f);
	}

	void ParalaxCloud::Draw(const FPoint &pos)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(0.f-BigBackground::OFFSET, 0.f, 0.f));

		IRect rect_tex = _cloudTexture->getBitmapRect();
		_cloudTexture->Bind();
		float t = _time;
		float up_y = (MyApplication::GAME_HEIGHT + 0.f)/rect_tex.Height();
		FRect frect(0.f, 0.98f, 0.f - t, up_y - t);
		FRect rect(0.f, rect_tex.Width() + 0.f, -ParalaxCloud::SCROLL_POS, MyApplication::GAME_HEIGHT-ParalaxCloud::SCROLL_POS);
		if(_right)
		{
			rect.xStart = MyApplication::GAME_WIDTH - rect.Width();
			rect.xEnd = MyApplication::GAME_WIDTH + 0.f;
			std::swap(frect.xStart, frect.xEnd);
		}
		Render::BeginAlphaMul(_alpha);
		Render::DrawRect(rect, frect);
		Render::EndAlphaMul();

		Render::device.PopMatrix();
	}

	void ParalaxCloud::Update(float dt)
	{
		_time = _timeOffset +  ParalaxCloud::SCROLL_POS*_timeScale/500.f;
	}

	bool ParalaxCloud::needDraw(const IRect &draw_rect) const
	{
		return true;
	}
	bool ParalaxCloud::needUpdate(const IRect &draw_rect) const
	{
		return true;
	}

	/**************************/
	// ParalaxItemDynamic
	/**************************/
	ParalaxItemDynamic::ParalaxItemDynamic()
		: SpriteCardItem()
		, text(NULL)
		, shift_text_y(0.f)
	{
	}
	ParalaxItemDynamic::~ParalaxItemDynamic()
	{
	}

	void ParalaxItemDynamic::init(Container* parent, rapidxml::xml_node<>* xml_elem)
	{
		SpriteCardItem::init(parent, xml_elem);
		text = Core::resourceManager.Get<Render::Text>(Xml::GetStringAttribute(xml_elem, "text"));
		shift_text_y = Xml::GetFloatAttribute(xml_elem, "text_shift_y");
	}

	void ParalaxItemDynamic::Update(float dt)
	{
		position.y = (part.screenRect.y + part.screenRect.Height()/2 + (ParalaxCloud::SCROLL_POS - MyApplication::GAME_HEIGHT/2)) * 0.25;
	}

	void ParalaxItemDynamic::Draw(const FPoint &shift)
	{
		SpriteCardItem::Draw(shift + position);
		text->Draw(shift + position + FPoint(320.f, shift_text_y + part.screenRect.y));
	}

	bool ParalaxItemDynamic::needDraw(const IRect &draw_rect) const
	{
		return draw_rect.Intersects(part.screenRect);
	}
	bool ParalaxItemDynamic::needUpdate(const IRect &update_rect) const
	{
		return update_rect.Intersects(part.screenRect);
	}
	bool ParalaxItemDynamic::isDrawUp() const
	{
		return true;
	}

}//namespace Card
