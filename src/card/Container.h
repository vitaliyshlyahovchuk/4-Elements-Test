#pragma once
#include "MapItem.h"

namespace Card {

// может в себе содержать детей - MapItem
// поддерживается только смещение (поворота и масштабирования нет)
class Container : public MapItem {
public:
	~Container();
	virtual void init(Container* parent, rapidxml::xml_node<>* info);
	virtual void Draw(const FPoint& shift);
	virtual void Update(float dt);
	virtual void MouseDown(const FPoint& mouse_position, bool& capture);
	virtual void MouseMove(const FPoint& mouse_position, bool& capture);
	virtual void MouseUp(const FPoint& mouse_position, bool& capture);

	MapItem* GetChildByName(const std::string &name);
	MapItem* GetChildByIndex(int index);

	virtual int numChildren() const;
	virtual void AddChild(MapItem* child);
	virtual void removeChildren();
	
	virtual void AcceptMessage(const Message &message);

	typedef std::vector<MapItem*> MapItems;
	typedef MapItems::iterator MapItemIterator;
	typedef MapItems::reverse_iterator MapItemReverseIterator;

protected:
	MapItems children;
};

} // namespace