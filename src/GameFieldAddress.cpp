#include "stdafx.h"
#include "GameFieldAddress.h"

#include "Game.h"

namespace Game {

FieldAddress FieldAddress::ZERO = Game::FieldAddress(0, 0);

FieldAddress FieldAddress::UP = Game::FieldAddress(0, +1);

FieldAddress FieldAddress::DOWN = Game::FieldAddress(0, -1);

FieldAddress FieldAddress::LEFT = Game::FieldAddress(-1, 0);

FieldAddress FieldAddress::RIGHT = Game::FieldAddress(+1, 0);

FieldAddress FieldAddress::LEFT_UP = Game::FieldAddress(-1, +1);

FieldAddress FieldAddress::RIGHT_UP = Game::FieldAddress(+1, +1);

FieldAddress FieldAddress::LEFT_DOWN = Game::FieldAddress(-1, -1);

FieldAddress FieldAddress::RIGHT_DOWN = Game::FieldAddress(+1, -1);
	
FieldAddress::FieldAddress(int col, int row)
	: _col(col)
	, _row(row)
{
}

FieldAddress::FieldAddress()
	: _col(-1)
	, _row(-1)
{
}

FieldAddress::FieldAddress(const IPoint& p)
	: _col(p.x)
	, _row(p.y)
{
}

FieldAddress::FieldAddress(rapidxml::xml_node<> *xml_elem)
{
	_col = Xml::GetIntAttributeOrDef(xml_elem, "i", -1);
	_row = Xml::GetIntAttributeOrDef(xml_elem, "j", -1);
}

void FieldAddress::SaveToXml(Xml::TiXmlElement *xml_elem) const
{
	xml_elem->SetAttribute("i", _col);
	xml_elem->SetAttribute("j", _row);
}

int FieldAddress::GetCol() const {
	return _col;
}

int FieldAddress::GetRow() const {
	return _row;
}

IPoint FieldAddress::Get() const 
{
	return IPoint(_col, _row);
}

bool FieldAddress::IsValid() const {
	if (_col < 0) {
		return false;
	}
	if (_col >= GameSettings::FIELD_MAX_WIDTH) {
		return false;
	}
	if (_row < 0) {
		return false;
	}
	if (_row >= GameSettings::FIELD_MAX_HEIGHT) {
		return false;
	}
	return true;
}

bool FieldAddress::operator == (const FieldAddress& right) const {
	return _col == right._col && _row == right._row;
}

bool FieldAddress::operator != (const FieldAddress& right) const {
	return !operator == (right);
}

FieldAddress& FieldAddress::operator += (const FieldAddress& shift) {
	_col += shift._col;
	_row += shift._row;
	return *this;
}

FieldAddress& FieldAddress::operator -= (const FieldAddress& shift) {
	_col -= shift._col;
	_row -= shift._row;
	return *this;
}

const FieldAddress FieldAddress::operator + (const FieldAddress& shift) const {
	return FieldAddress(*this) += shift;
}

const FieldAddress FieldAddress::operator - (const FieldAddress& shift) const {
	return FieldAddress(*this) -= shift;
}

const IPoint FieldAddress::ToPoint() const {
	return IPoint(_col, _row);
}

const FieldAddress FieldAddress::Down() const {
	return FieldAddress(_col, _row - 1);
}

const FieldAddress FieldAddress::Up() const {
	return FieldAddress(_col, _row + 1);
}

const FieldAddress FieldAddress::Left() const {
	return FieldAddress(_col - 1, _row);
}

const FieldAddress FieldAddress::Right() const {
	return FieldAddress(_col + 1, _row);
}

void FieldAddress::SetRow(int row) {
	_row = row;
}

void FieldAddress::SetCol(int col) {
	_col = col;
}

bool FieldAddress::operator < (const FieldAddress& right) const {
	return _col < right._col || (_col == right._col && _row < right._row);
}

FieldAddress FieldAddress::Scale(int scaleCol, int scaleRow) const {
	return FieldAddress(_col * scaleCol, _row * scaleRow);
}

FieldAddress FieldAddress::Shift(int shiftCol, int shiftRow) const {
	return FieldAddress(_col + shiftCol, _row + shiftRow);
}

void FieldAddress::FillDirections4(AddressVector &directions)
{
	directions.clear();
	directions.push_back(*this + DOWN);
	directions.push_back(*this + UP);
	directions.push_back(*this + LEFT);
	directions.push_back(*this + RIGHT);
}

void FieldAddress::FillDirections8(AddressVector &directions)
{
	directions.clear();
	directions.push_back(*this + DOWN);
	directions.push_back(*this + UP);
	directions.push_back(*this + LEFT);
	directions.push_back(*this + RIGHT);
	directions.push_back(*this + LEFT_DOWN);
	directions.push_back(*this + LEFT_UP);
	directions.push_back(*this + RIGHT_DOWN);
	directions.push_back(*this + RIGHT_UP);
}

} // namespace Game
