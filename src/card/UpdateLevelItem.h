#pragma once

#include "MapItem.h"

namespace Card 
{
	class CardAvatarFriends;
	// общие данные/поведением у маркера уровня(LevelMarker), и у шлюза (Gateway)
	class UpdateLevelItem : public MapItem {
	public:
		virtual void init(Container* parent, rapidxml::xml_node<>* info);
		virtual void Update(float dt);
		virtual bool needDraw(const IRect &draw_rect) const;
		virtual bool needUpdate(const IRect &update_rect) const;
		virtual void MouseDown(const FPoint& mouse_position, bool& capture);
		virtual void MouseMove(const FPoint& mouse_position, bool& capture);
		virtual void MouseUp(const FPoint& mouse_position, bool& capture);
		virtual void OnClick() {};
		// инициализация элемента (открыт/закрыт/пройден)
		virtual void InitMarker();
		// визуальный центр элемента относительно точки начала координат элемента (от position)
		virtual FPoint getCenterPosition() const { return FPoint(); }
		// номер в истории 
		int getMarker() const { return marker; }
		// начало визуализации (какая визуализация зависит от флагов isOpen и isComplete)
		virtual void startVisualization();
	protected:
		// к какому уровню/воротам привязан элемент (соответствует переменной "current_marker" в gameInfo)
		int marker;
		// открыт ли данный элемент (элемент может быть пройденным, а может быть и нет) "current_marker" >= marker
		bool isOpen;
		// элемент пройден "current_marker" > marker
		bool isComplete;

		// запущена визуализация
		bool activeVisualization;
		// время визуализации
		float visualizationTime;
		// счетчик времени визуализации
		float visualizationTimer;
		// завершение визиуализации
		virtual void endVisualization();
		// попадает ли мышь в объект
		virtual bool hitTest(const FPoint & position) const;
		// мышь в MouseDown
		FPoint pressed_mouse;
		// нажат ли объект
		bool pressed;
	};
} // namespace Card