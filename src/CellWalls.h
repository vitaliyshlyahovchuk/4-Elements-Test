#ifndef __CELL_WALLS_H__
#define __CELL_WALLS_H__

#include "BaseEditorMaker.h"
#include "GameSquare.h"
#include "GameFieldAddress.h"

namespace Gadgets
{

class CellWalls
	: public BaseEditorMaker
{
	typedef std::map<Game::FieldAddress, BYTE> WallMap;
	WallMap _wallsEnergy;
	WallMap _wallsChip;

	void AddChipWalls(Game::FieldAddress fa, BYTE pieces);
	void RemoveChipWalls(Game::FieldAddress fa, BYTE pieces);

	void DrawPiece(Game::FieldAddress fa, BYTE piece);
	void DrawHorizontal(Game::FieldAddress fa);
	void DrawVertical(Game::FieldAddress fa);

	void PieceUnderMousePos(IPoint mouse_pos, Game::FieldAddress &fa, BYTE &type);

	Game::FieldAddress _editCell;
	BYTE _editPiece;
public:
	static const BYTE LEFT  = 0x01;
	static const BYTE DOWN  = 0x02;
	static const BYTE RIGHT = 0x04;
	static const BYTE UP    = 0x08;

	void Init(GameField *field, const bool &with_editor);

	void LoadLevel(rapidxml::xml_node<> *root);
	void SaveLevel(Xml::TiXmlElement *root);

	void Clear();

	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);

	void Draw();
	void DrawEdit();

	void RemoveChipWall(Game::FieldAddress fa, BYTE piece);

	BYTE EnergyWallsInCell(Game::FieldAddress fa) const;
};

extern CellWalls cellWalls;

} // Gadgets

#endif