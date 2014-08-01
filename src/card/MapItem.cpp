#include "stdafx.h"
#include "MapItem.h"


namespace Card {

	MapItem::MapItem()
		: position(0,0) 
#ifdef CARD_EDITOR
		, change_position(false)
		, info(NULL)
		, edited(false)
#endif
	{
	};

	void MapItem::init(Container* parent, rapidxml::xml_node<>* info) {
		this->parent = parent;
		yBound = Xml::GetFloatAttributeOrDef(info, "yBound", 0.f);
		position = FPoint(info);
		name = Xml::GetStringAttributeOrDef(info, "id", "");
		type = Xml::GetStringAttribute(info, "type");
		rapidxml::xml_node<>* xml_rect = info->first_node("rect");
		if (xml_rect) {
			rect = IRect(xml_rect);
			rect.MoveBy(position.Rounded());
		}

#ifdef CARD_EDITOR
		// для визуального редактирования карты 
		this->info = info;
		change_position = false;
		edited = false;
#endif
	}

	bool MapItem::CompareByBound(MapItem *item1, MapItem *item2) {
		return item1->position.y + item1->yBound > item2->position.y + item2->yBound;
	}

	bool MapItem::needDraw(const IRect &draw_rect) const {
		return rect.Intersects(draw_rect);
	}
	bool MapItem::needUpdate(const IRect &update_rect) const {
		return rect.Intersects(update_rect);
	}

	void MapItem::setPosition(const FPoint new_pos) {
		position = new_pos;
		rect.MoveTo(new_pos.Rounded());
	}

	bool MapItem::isDrawUp() const {
		return false;
	}

	void MapItem::AcceptMessage(const Message& message) {
	}

#ifndef CARD_EDITOR

	void MapItem::Draw(const FPoint& shift) {};
	void MapItem::MouseDown(const FPoint& mouse_position, bool &capture) {}
	void MapItem::MouseMove(const FPoint& mouse_position, bool &capture) {}
	void MapItem::MouseUp(const FPoint& mouse_position, bool &capture) {}


#else

	void MapItem::MouseDown(const FPoint& mouse_position, bool& capture) {
		if (!capture) {
			if (rect.Contains(mouse_position.Rounded())) {
				const float double_click_time = 0.2f;
				if (edited) {
					if (Core::mainInput.IsShiftKeyDown()) {
						edited = false;
						capture = true;
						return;
					}
					change_position = true;
					prev_mouse_pos = mouse_position;
					capture = true;
				} else {
					if (Core::mainInput.IsShiftKeyDown()) {
						capture = true;
						edited = true;
					}
				}
			}
		}
	}

	void MapItem::MouseMove(const FPoint& mouse_position, bool &capture) {
		if (change_position && prev_mouse_pos != mouse_position) {
			FPoint delta = mouse_position - prev_mouse_pos;
			setPosition(position + delta);
			prev_mouse_pos = mouse_position;
		}
	}

	void MapItem::MouseUp(const FPoint& mouse_position, bool &capture) {
		if (change_position && info) {
			change_position = false;
			Xml::SetIntAttribute(info, "x", static_cast<int>(position.x));
			Xml::SetIntAttribute(info, "y", static_cast<int>(position.y));
		}
	}

	void MapItem::Draw(const FPoint& shift) {
		if (edited) {
			Render::device.SetTexturing(false);
			Render::BeginColor(Color::BLUE);
			Render::DrawFrame(rect);
			Render::EndColor();
			Render::device.SetTexturing(true);
			Render::FreeType::BindFont("debug");
			Render::PrintString(static_cast<float>(rect.x), static_cast<float>(rect.y + rect.height + 20), name); 
		}
	};
#endif
}