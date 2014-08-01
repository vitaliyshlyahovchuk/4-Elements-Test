#include "stdafx.h"

#include "SnapGadgetsClass.h"
#include "GameField.h"
#include "Game.h"
#include "Match3Gadgets.h"
#include "EditorUtils.h"
#include "Energy.h"
#include "EnergyReceivers.h"
#include "SquareNewInfo.h"
#include "Place2D.h"

Color inColor = Color(0, 255, 0, 127);
Color outColor1 = Color(255, 0, 0, 127);
Color outColor2 = Color(0, 0, 255, 127);
Color textColor = Color(255, 255, 255, 223);
Color centerColor = Color(255, 127, 0, 191);
Color outBorder = Color("#FA06F1");


int out_count = 0;
int in_count = 0;

/*
 *
 * SnapGadgetBaseElement
 *
 */


SnapGadgetBaseElement::SnapGadgetBaseElement()
	: activateType(ACTIVATE_TYPE_AND)
	, cell_index(-1, -1)
{

}

SnapGadgetBaseElement::~SnapGadgetBaseElement()
{

}

bool SnapGadgetBaseElement::IsActivateType(bool type)
{
	return activateType == type;
}

IPoint SnapGadgetBaseElement::GetIndex()
{
	return cell_index;
}

SnapGadgetBaseElement::SHARED_PTR SnapGadgetBaseElement::Clone(SnapGadgetBaseElement::SHARED_PTR other)
{
	SnapGadgetBaseElement::SHARED_PTR e_new;
	if(SnapGadgetCaptureElement::Is(other.get()))
	{
		e_new = SnapGadgetCaptureElement::SHARED_PTR(new SnapGadgetCaptureElement());
	}else if(SnapGadgetReleaseElement::Is(other.get()))
	{
		e_new = SnapGadgetReleaseElement::SHARED_PTR(new SnapGadgetReleaseElement());
	}else if(SnapGadgetReleaseBorderElement::Is(other.get()))
	{
		e_new = SnapGadgetReleaseBorderElement::SHARED_PTR(new SnapGadgetReleaseBorderElement());
	}
	Assert(e_new.get());
	e_new->SetIndex(other->GetIndex());
	e_new->activateType = other->activateType;
	return e_new;
}


/*
 *
 * SnapGadgetCaptureElement
 *
 */

SnapGadgetCaptureElement::SnapGadgetCaptureElement()
{
}

SnapGadgetCaptureElement::~SnapGadgetCaptureElement()
{

}

bool SnapGadgetCaptureElement::SetIndex(const IPoint &index_new)
{
	cell_index = index_new;
	FPoint p1;
	p1.x = (cell_index.x + 0.5f)*GameSettings::SQUARE_SIDEF;
	p1.y = (cell_index.y + 0.5f)*GameSettings::SQUARE_SIDEF;
	_draw_rect = IRect(p1.x, p1.y, 0 , 0).Inflated(GameSettings::SQUARE_SIDE/3);
	_draw_rect.MoveBy(IPoint(GameSettings::SQUARE_SIDE/10, -GameSettings::SQUARE_SIDE/10));
	return true;
}

void SnapGadgetCaptureElement::Edit_Draw(FPoint snapPoint)
{
	Render::device.SetTexturing(false);
	Render::BeginColor(inColor);

	FPoint p1;
	FPoint p0;

	p0.x = snapPoint.x;
	p0.y = snapPoint.y;
	p1.x = (cell_index.x + 0.5f)*GameSettings::SQUARE_SIDEF;
	p1.y = (cell_index.y + 0.5f)*GameSettings::SQUARE_SIDEF;

	Render::DrawLine(p0, p1);

	Render::DrawRect(_draw_rect);
	Render::DrawFrame(_draw_rect);

	Render::EndColor();
	Render::device.SetTexturing(true);


	// Риcуем текcт для квадратов
	Render::BeginColor(textColor);
	Render::FreeType::BindFont("editor");
	std::string  str;	
	int x = cell_index.x * GameSettings::SQUARE_SIDE;
	int y = cell_index.y * GameSettings::SQUARE_SIDE;
	str = "grab " + utils::lexical_cast(++in_count);
	Render::PrintString(x, y + GameSettings::SQUARE_SIDE - 16, str, 1.f, LeftAlign, TopAlign);
	Render::EndColor();
}

bool SnapGadgetCaptureElement::IsUnderMouse(IPoint mouse_pos)
{
	return _draw_rect.Contains(mouse_pos);
}

void SnapGadgetCaptureElement::Editor_MoveElements(const IRect& part, const IPoint& delta)
{
	if(part.Contains(cell_index))
	{
		SetIndex(GetIndex() + delta);
	}
}

bool SnapGadgetCaptureElement::IsStayOnVisibleSquare()
{
	return Game::isVisible(Game::FieldAddress(cell_index));
}

void SnapGadgetCaptureElement::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *element = root -> InsertEndChild(Xml::TiXmlElement("Item")) -> ToElement();
	// по алфавиту
	element -> SetAttribute("type", "0");
	element -> SetAttribute("x", utils::lexical_cast(cell_index.x));
	element -> SetAttribute("y", utils::lexical_cast(cell_index.y));
}

void SnapGadgetCaptureElement::LoadLevel(rapidxml::xml_node<> *xml_elem)
{
	SetIndex(xml_elem);
	activateType = Xml::GetBoolAttributeOrDef(xml_elem, "relese_type", false);
}

/*
 *
 * SnapGadgetReleaseElement
 *
 */

SnapGadgetReleaseElement::SnapGadgetReleaseElement()
{
}

SnapGadgetReleaseElement::~SnapGadgetReleaseElement()
{

}

bool SnapGadgetReleaseElement::SetIndex(const IPoint &index_new)
{
	cell_index = index_new;
	FPoint p1;
	p1.x = (cell_index.x + 0.5f)*GameSettings::SQUARE_SIDEF;
	p1.y = (cell_index.y + 0.5f)*GameSettings::SQUARE_SIDEF;
	_draw_rect = IRect(p1.x, p1.y, 0 , 0).Inflated(GameSettings::SQUARE_SIDE/3);
	_draw_rect.MoveBy(IPoint(-GameSettings::SQUARE_SIDE/10, GameSettings::SQUARE_SIDE/10));
	return true;
}

void SnapGadgetReleaseElement::Edit_Draw(FPoint snapPoint)
{
	Render::device.SetTexturing(false);
	if(IsActivateType(ACTIVATE_TYPE_OR))
	{
		Render::BeginColor(outColor1);
	}else{
		Render::BeginColor(outColor2);
	}

	FPoint p1;
	FPoint p0;

	p0.x = snapPoint.x;
	p0.y = snapPoint.y;

	p1.x = (cell_index.x + 0.5f)*GameSettings::SQUARE_SIDEF;
	p1.y = (cell_index.y + 0.5f)*GameSettings::SQUARE_SIDEF;

	Render::DrawLine(p0, p1);

	Render::DrawRect(_draw_rect);
	Render::DrawFrame(_draw_rect);

	Render::EndColor();

	Render::device.SetTexturing(true);

	Render::BeginColor(textColor);
	Render::FreeType::BindFont("editor");
	std::string  str;
	int x = cell_index.x * GameSettings::SQUARE_SIDE;
	int y = cell_index.y * GameSettings::SQUARE_SIDE;
	Render::PrintString(x + GameSettings::SQUARE_SIDE*4/9,y + GameSettings::SQUARE_SIDE*5/9, activateType == ACTIVATE_TYPE_OR ? "OR" : "AND", 1.5f, CenterAlign, CenterAlign);		
	str = "drop " + utils::lexical_cast(++out_count);
	Render::PrintString(x, y, str, 1.f, LeftAlign, BottomAlign);
	Render::EndColor();
}

bool SnapGadgetReleaseElement::IsStayOnVisibleSquare()
{
	return Game::isVisible(Game::FieldAddress(cell_index));
}

bool SnapGadgetReleaseElement::CheckElementsOnRelease(bool lastReceiver, bool future, bool &or_elements_exist, bool &release_elements_exist, bool &any_release_element_activated, bool &all_release_element_activated)
{
	Game::Square *sq = GameSettings::gamefield[cell_index];
	if (Game::isBuffer(sq)){
		return true;
	}

	// не отпускаем камеру если точка отпускания стоит на последнем приемнике
	// чтобы она никуда не уезжала после окончания уровня
	if( lastReceiver && Gadgets::receivers.GetReceiverOnSquare(sq->address) && Gadgets::receivers.IsLastReceiver(sq->address)) 
	{
		return false;
	}

	if(IsActivateType(ACTIVATE_TYPE_OR))
	{
		or_elements_exist = true;
		release_elements_exist = true;
		if(sq->IsEnergyChecked(future))
		{
			any_release_element_activated = true;
		}
	}
	else if(IsActivateType(ACTIVATE_TYPE_AND))
	{
		release_elements_exist = true;
		if(!sq->IsEnergyChecked(future))
		{
			all_release_element_activated = false;
		}
	}
	return true;
}

bool SnapGadgetReleaseElement::IsUnderMouse(IPoint mouse_pos)
{
	return _draw_rect.Contains(mouse_pos);
}

void SnapGadgetReleaseElement::Editor_MoveElements(const IRect& part, const IPoint& delta)
{
	if(part.Contains(cell_index))
	{
		SetIndex(GetIndex() + delta);
	}
}

void SnapGadgetReleaseElement::LoadLevel(rapidxml::xml_node<> *xml_elem)
{
	SetIndex(xml_elem);
	activateType = Xml::GetBoolAttributeOrDef(xml_elem, "relese_type", false);
}

void SnapGadgetReleaseElement::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *element = root -> InsertEndChild(Xml::TiXmlElement("Item")) -> ToElement();
	// по алфавиту
	element -> SetAttribute("relese_type", utils::lexical_cast<bool>(activateType));
	element -> SetAttribute("type", "1");
	element -> SetAttribute("x", utils::lexical_cast(cell_index.x));
	element -> SetAttribute("y", utils::lexical_cast(cell_index.y));
}

/*
 *
 * SnapGadgetReleaseBorderElement
 *
 */

void BorderInfo::Draw(FPoint start)
{
	start += FPoint(index_offset)*GameSettings::SQUARE_SIDEF;
	float angle = dir*math::PI*0.25f;
	{
		Render::BeginColor(Color(100, 100, 100));
		FPoint p0 = start + FPoint(GameSettings::SQUARE_SIDEF*0.25f, 0.f).Rotated(angle);
		FPoint p1 = start + FPoint(GameSettings::SQUARE_SIDEF*0.71f, 0.f).Rotated(angle - math::PI*0.25f);
		FPoint p2 = start + FPoint(GameSettings::SQUARE_SIDEF*0.71f, 0.f).Rotated(angle + math::PI*0.25f);
		Render::device.DrawTriangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f);
		Render::EndColor();
	}
	{
		FPoint p0 = start + FPoint(GameSettings::SQUARE_SIDEF*0.29f, 0.f).Rotated(angle);
		FPoint p1 = start + FPoint(GameSettings::SQUARE_SIDEF*0.60f, 0.f).Rotated(angle - math::PI*0.21f);
		FPoint p2 = start + FPoint(GameSettings::SQUARE_SIDEF*0.60f, 0.f).Rotated(angle + math::PI*0.21f);
		Render::device.DrawTriangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f);
	}
}

void BorderInfo::LoadLevel(rapidxml::xml_node<> *xml_elem)
{
	dir = Xml::GetIntAttribute(xml_elem, "dir")*2;
	index_offset.x = Xml::GetIntAttribute(xml_elem, "x_offset");
	index_offset.y = Xml::GetIntAttribute(xml_elem, "y_offset");
}

void BorderInfo::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *element = root -> InsertEndChild(Xml::TiXmlElement("border")) -> ToElement();
	element -> SetAttribute("dir", utils::lexical_cast(dir/2));
	element -> SetAttribute("x_offset", utils::lexical_cast(index_offset.x));
	element -> SetAttribute("y_offset", utils::lexical_cast(index_offset.y));
}

const IPoint BorderInfo::dirs[8] = {IPoint(1, 0), IPoint(1, 1), IPoint(0, 1), IPoint(-1,1), IPoint(-1, 0), IPoint(-1, -1), IPoint(0, -1), IPoint(1, -1)};

IPoint BorderInfo::Get()
{
	return index_offset + dirs[dir];
}

int SnapGadgetReleaseBorderElement::RELEASED_SQUARES = 0;
int SnapGadgetReleaseBorderElement::LEVEL_SQUARES_COUNT = 0;
SnapGadgetReleaseBorderElement *more_far_release_border_element = 0;



SnapGadgetReleaseBorderElement::SnapGadgetReleaseBorderElement()
	: _editBorder(0)
	, _countSellForWaitEnergy(0)
	, _alphaTimer(0.f)
{
	_borders.push_back(BorderInfo());
	BorderInfo &info = _borders.back();
	info.dir = 0;
	info.index_offset = IPoint(0,0);
}

SnapGadgetReleaseBorderElement::~SnapGadgetReleaseBorderElement()
{

}

Color SnapGadgetReleaseBorderElement::COLOR_LIGHT = Color(0,0,200,200);
bool SnapGadgetReleaseBorderElement::ENERGY_LEVEL_LIGHT_SHOW = false;

void SnapGadgetReleaseBorderElement::DrawGame()
{
	if(!SnapGadgetReleaseBorderElement::ENERGY_LEVEL_LIGHT_SHOW)
	{
		return;
	}
	Place2D::Clear();
	for(auto i :_lightSquares)
	{
		Place2D::AddAddress(i);
	}
	Place2D::misc_borders.clear();
	Place2D::BindBorders(&Place2D::misc_borders);
	Place2D::CalculateWithoutVisability();
	Render::device.SetBlendMode(Render::ADD);
	Color clr = SnapGadgetReleaseBorderElement::COLOR_LIGHT;
	clr.alpha = math::lerp(0, 255, _alphaTimer);
	Place2D::DrawPlaceWithoutVisability(clr, GameSettings::CELL_RECT, 0.f, "energy_level");
	Place2D::DrawBorders(Place2D::misc_borders, "energy_level", clr);
	Render::device.SetBlendMode(Render::ALPHA);
}

void SnapGadgetReleaseBorderElement::Update(float dt)
{
	if(_alphaTimer < 1)
	{
		_alphaTimer += dt;
		if(_alphaTimer >= 1.f)
		{
			_alphaTimer = 1.f;
		}
	}
}


void SnapGadgetReleaseBorderElement::Edit_Draw(FPoint snapPoint)
{
	Render::device.SetTexturing(false);
	Render::BeginColor(Color(244, 44, 255));

	FPoint p1;
	FPoint p0;

	p0.x = snapPoint.x;
	p0.y = snapPoint.y;

	p1.x = (cell_index.x + 0.5f)*GameSettings::SQUARE_SIDEF;
	p1.y = (cell_index.y + 0.5f)*GameSettings::SQUARE_SIDEF;

	Render::DrawLine(p0, p1);

	Render::DrawRect(_draw_rect);
	Render::DrawFrame(_draw_rect);
	Render::EndColor();

	Render::BeginColor(Color(32,124,32, 100));
	for(std::list<BorderInfo>::iterator i = _borders.begin(); i != _borders.end(); i++)
	{
		i->Draw(p1);
	}
	Render::EndColor();

	Render::device.SetTexturing(true);

	//Render::BeginColor(textColor);
	//Render::FreeType::BindFont("editor");
	//std::string  str;
	//int x = cell_index.x * GameSettings::SQUARE_SIDE;
	//int y = cell_index.y * GameSettings::SQUARE_SIDE;
	//Render::PrintString(x + GameSettings::SQUARE_SIDE*4/9,y + GameSettings::SQUARE_SIDE*5/9, activateType == ACTIVATE_TYPE_OR ? "OR" : "AND", 1.5f, CenterAlign, CenterAlign);		
	//str = "drop " + utils::lexical_cast(++out_count);
	//Render::PrintString(x, y, str, 1.f, LeftAlign, BottomAlign);
	//Render::EndColor();
}

bool SnapGadgetReleaseBorderElement::IsStayOnVisibleSquare()
{
	return true;
}

bool SnapGadgetReleaseBorderElement::CheckElementsOnRelease(bool lastReceiver, bool future, bool &or_elements_exist, bool &release_elements_exist, bool &any_release_element_activated, bool &all_release_element_activated)
{
	release_elements_exist = true;
	if(_countSellForWaitEnergy == 0)
	{
		any_release_element_activated = true;
	}else{
		all_release_element_activated = false;
	}
	return true;
}

bool SnapGadgetReleaseBorderElement::IsUnderMouse(IPoint mouse_pos)
{
	return _draw_rect.Contains(mouse_pos);
}

void SnapGadgetReleaseBorderElement::Editor_MoveElements(const IRect& part, const IPoint& delta)
{
	if(part.Contains(GetIndex()))
	{
		SetIndex(GetIndex() + delta);
	}
}

bool SnapGadgetReleaseBorderElement::SetIndex(const IPoint &index)
{
	cell_index = index;
	_draw_rect = IRect(cell_index.x*GameSettings::SQUARE_SIDE, cell_index.y*GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDEF , GameSettings::SQUARE_SIDEF);
	return true;
}

void SnapGadgetReleaseBorderElement::LoadLevel(rapidxml::xml_node<> *xml_elem)
{
	_borders_for_check.clear();
	SetIndex(xml_elem);
	activateType = Xml::GetBoolAttributeOrDef(xml_elem, "relese_type", false);
	_borders.clear();
	rapidxml::xml_node<> *xml_border = xml_elem->first_node("border");
	while(xml_border)
	{
		_borders.push_back(BorderInfo());
		_borders.back().LoadLevel(xml_border);
		xml_border = xml_border->next_sibling("border");
	}
}

void SnapGadgetReleaseBorderElement::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *element = root -> InsertEndChild(Xml::TiXmlElement("Item")) -> ToElement();
	// по алфавиту
	element -> SetAttribute("relese_type", utils::lexical_cast<bool>(activateType));
	element -> SetAttribute("type", utils::lexical_cast("2"));
	element -> SetAttribute("x", utils::lexical_cast(cell_index.x));
	element -> SetAttribute("y", utils::lexical_cast(cell_index.y));
	for(std::list<BorderInfo>::iterator i = _borders.begin(); i != _borders.end(); i++)
	{
		i->SaveLevel(element);
	}
}

Byte GetDir(FPoint element_pos, FPoint mouse_field_pos)
{
	FPoint dir = mouse_field_pos - (element_pos*GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF);
	float angle = FPoint(1.f, 0.f).GetDirectedAngleNormalize(dir)*180.f/math::PI + 45.f;
	Byte dir_id =  int(angle/90.f) % 4;
	dir_id = dir_id << 1; //*2
	return dir_id;
}

bool SnapGadgetReleaseBorderElement::Editor_MouseDown(const IPoint &mouse_pos)
{
	return false;
}

void SnapGadgetReleaseBorderElement::PrepareLevel()
{
	SnapGadgetReleaseBorderElement::ENERGY_LEVEL_LIGHT_SHOW = Gadgets::levelSettings.getString("EnergyLevelShowLight") == "true";
	_lightSquares.clear();
	_borders_for_check.clear();
	std::list<BorderInfo>::iterator i1, i2, i1_next;
	for( i1= _borders.begin(); i1 != _borders.end(); i1++)
	{
		_borders_for_check.push_back(*i1);
		IPoint offset = i1->index_offset;
		unsigned int dir_id = i1->dir + 8;
		IPoint dir = BorderInfo::dirs[(dir_id % 8)];

		//Закрываем диагональные проходы
		_borders_for_check.push_back(BorderInfo());
		_borders_for_check.back().index_offset = offset;
		_borders_for_check.back().dir = (dir_id + 1) % 8;

		_borders_for_check.push_back(BorderInfo());
		_borders_for_check.back().index_offset = offset;
		_borders_for_check.back().dir = (dir_id - 1) % 8;

		_borders_for_check.push_back(BorderInfo());
		_borders_for_check.back().index_offset = offset + IPoint(-dir.y, dir.x);
		_borders_for_check.back().dir = (dir_id - 1) % 8;

		_borders_for_check.push_back(BorderInfo());
		_borders_for_check.back().index_offset = offset + IPoint(dir.y, -dir.x);
		_borders_for_check.back().dir = (dir_id + 1) % 8;
	}

	_countSellForWaitEnergy = 0;
	std::list<Game::Square*> squares_for_wait;

	std::vector<IPoint> vec, vec_next;
	std::vector<IPoint> *vec_ptr(&vec), *vec_next_ptr(&vec_next);

	Gadgets::square_new_info.EnergySource_Get(vec);
	while(!vec_ptr->empty())
	{
		for(auto i : *vec_ptr)
		{
			IPoint index = i;
			if(!Game::isVisible(Game::FieldAddress(index)))
			{
				continue;
			}
			if(isBorder(index))
			{
				continue;
			}
			if(Energy::field.AddSnapListenerOnEnergyFill(Game::FieldAddress(index), this))
			{
				_countSellForWaitEnergy++;
				_lightSquares.push_back(Game::FieldAddress(index));
				for(int k = 0; k < 8; k++)
				{
					vec_next_ptr->push_back(i + BorderInfo::dirs[k]);
				}
			}
		}
		std::swap(vec_next_ptr, vec_ptr);
		vec_next_ptr->clear();
	}
	//Выбираем самый дальний контроллер, он и будет содержать в себе все клетки уровня которые надо активировать энергией
	if(SnapGadgetReleaseBorderElement::LEVEL_SQUARES_COUNT < _countSellForWaitEnergy)
	{
		SnapGadgetReleaseBorderElement::LEVEL_SQUARES_COUNT = _countSellForWaitEnergy;
		more_far_release_border_element = this;
	}
	_borders_for_check.clear();
	_alphaTimer = 0.f;
}

bool SnapGadgetReleaseBorderElement::isBorder(IPoint index)
{
	bool find = false;
	for(auto border : _borders_for_check)
	{
		if(index == cell_index + border.index_offset + BorderInfo::dirs[border.dir])
		{
			find = true;
			break;
		}
	}
	if(find)
	{
		for(auto border : _borders_for_check)
		{
			if(index == cell_index + border.index_offset && (border.dir % 2) == 0)
			{
				//Хак. Если клетка явно содержит границу, то она не может попасть под определение границы.
				return false;
			}
		}	
		return true;
	}
	return false;
}

void SnapGadgetReleaseBorderElement::OnEnergySquareFilled(Game::FieldAddress index)
{
	if(this == more_far_release_border_element)
	{
		//Общее кол-во заполненных клеток считается только для самого дальнего элемента
		SnapGadgetReleaseBorderElement::RELEASED_SQUARES++;
	}
	_lightSquares.remove(index);
	_countSellForWaitEnergy--;
	if(_countSellForWaitEnergy < 0)
	{
		_countSellForWaitEnergy = 0;
		Assert(false);
	}
}

//bool SnapGadgetReleaseBorderElement::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
//{
//	return false;
//}

bool SnapGadgetReleaseBorderElement::Editor_MouseUp(const IPoint &mouse_pos)
{
	for(auto i = _borders.begin(); i != _borders.end(); i++)
	{
		if((cell_index + i->index_offset) == GameSettings::underMouseIndex)
		{
			Byte check_dir = GetDir(cell_index + i->index_offset, mouse_pos);
			if(check_dir == i->dir)
			{
				_borders.erase(i);
				return true;
			}
		}
	}
	_borders.push_back(BorderInfo());
	BorderInfo &info = 	_borders.back();
	info.index_offset = GameSettings::underMouseIndex - cell_index;
	_borders.back().dir = GetDir(GameSettings::underMouseIndex, mouse_pos);
	return false;
}


//template<struct T>
//bool CheckType<T>(SnapGadgetBaseElement* ptr)
//{
//	return dynamic_cast<T*>(ptr) != 0;
//}


bool SnapGadgetCaptureElement::Is(SnapGadgetBaseElement *item)
{
	return dynamic_cast<SnapGadgetCaptureElement*>(item) != 0;
}

bool SnapGadgetReleaseElement::Is(SnapGadgetBaseElement *item)
{
	return dynamic_cast<SnapGadgetReleaseElement*>(item) != 0;
}

bool SnapGadgetReleaseBorderElement::Is(SnapGadgetBaseElement *item)
{
	return dynamic_cast<SnapGadgetReleaseBorderElement*>(item) != 0;
}


/* --- SnapGadget class functions --- */

int SnapGadget::CHECK_FOR_INTRO = 0;

SnapGadget::SnapGadget()
	:_check_for_intro(0)
{
	_gamefield = NULL;
	_state_link = GADGET_STATE_NORMAL;
	_snapPoint = IPoint(-1, -1);
	_scrollSpeed = SNAP_SCROLL_SPEED;
}

SnapGadget::SnapGadget(SnapGadget &other)
{
	_gamefield = NULL;
	_state_link = GADGET_STATE_NORMAL;
	_snapPoint = other._snapPoint;
	_scrollSpeed = other._scrollSpeed;
	_snapPointStart = other._snapPointStart;
	_scrollTimeScale = other._scrollTimeScale;
	_scrollTime = other._scrollTime;

	for(size_t i = 0; i < other._elements.size(); i++)
	{
		SnapGadgetBaseElement::SHARED_PTR e_new = SnapGadgetBaseElement::Clone(other._elements[i]);
		_elements.push_back(e_new);
	}
	Init(GameField::Get());
}


SnapGadget::~SnapGadget()
{

}

bool SnapGadget::Validate()
{
	if ((_snapPoint.x == -1) || (_snapPoint.y == -1)){
		return false;
	}
	if (_elements.empty()){
		return false;
	}

	//По просьбам трудящихся снапы без отпускющих щупалец имеют место быть!
	bool have_capture = false;
	bool on_squares = false;
	size_t count = _elements.size();
	for (size_t i = 0; i < count; i++)
	{
		SnapGadgetBaseElement* element = _elements[i].get();
		if(SnapGadgetCaptureElement::Is(element))
		{
			have_capture = true;
		}
		if(element->IsStayOnVisibleSquare())
		{
			on_squares = true;
		}
	}
	return have_capture && on_squares;
}

size_t SnapGadget::AddElement(SnapGadgetBaseElement::SHARED_PTR element)
{
    _elements.push_back(element);
	return _elements.size()-1;
}

void SnapGadget::RemoveElement(int index)
{
    _elements.erase(_elements.begin() + index);
}

IPoint SnapGadget::GetAddressSnap()
{
	return IPoint(_snapPoint.x/GameSettings::SQUARE_SIDE, _snapPoint.y/GameSettings::SQUARE_SIDE);
}

void SnapGadget::SetSnapPoint(const IPoint& point)
{
	//_snapPoint.x = point.x - (point.x % (GameSettings::SQUARE_SIDE/2));
	//_snapPoint.y = point.y - (point.y % (GameSettings::SQUARE_SIDE/2));
	_snapPoint.x = math::round(point.x/(GameSettings::SQUARE_SIDEF/2.f))*GameSettings::SQUARE_SIDE/2.f;
	_snapPoint.y = math::round(point.y/(GameSettings::SQUARE_SIDEF/2.f))*GameSettings::SQUARE_SIDE/2.f;
}

void SnapGadget::DrawGame()
{
	for(auto i : _elements)
	{
		SnapGadgetReleaseBorderElement *el = dynamic_cast<SnapGadgetReleaseBorderElement*>(i.get());
		if(el)
		{
			el->DrawGame();
		}
	}		
}

void SnapGadget::Update(float dt)
{
	for(auto i : _elements)
	{
		i->Update(dt);
	}
}

void SnapGadget::Draw(int drawState)
{
	Render::device.SetTexturing(false);

	if (drawState == 1)	// Отображаютcя, но не редактируютcя
	{
		Render::BeginAlphaMul(63.f/255.f);
	}

	out_count = 0;
	in_count = 0;

	// Связующие линии...
	size_t count = _elements.size();
	for (size_t i = 0; i < count; i++)
	{
		_elements[i]->Edit_Draw(_snapPoint);
	}

	{
		// Точка центрирования (оранжевая)
		Render::device.SetTexturing(false);
		IRect draw (_snapPoint.x, _snapPoint.y, 0, 0);
		IRect draw_screen(draw);
		draw.Inflate(20);

		Render::BeginColor(centerColor);

		Render::DrawRect(draw);
		Render::DrawFrame(draw);

		//draw = IRect(draw.x + 1, draw.y + 1, 0, 0);
		//draw.Inflate(2);

		//Render::DrawRect(draw);
		//Render::DrawFrame(draw);

		draw_screen = IRect(_snapPoint.x - GameSettings::COLUMNS_COUNT*GameSettings::SQUARE_SIDE/2, _snapPoint.y - GameSettings::ROWS_COUNT*GameSettings::SQUARE_SIDE/2, GameSettings::COLUMNS_COUNT*GameSettings::SQUARE_SIDE, GameSettings::ROWS_COUNT*GameSettings::SQUARE_SIDE);
		
		Render::DrawFrame(draw_screen);
		Render::DrawFrame(draw_screen.Inflated(1));

		Render::EndColor();
	}

	{
		Render::device.SetTexturing(true);
		Render::BeginColor(textColor);

		Render::FreeType::BindFont("debug");
		Render::PrintString(_snapPoint.x - 18, _snapPoint.y + 10, utils::lexical_cast(in_count), 1.0f, LeftAlign, BaseLineAlign);
		Render::PrintString(_snapPoint.x - 18, _snapPoint.y - 2, utils::lexical_cast(out_count), 1.0f, LeftAlign, BaseLineAlign);
	
		if (_scrollSpeed == SNAP_SCROLL_SPEED)
		{
			Render::PrintString(_snapPoint.x - 18, _snapPoint.y - 14, "slow", 1.0f, LeftAlign, BaseLineAlign);
		}
		else if (_scrollSpeed == SNAP_SCROLL_SPEED_FAST)
		{
			Render::PrintString(_snapPoint.x - 18, _snapPoint.y - 14, "fast", 1.0f, LeftAlign, BaseLineAlign);
		}
		else if (_scrollSpeed == SNAP_SCROLL_SPEED_FASTEST)
		{
			Render::PrintString(_snapPoint.x - 18, _snapPoint.y - 14, "fastest", 1.0f, LeftAlign, BaseLineAlign);
		}else{
			Render::PrintString(_snapPoint.x - 18, _snapPoint.y - 14, utils::lexical_cast(_scrollSpeed), 1.0f, LeftAlign, BaseLineAlign);
		}

		Render::EndColor();
	}
	if (drawState == 1)	// Отображаютcя, но не редактируютcя
	{
		Render::EndAlphaMul();
	}
}

void SnapGadget::Init(GameField *field)
{
	_gamefield = field;
	_countReleaseElements = 0;
	for(size_t i = 0; i < _elements.size();i++)
	{
		if(SnapGadgetReleaseBorderElement::Is(_elements[i].get()) || SnapGadgetReleaseElement::Is(_elements[i].get()))
		{
			_countReleaseElements++; //Подсчет валиден только если гаджет перезагружен из файла уровня для игры (обеспечивается автоматически)
		}
	}
}

size_t SnapGadget::GetCountReleaseElement()
{
	return _countReleaseElements;
}

bool SnapGadget::IsCheckCameraForRelease(const bool future) const
{
	bool lastReceiver = (Gadgets::receivers.TotalCount() - Gadgets::receivers.ActiveCount() == 1);

	bool all_release_element_activated = true;
	bool any_release_element_activated = false;
	bool or_elements_exist = false;
	bool release_elements_exist = false;

	size_t count = _elements.size();
	for (size_t i = 0; i < count; i++)
	{
		if(!_elements[i].get()->CheckElementsOnRelease(lastReceiver, future, or_elements_exist, release_elements_exist, any_release_element_activated, all_release_element_activated))
		{
			return false;
		}
	}
	if(!release_elements_exist)
	{
		//Если релизных элементов нет, то камера держится тут до конца уровня.			
		all_release_element_activated = false;
	}
	if( (any_release_element_activated || !or_elements_exist) && all_release_element_activated)
	{
		return true;
	}
	return false;
}


SnapGadgetState SnapGadget::UpdateState(float &distance_max) const
{
	SnapGadgetState new_state = _state_link;

	if(new_state == GADGET_STATE_PASSED || new_state == GADGET_STATE_REMOVE)
	{
		return new_state;
	}
	size_t count = _elements.size();
	if(new_state == GADGET_STATE_NORMAL)
	{
		//Готов к захвату
		//Проходимся по всем без прерывания и ищем самую близкую к фишишу точку привязки
		for (size_t i = 0; i < count; i++)
		{
			SnapGadgetCaptureElement* capture_element = dynamic_cast<SnapGadgetCaptureElement*>(_elements[i].get());
			if(capture_element)
			{
				int x = capture_element->GetIndex().x;
				int y = capture_element->GetIndex().y;
				Game::Square *sq = GameSettings::gamefield[x + 1][y + 1];
				if (Game::isBuffer(sq)){
					continue;
				}
				if(sq->IsEnergyChecked(false) || Gadgets::square_new_info.IsEnergySourceSquare(sq->address.ToPoint()))
				{
					float dist = Gadgets::squareDist[capture_element->GetIndex()]; //Расстояние до конца уровня (условное)
					//Берем ближайшую (на совести дизайнеров уровня)
					if(dist < 0)
					{
						//Assert(false); //Точка находится не на поле	
					}else if(dist > distance_max)
					{
						new_state = GADGET_STATE_ACTIVE;
						distance_max = dist;
						break;
					}
				}
			}
		}
		return new_state;
	}
	if(new_state == GADGET_STATE_ACTIVE)
	{
		//Захвачен готов к отпусканию
		if(IsCheckCameraForRelease(false))
		{
			new_state = GADGET_STATE_PASSED;
		}
	}
	return new_state;
}

void SnapGadget::StartSnap(const FPoint &fieldPos)
{
	_snapPointStart = fieldPos;
	_scrollPoint = fieldPos;
	_scrollTime = 0.0f;

	float dx = float (_snapPoint.x - GameSettings::FIELD_SCREEN_CENTER.x) - _snapPointStart.x;
	float dy = float (_snapPoint.y - GameSettings::FIELD_SCREEN_CENTER.y) - _snapPointStart.y;

	float d = sqrtf(dx * dx + dy * dy);
	
	if (d > 0.0f)
		if( IsFix()){
			_scrollTimeScale = (_gamefield->_energyTimeScale * _gamefield->_energyDistanceTimeScale * _scrollSpeed) / d;
		}else{
			_scrollTimeScale = _scrollSpeed / d;
		}
	else
		_scrollTimeScale = 1.0f;
}

bool SnapGadget::IsFix() const
{
	return (_scrollSpeed == SNAP_SCROLL_SPEED) || (_scrollSpeed == SNAP_SCROLL_SPEED_FAST) || (_scrollSpeed == SNAP_SCROLL_SPEED_FASTEST);
}


void SnapGadget::StopSnap()
{
}

bool SnapGadget::CheckTargetingFirst(IPoint &to)
{
	std::vector<IPoint> vec;
	Gadgets::square_new_info.EnergySource_Get(vec);
	for(size_t i = 0; i < vec.size(); i++)
	{
		size_t count = _elements.size();
		for(size_t k = 0; k < count; k++)
		{
			SnapGadgetCaptureElement* capture_element = dynamic_cast<SnapGadgetCaptureElement*>(_elements[k].get());
			if(capture_element)
			{
				if(capture_element->GetIndex() == vec[i])
				{
					to.x = _snapPoint.x;			
					to.y = _snapPoint.y;
					return true;
				}
			}
		}
	}
	if(!vec.empty())
	{
		IPoint half_screen(GameSettings::FIELD_SCREEN_CENTER);
		to = vec.front()*GameSettings::SQUARE_SIDE - half_screen;
	}
	return false;
}

void SnapGadget::UpdateScrolling(float dt)
{
	_scrollPoint.x = _snapPoint.x - GameSettings::FIELD_SCREEN_CENTER.x;
	_scrollPoint.y = _snapPoint.y - GameSettings::FIELD_SCREEN_CENTER.y;
	_scrollTime = 1.f;
}

void SnapGadget::SaveLevel(Xml::TiXmlElement *root)
{
	// по алфавиту!
	root -> SetAttribute("scrollSpeed", utils::lexical_cast(_scrollSpeed));

	root -> SetAttribute("change_point_to_index", "true");

	FPoint for_save = _snapPoint; //Сохраняем индекс а не абсолютное значение (для масштабируемости графики)
	for_save.x = _snapPoint.x/GameSettings::SQUARE_SIDEF;
	for_save.y = _snapPoint.y/GameSettings::SQUARE_SIDEF;

	root -> SetAttribute("x", utils::lexical_cast(for_save.x));
	root -> SetAttribute("y", utils::lexical_cast(for_save.y));


	for (size_t i = 0; i < _elements.size(); i++)
	{
		_elements[i]->SaveLevel(root);
	}
}

void SnapGadget::Editor_MoveGadget(const IPoint& mouse_pos, int x, int y)
{
}

int SnapGadget::Editor_CaptureGadget(const IPoint& mouse_pos)
{
	// Преобразовываем координаты к координатам на поле
	IPoint mouse;
	mouse.x = mouse_pos.x + GameSettings::FieldCoordMouse().x;
	mouse.y = mouse_pos.y + GameSettings::FieldCoordMouse().y;

	if(FPoint(_snapPoint).GetDistanceTo(mouse) < 30.f)
	{
		return (-2);
	}

	size_t count = _elements.size();
	for (size_t i = 0; i < count; i++)
	{
		if(_elements[i]->IsUnderMouse(mouse))
		{
			return int(i);
		}
	}
	return -1;
}

void SnapGadget::Editor_ReleaseGadget(const IPoint& mouse_pos, int x, int y)
{
}

void SnapGadget::Editor_MoveElements(const IRect& part, const IPoint& delta)
{
	IRect r (part);

	size_t count = _elements.size();
	for (size_t i = 0; i < count; i++)
	{
		_elements[i].get()->Editor_MoveElements(part, delta);
	}

	r.x *= GameSettings::SQUARE_SIDE;
	r.y *= GameSettings::SQUARE_SIDE;
	r.width *= GameSettings::SQUARE_SIDE;
	r.height *= GameSettings::SQUARE_SIDE;

	if (r.Contains(_snapPoint))
	{
		_snapPoint.x += delta.x * GameSettings::SQUARE_SIDE;
		_snapPoint.y += delta.y * GameSettings::SQUARE_SIDE;
	}
}

void SnapGadget::SetCheckedForIntro()
{
	_check_for_intro = SnapGadget::CHECK_FOR_INTRO;
}

bool SnapGadget::CheckedForIntro()
{
	return (_check_for_intro == SnapGadget::CHECK_FOR_INTRO);
}

void SnapGadget::GetCaptureElements(std::vector<IPoint> &vec)
{
	for(size_t i = 0; i < _elements.size(); i++)
	{
		SnapGadgetCaptureElement* capture_element = dynamic_cast<SnapGadgetCaptureElement*>(_elements[i].get());
		if(capture_element)
		{
			vec.push_back(capture_element->GetIndex());
		}
	}
}

void SnapGadget::GetReleaseElements(std::vector<IPoint> &vec)
{
	for(size_t i = 0; i < _elements.size(); i++)
	{
		SnapGadgetBaseElement* el = _elements[i].get();
		if(SnapGadgetReleaseElement::Is(el) || SnapGadgetReleaseBorderElement::Is(el))
		{
			vec.push_back(el->GetIndex());
		}
	}
}

void SnapGadget::FindSnap(const IPoint index, std::vector<SnapGadgetBaseElement::SHARED_PTR> &elements_result)
{
	size_t count = _elements.size();
	for(size_t i = 0; i < count; i++)
	{
		if(_elements[i]->GetIndex() == index)
		{
			elements_result.push_back(_elements[i]);
		}
	}
}

bool SnapGadget::Editor_MouseWheel(int delta)
{
	if(EditorUtils::activeEditBtn !=  EditorUtils::SnapGadgetAdd)
	{
		return false;	
	}
	if(Core::mainInput.IsShiftKeyDown()){
		_scrollSpeed += delta*10.f;
		if(_scrollSpeed < 0.f)
		{
			_scrollSpeed = 0.f;
		}
	}else if(Core::mainInput.IsControlKeyDown()){
		//Как раньше
		if (_scrollSpeed == SNAP_SCROLL_SPEED)
		{
			_scrollSpeed = SNAP_SCROLL_SPEED_FAST;
		}
		else if (_scrollSpeed == SNAP_SCROLL_SPEED_FAST)
		{
			_scrollSpeed = SNAP_SCROLL_SPEED_FASTEST;
		}
		else
		{
			_scrollSpeed = SNAP_SCROLL_SPEED;
		}
	}
	return true;
}

bool SnapGadget::Editor_MouseDown(const IPoint &mouse_pos)
{
	for(auto i : _elements)
	{
		SnapGadgetReleaseBorderElement *el = dynamic_cast<SnapGadgetReleaseBorderElement*>(i.get());
		if(el)
		{
			if(el->Editor_MouseDown(mouse_pos))
			{
				return true;
			}
		}
	}
	return false;
}

bool SnapGadget::Editor_MouseUp(const IPoint &mouse_pos)
{
	for(auto i : _elements)
	{
		SnapGadgetReleaseBorderElement *el = dynamic_cast<SnapGadgetReleaseBorderElement*>(i.get());
		if(el)
		{
			if(el->Editor_MouseUp(mouse_pos))
			{
				return true;
			}
		}
	}
	return false;
}

void SnapGadget::PrepareLevel()
{
	for(auto i : _elements)
	{
		SnapGadgetReleaseBorderElement *el = dynamic_cast<SnapGadgetReleaseBorderElement*>(i.get());
		if(el)
		{
			el->PrepareLevel();
		}
	}	
}

