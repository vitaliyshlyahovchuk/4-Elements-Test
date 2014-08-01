#pragma once

#include "BaseEditorMaker.h"
#include "GameSquare.h"
#include "GameFieldAddress.h"

namespace Gadgets
{

struct BearType
{
	IPoint size;
	std::string texID;

	BearType(IPoint s, const std::string& tex) : size(s), texID(tex) {}
};

struct Bear
{
	IRect rect;
	int type;
	Render::Texture *tex;

	Bear() : rect(0,0,1,1), tex(nullptr), type(0) {}

	Bear(IPoint pos, const BearType &type, int t)
		: rect(pos.x, pos.y, type.size.x, type.size.y)
		, tex( Core::resourceManager.Get<Render::Texture>(type.texID) )
		, type(t)
	{}

	bool IsFilled() const;
};

class FieldBears
	: public BaseEditorMaker
{
	typedef std::list<Bear> BearList;

	BearList _bears;
	std::vector<BearType> _bearTypes;

	int _type;
	int _totalBears;

	Bear _currentBear;
	bool _currentRectValid;

	bool _drawOverlay;

	BearList::iterator FindBearOnSquare(IPoint pos);
	bool CheckRect(IRect area);
public:
	void Init(GameField *field, const bool &with_editor) override;

	void LoadLevel(rapidxml::xml_node<> *root) override;
	void SaveLevel(Xml::TiXmlElement *root) override;

	void Clear() override;

	int TotalBears() const;
	int BearsCount() const;
	int FoundBears() const;

	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq) override;
	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq) override;
	bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq) override;
	bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq) override;
	bool Editor_MouseWheel(int delta, Game::Square *sq) override;

	void Draw(bool on_energy);
	void DrawOverlay();
	void DrawEdit() override;

	void Update();
};

extern FieldBears bears;

} // Gadgets