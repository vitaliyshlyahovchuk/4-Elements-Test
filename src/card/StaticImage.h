#pragma once

#include "MapItem.h"

namespace Card {

// статичная текстура
class StaticImage: public MapItem {
public:
	virtual void init(Container* parent, rapidxml::xml_node<>* info);
	virtual void Draw(const FPoint& shift);
	virtual void Update(float dt) {};
protected:
	Render::Texture* texture;
};

} // namespace