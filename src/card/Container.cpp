#include "stdafx.h"
#include "Container.h"
#include "CardFactory.h"

using namespace Card;

void Container::init(Container* parent, rapidxml::xml_node<>* info) {
	MapItem::init(parent, info);
	rapidxml::xml_node<>* e = info->first_node("children");
	Assert(e);
	for (e = e->first_node(); e; e = e->next_sibling()) {
		children.push_back(Factory::create(this, e));
	}
	rect = children[0]->getRect();
	for (size_t i = 1, count = children.size(); i < count; ++i) {
		rect = unionRect(rect, children[i]->getRect());
	}
	rect.MoveBy(position.Rounded());
}

void Container::Draw(const FPoint& shift) {
	for (MapItemIterator i = children.begin(), e = children.end(); i != e; ++i) {
		(*i)->Draw(position + shift);
	}
	MapItem::Draw(shift);
}

void Container::Update(float dt) {
	for (MapItemIterator i = children.begin(), e = children.end(); i != e; ++i) {
		(*i)->Update(dt);
	}
}

void Container::MouseDown(const FPoint& mouse_position, bool& capture) {
	MapItem::MouseDown(mouse_position, capture);
	const FPoint p = mouse_position - position;
	for (MapItemReverseIterator i = children.rbegin(), e = children.rend(); i != e; ++i) {
		(*i)->MouseDown(p, capture);
	}
}

void Container::MouseMove(const FPoint& mouse_position, bool& capture) {
	MapItem::MouseMove(mouse_position, capture);
	const FPoint p = mouse_position - position;
	for (MapItemReverseIterator i = children.rbegin(), e = children.rend(); i != e; ++i) {
		(*i)->MouseMove(p, capture);
	}
}

void Container::MouseUp(const FPoint& mouse_position, bool& capture) {
	MapItem::MouseUp(mouse_position, capture);
	const FPoint p = mouse_position - position;
	for (MapItemReverseIterator i = children.rbegin(), e = children.rend(); i != e; ++i) {
		(*i)->MouseUp(p, capture);
	}
}

MapItem* Container::GetChildByName(const std::string &name) {
	for (MapItemIterator i = children.begin(), e = children.end(); i != e; ++i) {
		if ( (*i)->GetName() == name) {
			return *i;
		}
	}
	Assert(false);
	return NULL;
}

MapItem* Container::GetChildByIndex(int index) {
	Assert(index >= 0 && index < static_cast<int>(children.size()));
	return children[index];
}

int Container::numChildren() const {
	return static_cast<int>(children.size());
}

void Container::removeChildren() {
	for (MapItemIterator i = children.begin(), e = children.end(); i != e; ++i) {
		delete *i;
	}
	children.clear();
}

Container::~Container() {
	removeChildren();
}

void Container::AddChild(MapItem* child) {
	children.push_back(child);
}

void Container::AcceptMessage(const Message &message) {
	for (size_t i = 0, count = children.size(); i < count; ++i) {
		children[i]->AcceptMessage(message);
	}
}