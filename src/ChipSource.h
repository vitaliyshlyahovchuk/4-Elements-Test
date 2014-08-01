#ifndef __CHIP_SOURCE_H__
#define __CHIP_SOURCE_H__

#include "BaseEditorMaker.h"

namespace Gadgets
{

/*
l - лакрица
t - тайм-бомба
c - обычная фишка
*/

class ChipSource
{
	struct Chip
	{
		enum Type
		{
			NONE, CHIP, LICORICE, TIME_BOMB
		};
		Type type;
		int color;

		Chip() : type(NONE), color(-1) {}
		Chip(Type t, int c) : type(t), color(c) {}
	};
	std::string _source;
	std::vector<Chip> _pattern;
	bool _random_start;
	size_t _cr;

	int _bomb_moves;

	Chip ProcessPatternChip(const std::string& pattern, int &idx);
	void ProcessSourcePattern();

	friend class ChipSources;
public:
	ChipSource(const std::string& pattern, int bomb_moves, bool random_start);
	ChipSource(rapidxml::xml_node<> *elem);

	void Save(Xml::TiXmlElement *elem, Game::FieldAddress fa);

	void GenerateChipInSquare(Game::Square *sq);

	typedef boost::shared_ptr<ChipSource> HardPtr;
};

class ChipSources : public BaseEditorMaker
{
	typedef std::map<Game::FieldAddress, ChipSource::HardPtr> Sources;
	Sources _sources;
	Sources::iterator _selected;
	ChipSource::HardPtr _copy;

	std::set<Game::FieldAddress> _diamondSources;
public:
	void LoadLevel(rapidxml::xml_node<> *root);
	void SaveLevel(Xml::TiXmlElement *root);

	void Clear();
	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	void DrawEdit();
	bool AcceptMessage(const Message &message);

	ChipSource* GetSource(Game::FieldAddress fa);
	Game::Square *GetRandomDiamondSource() const;
};

} //Gadgets

#endif