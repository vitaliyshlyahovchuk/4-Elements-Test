#pragma once

namespace Card {

class MapItem;
class Container;

class Factory {
public:
	// регистрация всех элементов
	static void registerItem();
	// фабрика создания все MapItem-ов
	static MapItem* create(Container* parent, rapidxml::xml_node<>* itemInfo);
private:
	typedef MapItem* (*CreateItemFunc)();
	static std::map<std::string, CreateItemFunc> createFunctions;
};

} // namespace