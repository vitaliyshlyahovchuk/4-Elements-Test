#include "stdafx.h"
#include "Game.h"
#include "CellWalls.h"
#include "EditorUtils.h"
#include "Match3.h"

namespace Gadgets
{

CellWalls cellWalls;

void CellWalls::Init(GameField *field, const bool &with_editor)
{
	BaseEditorMaker::Init(field, with_editor);

	_editCell = Game::FieldAddress(-1,-1);
	_editPiece = 0;
}

void CellWalls::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear();
	rapidxml::xml_node<> *xml_cells = root->first_node("CellWalls");
	if( xml_cells )
	{
		rapidxml::xml_node<> *xml_cell = xml_cells->first_node("Cell");
		while(xml_cell)
		{
			Game::FieldAddress fa(xml_cell);
			BYTE dir = Xml::GetIntAttributeOrDef(xml_cell, "walls_energy", 0);
			if( dir > 0 ) {
				_wallsEnergy.insert( std::make_pair(fa, dir) );
			}
			dir = Xml::GetIntAttributeOrDef(xml_cell, "walls_chip", 0);
			if( dir > 0 ) {
				_wallsChip.insert( std::make_pair(fa, dir) );
				AddChipWalls(fa, dir);
			}
			xml_cell = xml_cell->next_sibling("Cell");
		}
	}
}

void CellWalls::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *xml_cells = root->InsertEndChild(Xml::TiXmlElement("CellWalls"))->ToElement();
	for(WallMap::const_iterator itr = _wallsEnergy.begin(); itr != _wallsEnergy.end(); ++itr)
	{
		Xml::TiXmlElement *xml_cell = xml_cells->InsertEndChild(Xml::TiXmlElement("Cell"))->ToElement();
		if(itr->second > 0) {
			itr->first.SaveToXml(xml_cell);
			xml_cell->SetAttribute("walls_energy", utils::lexical_cast(itr->second));
		}
	}
	for(WallMap::const_iterator itr = _wallsChip.begin(); itr != _wallsChip.end(); ++itr)
	{
		Xml::TiXmlElement *xml_cell = xml_cells->InsertEndChild(Xml::TiXmlElement("Cell"))->ToElement();
		if(itr->second > 0) {
			itr->first.SaveToXml(xml_cell);
			xml_cell->SetAttribute("walls_chip", utils::lexical_cast(itr->second));
		}
	}
}

void CellWalls::Clear()
{
	for(WallMap::const_iterator itr = _wallsChip.begin(); itr != _wallsChip.end(); ++itr)
	{
		GameSettings::gamefield[itr->first]->cellWallsChip = 0;
	}
	_wallsEnergy.clear();
	_wallsChip.clear();
}

bool CellWalls::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::CellWalls || EditorUtils::activeEditBtn == EditorUtils::CellWallsChip)
	{
		return true;
	}
	return false;
}

bool CellWalls::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::CellWalls || EditorUtils::activeEditBtn == EditorUtils::CellWallsChip)
	{
		PieceUnderMousePos(mouse_pos, _editCell, _editPiece);
		return true;
	}
	return false;
}

bool CellWalls::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::CellWalls )
	{
		PieceUnderMousePos(mouse_pos, _editCell, _editPiece);
		
		if(_editPiece > 0) {
			std::pair<WallMap::iterator, bool> result = _wallsEnergy.insert( std::make_pair(_editCell, _editPiece) );
			if( !result.second )
			{
				result.first->second = result.first->second ^ _editPiece;
			}
		}

		return true;
	}
	else if( EditorUtils::activeEditBtn == EditorUtils::CellWallsChip )
	{
		PieceUnderMousePos(mouse_pos, _editCell, _editPiece);
		
		if(_editPiece > 0) {
			std::pair<WallMap::iterator, bool> result = _wallsChip.insert( std::make_pair(_editCell, _editPiece) );
			if( !result.second )
			{
				result.first->second = result.first->second ^ _editPiece;
			}
		}

		return true;
	}
	return false;
}

bool CellWalls::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::CellWalls || EditorUtils::activeEditBtn == EditorUtils::CellWallsChip)
	{
		PieceUnderMousePos(mouse_pos, _editCell, _editPiece);
		return true;
	}
	return false;
}

void CellWalls::AddChipWalls(Game::FieldAddress fa, BYTE pieces)
{
	Game::Square *sq = GameSettings::gamefield[fa];
	if( Game::isVisible(sq) )
		sq->cellWallsChip |= pieces;
	
	sq = GameSettings::gamefield[fa.Left()];
	if( Game::isVisible(sq) && (pieces & LEFT) ) {
		sq->cellWallsChip |= RIGHT;
	}
	
	sq = GameSettings::gamefield[fa.Down()];
	if( Game::isVisible(sq) && (pieces & DOWN)) {
		sq->cellWallsChip |= UP;
	}
}

void CellWalls::RemoveChipWalls(Game::FieldAddress fa, BYTE pieces)
{
	Game::Square *sq = GameSettings::gamefield[fa];
	if( Game::isVisible(sq) )
		sq->cellWallsChip &= (~pieces);
	
	sq = GameSettings::gamefield[fa.Left()];
	if( Game::isVisible(sq) && (pieces & LEFT) ) {
		sq->cellWallsChip &= (~RIGHT);
	}
	
	sq = GameSettings::gamefield[fa.Down()];
	if( Game::isVisible(sq) && (pieces & DOWN)) {
		sq->cellWallsChip &= (~UP);
	}
}

void CellWalls::RemoveChipWall(Game::FieldAddress fa, BYTE piece)
{
	Assert( (piece & RIGHT) == 0 );
	Assert( (piece & UP) == 0 );

	WallMap::iterator itr = _wallsChip.find(fa);
	if( itr != _wallsChip.end() )
	{
		itr->second = (itr->second & (~piece));
		RemoveChipWalls(fa, piece);
		if( itr->second == 0 )
			_wallsChip.erase(itr);

		Match3::RunFallColumn(fa.GetCol() - 1);
		Match3::RunFallColumn(fa.GetCol());
		Match3::RunFallColumn(fa.GetCol() + 1);
	}
}

void CellWalls::DrawHorizontal(Game::FieldAddress fa)
{
	FPoint pos = FPoint(fa.ToPoint()) * GameSettings::SQUARE_SIDEF;

	FPoint p1 = FPoint(0.0f, 0.0625f) * GameSettings::SQUARE_SIDEF;
	FPoint p2 = FPoint(1.0f, 0.0625f) * GameSettings::SQUARE_SIDEF;
	FPoint p3 = FPoint(0.0f, -0.0625f) * GameSettings::SQUARE_SIDEF;
	FPoint p4 = FPoint(1.0f, -0.0625f) * GameSettings::SQUARE_SIDEF;

	Render::DrawQuad(pos + p1, pos + p2, pos + p3, pos + p4);
}

void CellWalls::DrawVertical(Game::FieldAddress fa)
{
	FPoint pos = FPoint(fa.ToPoint()) * GameSettings::SQUARE_SIDEF;

	FPoint p1 = FPoint(-0.0625f, 1.0f) * GameSettings::SQUARE_SIDEF;
	FPoint p2 = FPoint(0.0625f, 1.0f) * GameSettings::SQUARE_SIDEF;
	FPoint p3 = FPoint(-0.0625f, 0.0f) * GameSettings::SQUARE_SIDEF;
	FPoint p4 = FPoint(0.0625f, 0.0f) * GameSettings::SQUARE_SIDEF;

	Render::DrawQuad(pos + p1, pos + p2, pos + p3, pos + p4);
}

void CellWalls::Draw()
{
	Render::device.SetTexturing(false);

	Render::BeginColor(Color(50, 50, 150, 255));
	for(WallMap::const_iterator itr = _wallsEnergy.begin(); itr != _wallsEnergy.end(); ++itr)
	{
		DrawPiece(itr->first, itr->second);
	}
	Render::EndColor();

	Render::BeginColor(Color(60, 60, 60, 255));
	for(WallMap::const_iterator itr = _wallsChip.begin(); itr != _wallsChip.end(); ++itr)
	{
		DrawPiece(itr->first, itr->second);
	}
	Render::EndColor();

	Render::device.SetTexturing(true);
}

void CellWalls::DrawPiece(Game::FieldAddress fa, BYTE piece)
{
	if(piece & DOWN)
		DrawHorizontal(fa);
	if(piece & LEFT)
		DrawVertical(fa);
}

void CellWalls::DrawEdit()
{
	Render::device.SetTexturing(false);
	Render::BeginColor(Color(70, 70, 255, 150));
	DrawPiece(_editCell, _editPiece);
	Render::EndColor();
	Render::device.SetTexturing(true);
}

void CellWalls::PieceUnderMousePos(IPoint mouse_pos, Game::FieldAddress &fa, BYTE &type)
{
	fa = GameSettings::GetMouseAddress(mouse_pos);

	IPoint pos = GameSettings::ToFieldPos(mouse_pos) - fa.ToPoint() * GameSettings::SQUARE_SIDE;

	FPoint p = FPoint(pos) / GameSettings::SQUARE_SIDEF;

	if( p.x > 0.75f ) {
		p.x -= 1.0f;
		fa = fa.Shift(1, 0);
	}
	if( p.y > 0.75f ) {
		p.y -= 1.0f;
		fa = fa.Shift(0, 1);
	}

	if( p.x < 0.25f && p.x > -0.25f && p.y > 0.25f) {
		type = LEFT;
	} else if( p.x > 0.25f && p.y < 0.25f && p.y > -0.25f) {
		type = DOWN;
	} else {
		type = 0;
	}
}

BYTE CellWalls::EnergyWallsInCell(Game::FieldAddress fa) const
{
	WallMap::const_iterator itr = _wallsEnergy.find(fa);
	BYTE result = (itr != _wallsEnergy.end()) ? itr->second : 0;

	itr = _wallsEnergy.find(fa.Up());
	if( itr != _wallsEnergy.end() && (itr->second & DOWN) )
		result |= UP;

	itr = _wallsEnergy.find(fa.Right());
	if( itr != _wallsEnergy.end() && (itr->second & LEFT) )
		result |= RIGHT;

	return result;
}

} // Gadgets