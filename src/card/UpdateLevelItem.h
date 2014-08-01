#pragma once

#include "MapItem.h"

namespace Card 
{
	class CardAvatarFriends;
	// ����� ������/���������� � ������� ������(LevelMarker), � � ����� (Gateway)
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
		// ������������� �������� (������/������/�������)
		virtual void InitMarker();
		// ���������� ����� �������� ������������ ����� ������ ��������� �������� (�� position)
		virtual FPoint getCenterPosition() const { return FPoint(); }
		// ����� � ������� 
		int getMarker() const { return marker; }
		// ������ ������������ (����� ������������ ������� �� ������ isOpen � isComplete)
		virtual void startVisualization();
	protected:
		// � ������ ������/������� �������� ������� (������������� ���������� "current_marker" � gameInfo)
		int marker;
		// ������ �� ������ ������� (������� ����� ���� ����������, � ����� ���� � ���) "current_marker" >= marker
		bool isOpen;
		// ������� ������� "current_marker" > marker
		bool isComplete;

		// �������� ������������
		bool activeVisualization;
		// ����� ������������
		float visualizationTime;
		// ������� ������� ������������
		float visualizationTimer;
		// ���������� �������������
		virtual void endVisualization();
		// �������� �� ���� � ������
		virtual bool hitTest(const FPoint & position) const;
		// ���� � MouseDown
		FPoint pressed_mouse;
		// ����� �� ������
		bool pressed;
	};
} // namespace Card