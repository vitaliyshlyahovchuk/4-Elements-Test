#include "stdafx.h"
#include "CardFactory.h"
#include "Container.h"
#include "StaticImage.h"
#include "LevelMarker.h"
#include "Gateway.h"
#include "ParalaxCloud.h"

using namespace Card;

std::map<std::string, Card::Factory::CreateItemFunc> Card::Factory::createFunctions;

template <class T>
MapItem* createItem() {
	return new T();
}

void Card::Factory::registerItem() {

	createFunctions["StaticImage"]       = &createItem<StaticImage>;
	createFunctions["Container"]         = &createItem<Container>;;
	createFunctions["LevelMarker"]       = &createItem<LevelMarker>;
	createFunctions["Gateway"]           = &createItem<Gateway>;
	createFunctions["ParalaxCloud"]      = &createItem<ParalaxCloud>;
	createFunctions["ParalaxItemDynamic"]= &createItem<ParalaxItemDynamic>;	
	//createFunctions[""] = &createItem<>;
};

MapItem* Card::Factory::create(Container* parent, rapidxml::xml_node<>* itemInfo) {
	std::string type = Xml::GetStringAttribute(itemInfo, "type");

	if (createFunctions.find(type) == createFunctions.end()) {
		Assert2(false, "Unknown type: \"" + type + "\" for map item");
		return NULL;
	}

	MapItem* item = createFunctions[type]();
	item->init(parent, itemInfo);
	return item;
}