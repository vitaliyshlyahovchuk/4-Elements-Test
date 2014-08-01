#include "stdafx.h"
#include "StaticImage.h"

using namespace Card;

void StaticImage::init(Container* parent, rapidxml::xml_node<>* info) {
	MapItem::init(parent, info);
	texture = Core::resourceManager.Get<Render::Texture>(name);
	Assert(texture);
	rect = texture->getBitmapRect().MovedBy(position.Rounded());
}

void StaticImage::Draw(const FPoint& shift) {
	texture->Draw(position + shift);
	MapItem::Draw(shift);
}
