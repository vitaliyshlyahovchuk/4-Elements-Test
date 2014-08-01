#include "stdafx.h"
#include "UpdateLevelItem.h"
#include "GameInfo.h"

using namespace Card;

void UpdateLevelItem::init(Container* parent, rapidxml::xml_node<>* info) {
	MapItem::init(parent, info);
	marker = Xml::GetIntAttribute(info, "marker");
	visualizationTimer = 0.f;
	visualizationTime = 1.f;
	activeVisualization = false;
	isOpen = false;
	isComplete = false;
	pressed = false;
}

void UpdateLevelItem::InitMarker() {
	int current_marker = gameInfo.getLocalInt("current_marker");
	activeVisualization = false;
	isOpen = current_marker >= marker;
	isComplete = current_marker > marker;
}

void UpdateLevelItem::startVisualization() {
	if (!isOpen) {
		isOpen = true;
		MM::manager.PlaySample("MapNextLevel");
	} else {
		isComplete = true;
	}
	activeVisualization = true;
	visualizationTimer = 0.f;
}

void UpdateLevelItem::endVisualization() {	
	activeVisualization = false;
	visualizationTimer = visualizationTime;
	Core::guiManager.getLayer("CardLayer")->getWidget("CardWidget")->AcceptMessage(Message("EndVisualization", isComplete ? "markerComplete" : "markerOpen" ));
};

void UpdateLevelItem::Update(float dt) {
	if (activeVisualization) {
		visualizationTimer += dt;
		if(visualizationTimer > visualizationTime) {
			endVisualization();
		}
	}
}

bool UpdateLevelItem::needDraw(const IRect &draw_rect) const {
	return rect.Inflated(150).Intersects(draw_rect);
}
bool UpdateLevelItem::needUpdate(const IRect &update_rect) const {
	return rect.Inflated(150).Intersects(update_rect);
}

void UpdateLevelItem::MouseDown(const FPoint& mouse_position, bool& capture) {
	if (!capture && hitTest(mouse_position)) {
		pressed_mouse = mouse_position;
		pressed = true;
		capture = true;
	} else {
		MapItem::MouseDown(mouse_position, capture);
	}
}

void UpdateLevelItem::MouseMove(const FPoint& mouse_position, bool& capture) {
	if (pressed) {
		float dx = pressed_mouse.x - mouse_position.x;
		float dy = pressed_mouse.y - mouse_position.y;
		if (hitTest(mouse_position) && (dx*dx + dy*dy < 100.f)) {
			capture = true;
		} else {
			pressed = false;
		}
	} else {
		MapItem::MouseMove(mouse_position, capture);
	}
}

void UpdateLevelItem::MouseUp(const FPoint& mouse_position, bool& capture) {
	if (pressed) {
		OnClick();
	} else {
		MapItem::MouseUp(mouse_position, capture);
	}
	pressed = false;
}

bool UpdateLevelItem::hitTest(const FPoint & position) const {
	return isOpen && rect.Contains(position.Rounded());
}