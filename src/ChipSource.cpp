#include "stdafx.h"
#include "ChipSource.h"
#include "Game.h"
#include "EditorUtils.h"
#include "GameField.h"
#include "Match3Gadgets.h"

namespace Gadgets
{

ChipSource::ChipSource(const std::string& pattern, int bomb_moves, bool random_start)
	: _source(pattern)
	, _cr(0)
	, _bomb_moves(bomb_moves)
	, _random_start(random_start)
{
	ProcessSourcePattern();
	if(_random_start) {
		_cr = math::random(0u, _pattern.size()-1);
	}
}

ChipSource::ChipSource(rapidxml::xml_node<> *elem)
	: _cr(0)
	, _bomb_moves(10)
{
	_source = Xml::GetStringAttribute(elem, "pattern");
	_bomb_moves = Xml::GetIntAttributeOrDef(elem, "bomb_moves", 10);
	_random_start = Xml::GetBoolAttributeOrDef(elem, "random_start", false);
	ProcessSourcePattern();
	if(_random_start) {
		_cr = math::random(0u, _pattern.size()-1);
	}
}

void ChipSource::Save(Xml::TiXmlElement *elem, Game::FieldAddress fa)
{
	// по алфавиту!
	elem->SetAttribute("bomb_moves", _bomb_moves);
	elem->SetAttribute("i", fa.GetCol());
	elem->SetAttribute("j", fa.GetRow());
	elem->SetAttribute("pattern", _source);
	elem->SetAttribute("random_start", utils::lexical_cast(_random_start));
}

bool IsDigit(char ch)
{
	return (ch >= '0') && (ch <= '9');
}

ChipSource::Chip ChipSource::ProcessPatternChip(const std::string &pattern, int &idx)
{
	Chip ch;

	// пропускаем пробелы
	while(idx < (int)pattern.size() && pattern[idx] == ' ') ++idx;

	// тип
	if( idx < (int)pattern.size() )
	{
		switch(pattern[idx]) {
			case 'c': ch.type = Chip::CHIP; break;
			case 't': ch.type = Chip::TIME_BOMB; break;
			case 'l': ch.type = Chip::LICORICE; break;
			default: ch.type = Chip::NONE; break;
		};
		++idx;
	}
	else
	{
		ch.type = Chip::NONE;
	}

	// цвет фишки
	std::string number;
	while( idx < (int)pattern.size() && IsDigit(pattern[idx]) )
	{
		number.push_back(pattern[idx]);
		++idx;
	}

	ch.color = number.empty() ? -1 : utils::lexical_cast<int>(number);

	return ch;
}

void ChipSource::ProcessSourcePattern()
{
	int idx = 0;
	Chip ch = ProcessPatternChip(_source, idx);
	while( ch.type != Chip::NONE )
	{
		_pattern.push_back(ch);
		ch = ProcessPatternChip(_source, idx);
	}

	if(_pattern.empty()){
		_pattern.push_back( Chip(Chip::CHIP, -1) );
	}
}

void ChipSource::GenerateChipInSquare(Game::Square *sq)
{
	Chip ch = _pattern[_cr];
	_cr = (_cr + 1) % _pattern.size();
	
	sq->GetChip().Reset(true);
	if( ch.type == Chip::LICORICE ) {
		sq->GetChip().SetLicorice();
	} else if( ch.type == Chip::TIME_BOMB ) {
		sq->GetChip().SetColor( (ch.color >= 0) ? ch.color : Gadgets::levelColors.GetRandom() );
		sq->GetChip().SetTimeBomb(_bomb_moves);
	} else if( ch.type == Chip::CHIP ) {
		sq->GetChip().SetColor( (ch.color >= 0) ? ch.color : Gadgets::levelColors.GetRandom() );
	}
}

void ChipSources::LoadLevel(rapidxml::xml_node<> *root)
{
	rapidxml::xml_node<> *srcElem = root->first_node("ChipSources");
	if( srcElem ) {
		rapidxml::xml_node<> *elem = srcElem->first_node("Source");
		while( elem )
		{
			Game::FieldAddress fa(elem);
			ChipSource::HardPtr src = boost::make_shared<ChipSource>(elem);
			_sources.insert( std::make_pair(fa, src) );
			elem = elem->next_sibling("Source");
		}

		elem = srcElem->first_node("Diamond");
		while( elem )
		{
			_diamondSources.insert( Game::FieldAddress(elem) );
			elem = elem->next_sibling("Diamond");
		}
	}
}

void ChipSources::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *srcElem = root->InsertEndChild(Xml::TiXmlElement("ChipSources"))->ToElement();
	for(Sources::iterator itr = _sources.begin(); itr != _sources.end(); ++itr)
	{
		Xml::TiXmlElement *elem = srcElem->InsertEndChild(Xml::TiXmlElement("Source"))->ToElement();
		//по алфавиту!
		//itr->first.SaveToXml(elem);
		itr->second->Save(elem, itr->first);
	}

	for(Game::FieldAddress fa : _diamondSources)
	{
		Xml::TiXmlElement *elem = srcElem->InsertEndChild(Xml::TiXmlElement("Diamond"))->ToElement();
		fa.SaveToXml(elem);
	}
}

void ChipSources::DrawEdit()
{
	for(Sources::iterator itr = _sources.begin(); itr != _sources.end(); ++itr)
	{
		IRect draw(0, 0, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
		IPoint pos = itr->first.ToPoint() * GameSettings::SQUARE_SIDE;
		Render::device.SetTexturing(false);
		Render::BeginColor( Color(128, 128, 128, 128) );
		Render::DrawRect( draw.MovedTo(pos) );
		Render::EndColor();
		Render::device.SetTexturing(true);

		std::string p = itr->second->_source;
		p.resize(8);

		Render::FreeType::BindFont("editor");
		Render::BeginColor(Color::GREEN);
		Render::PrintString( pos + GameSettings::CELL_HALF.Rounded(), p, 1.0f, CenterAlign, CenterAlign, false);
		Render::EndColor();
	}

	Render::BeginColor(Color::GREEN);
	Render::BindFont("debug");
	for(Game::FieldAddress fa : _diamondSources)
	{	
		FPoint pos = GameSettings::CellCenter(fa.ToPoint()) + FPoint(0.0f, GameSettings::SQUARE_SIDEF * 0.25f);
		Render::PrintString(pos, "D", 1.2f, CenterAlign, CenterAlign);
	}
	Render::EndColor();
}

bool ChipSources::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::ChipSource )
	{
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);
		_selected = _sources.find(fa);

		bool hasCopy = false;
		if(_selected == _sources.end())
		{
			hasCopy = _copy.get();
			ChipSource::HardPtr src = hasCopy ? _copy : boost::make_shared<ChipSource>("c", 10, false);
			_selected = _sources.insert( std::make_pair(fa, src) ).first;
			_copy.reset();
		}

		if( (_selected != _sources.end()) && !hasCopy )
		{
			Layer *layer = Core::guiManager.getLayer("ChipSourceConfig");
			layer->getWidget("Pattern")->AcceptMessage( Message("Set", _selected->second->_source) );
			layer->getWidget("TimeBombMoves")->AcceptMessage( Message("Set", utils::lexical_cast(_selected->second->_bomb_moves)) );
			layer->getWidget("RandomStart")->AcceptMessage( Message("SetState", utils::lexical_cast(_selected->second->_random_start)) );
			Core::mainScreen.pushLayer(layer);
			return true;
		}
	}
	else if (EditorUtils::activeEditBtn == EditorUtils::DiamondChip && Core::mainInput.IsControlKeyDown() )
	{
		if( Game::isVisible(sq) )
			_diamondSources.insert(sq->address);
	}
	return false;
}

bool ChipSources::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::ChipSource )
	{
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);
		if( Core::mainInput.IsControlKeyDown() )
		{
			Sources::iterator itr = _sources.find(fa);
			if( itr != _sources.end() ) {
				_copy = boost::make_shared<ChipSource>(*itr->second);
			}
		}
		else
		{
			_sources.erase(fa);
		}
		return true;
	}
	else if (EditorUtils::activeEditBtn == EditorUtils::DiamondChip && Core::mainInput.IsControlKeyDown() )
	{
		_diamondSources.erase(sq->address);
		return true;
	}
	return false;
}

void ChipSources::Clear()
{
	_sources.clear();
	_diamondSources.clear();
}

ChipSource* ChipSources::GetSource(Game::FieldAddress fa)
{
	Sources::iterator itr = _sources.find(fa);
	return (itr != _sources.end()) ? itr->second.get() : NULL;
}

bool ChipSources::AcceptMessage(const Message &message)
{
	if( message.is("ChipSourceParams") )
	{
		if(_selected != _sources.end())
		{
			std::string pattern = message.getData();
			int bomb_moves = message.variables.getInt("bomb_moves");
			bool random_start = message.variables.getBool("random_start");
			_selected->second = boost::make_shared<ChipSource>(pattern, bomb_moves, random_start);
		}
		return true;
	}
	return false;
}

Game::Square *ChipSources::GetRandomDiamondSource() const
{
	std::vector<Game::Square*> vec;

	for(Game::FieldAddress fa : _diamondSources)
	{
		Game::Square *sq = GameSettings::gamefield[fa];
		if( Game::activeRect.Contains(fa.ToPoint()) && sq->GetChip().IsExist() && sq->GetChip().GetType() != Game::ChipColor::DIAMOND )
			vec.push_back(sq);
	}

	return vec.empty() ? nullptr : vec[math::random(0u, vec.size()-1)];
}

} // Gadgets

