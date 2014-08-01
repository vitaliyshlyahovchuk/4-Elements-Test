#pragma once

namespace Card {

class MapItem;
class Container;

class Factory {
public:
	// ����������� ���� ���������
	static void registerItem();
	// ������� �������� ��� MapItem-��
	static MapItem* create(Container* parent, rapidxml::xml_node<>* itemInfo);
private:
	typedef MapItem* (*CreateItemFunc)();
	static std::map<std::string, CreateItemFunc> createFunctions;
};

} // namespace