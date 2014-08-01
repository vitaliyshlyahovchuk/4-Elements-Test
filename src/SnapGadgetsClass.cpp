#include "stdafx.h"

#include "SnapGadgetsClass.h"
#include "GameField.h"
#include "Game.h"
#include "SquareNewInfo.h"
#include "EditorUtils.h"
#include "EnergyReceivers.h"
#include "RoomGates.h"
#include "Match3Gadgets.h"

namespace Gadgets
{
	SnapGadgets snapGadgets;
}

/* --- SnapGadgets container functions --- */

SnapGadgets::SnapGadgets()
{
	_gamefield = NULL;
	_loaded = false;
	_isRightMouse = false;
}

void SnapGadgets::Release()
{
	Clear();
}

SnapGadgets::~SnapGadgets()
{
	Clear();
}

void SnapGadgets::Init(GameField *field)
{
	_gamefield = field;
	_activeGameGadget = NULL;
	
	_dragGadget = NULL;
	_dragElementId = -1;
	_currentEditorGadget = NULL;
	_editTimer = 0.f;
}

void SnapGadgets::Clear()
{
	_loaded = false;
	_max_distance = 4000000.f;
	_drag_gadget = false;
	_activeGameGadget = NULL;

	_currentEditorGadget = NULL;

	_dragGadget = NULL;
	_dragElementId = -1;

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		delete _gadgets[i];
		_gadgets[i] = NULL;
	}

	_gadgets.clear();
}

void SnapGadgets::Reset()
{
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		SnapGadget *gadget = _gadgets[i];
		gadget -> SetState(GADGET_STATE_NORMAL);
	}

	_activeGameGadget = NULL;
	_currentEditorGadget = NULL;

	_dragGadget = NULL;
	_dragElementId = -1;
}

void SnapGadgets::DrawGame()
{
	if(_activeGameGadget)
	{
		_activeGameGadget->DrawGame();
	}
}

void SnapGadgets::DrawEdit()
{
	int drawState = 0;	// Не риcуем cовcем
	if(!EditorUtils::editor)
	{
		if(EditorUtils::draw_debug){
			drawState = 1;
		}else{
			return;
		}	
	}

	if (EditorUtils::gadgetsVisible) 
	{
		drawState = 1;	// Риcуем неактивными, cерыми
	}

	if (EditorUtils::activeEditBtn == EditorUtils::SnapGadgetAdd) 
	{
		drawState = 2;	// Риcуем активными, цветными
	}

	if(drawState == 0)
	{
		return;
	}
	Render::BeginColor(_currentEditorGadget ? Color(10,10,10,100) : Color::WHITE);
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		SnapGadget *gadget = _gadgets[i];
		if(gadget == _currentEditorGadget)
		{
			continue;
		}
		gadget -> Draw(drawState);
	}
	Render::EndColor();

	if(_currentEditorGadget){
		Render::BeginAlphaMul(math::clamp(0.f, 1.f, (0.5f + 0.5f*(1.f + sinf(_editTimer*6.f))*0.5f)));
		_currentEditorGadget->Draw(drawState);
		Render::EndAlphaMul();
	}
}

void SnapGadgets::Update(float dt)
{
	if (_activeGameGadget) 
	{
		_activeGameGadget->UpdateScrolling(dt);
		_activeGameGadget->Update(dt);
	}
	UpdateActive();
	if(_currentEditorGadget){
		_editTimer += dt;
	}
}

void SnapGadgets::SetActivePassed()
{
	if (_activeGameGadget)
	{
		_activeGameGadget -> SetState(GADGET_STATE_REMOVE);
		_activeGameGadget -> StopSnap();
		_activeGameGadget = NULL;
	}
}

void SnapGadgets::UpdateActive()
{
	bool can_activate_next = (_activeGameGadget == NULL);
	if(_activeGameGadget && CountActiveGadgets() > 1) // если это последний чекпоинт, то ничего не трогаем и никуда не двигаемся больше
	{		
		float t(0.f);
		SnapGadgetState state =_activeGameGadget->UpdateState(t);
		if (state == GADGET_STATE_PASSED)
		{
			_activeGameGadget->SetState(GADGET_STATE_PASSED);
			// Cбраcываем привязку...
			SetActivePassed();
			can_activate_next = true;
		}
		else if(_activeGameGadget->GetCountReleaseElement() == 0)
		{
			can_activate_next = true;
		}
	}
	if(can_activate_next)
	{ 
		//Не проверяем новый пока не пройдем предыдущий
		int max_near = -1;
		float distance_max_to_end = -1.f;
		size_t count = _gadgets.size();
		for (size_t i = 0; i < count; i++)
		{
			SnapGadget *gadget = _gadgets[i];
			SnapGadgetState state = gadget -> GetState();
			if (_activeGameGadget == gadget || state == GADGET_STATE_REMOVE){
				continue;
			}

			state = gadget -> UpdateState(distance_max_to_end);

			if (state == GADGET_STATE_ACTIVE)
			{
				//MyAssert(distance_next > 0 && distance_next < _max_distance);
				max_near = i;
			}
		}
		if( max_near != -1 )
		{		
			SnapGadget *gadget = _gadgets[max_near];
			SetActivePassed();
			gadget->SetState(GADGET_STATE_ACTIVE);
			// Задаём новую...
			_activeGameGadget = gadget;

			_max_distance = distance_max_to_end;

			_activeGameGadget -> StartSnap(_gamefield -> _destFieldPos);
		}
	}
}

bool SnapGadgets::CheckTargetingFirst(IPoint &to)
{
	size_t count = _gadgets.size();
	for(size_t i = 0; i < count; i++)
	{
		if(_gadgets[i]->CheckTargetingFirst(to))
		{
			return true;
		}
	}	
	return false;
}

//void SnapGadgets::UpdateFirstFast()
//{
//	UpdateActive();
//	if(_activeGameGadget)
//	{
//		_activeGameGadget->UpdateFirstFast();
//	}
//}

// Добавляем элемент...
void SnapGadgets::AddGadget(SnapGadget *gadget)
{
	gadget -> Init(_gamefield);
	_gadgets.push_back(gadget);
	_currentEditorGadget = gadget;
}

void SnapGadgets::SaveLevel(Xml::TiXmlElement *root)
{
	if(_gadgets.empty())
	{
		return;
	}
	Xml::TiXmlElement *gadgets = root -> InsertEndChild(Xml::TiXmlElement("Snap")) -> ToElement();

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		SnapGadget *g = _gadgets[i];
		if (!g -> Validate()) continue;

		Xml::TiXmlElement *gadget = gadgets -> InsertEndChild(Xml::TiXmlElement("Gadget")) -> ToElement();
		g -> SaveLevel(gadget);
	}
}

void SnapGadgets::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear(); // Удаляем, еcли еcть что-то...

	rapidxml::xml_node<> *gadgets = root->first_node("Snap");

	rapidxml::xml_node<> *gadget = gadgets ? gadgets->first_node("Gadget") : NULL;

	while (gadget)
	{
		SnapGadget *g = new SnapGadget();

		int x = Xml::GetFloatAttribute(gadget, "x")*GameSettings::SQUARE_SIDEF;
		int y = Xml::GetFloatAttribute(gadget, "y")*GameSettings::SQUARE_SIDEF;

		if(!Xml::GetBoolAttributeOrDef(gadget, "change_point_to_index", false))
		{
			x = int(x/80.f);
			y = int(y/80.f);
		}

		float scrollSpeed = SNAP_SCROLL_SPEED;

		scrollSpeed = Xml::GetFloatAttribute(gadget, "scrollSpeed");
		bool activateType_default = Xml::GetBoolAttributeOrDef(gadget, "activate_relese_type", false);

		g -> SetSnapPoint(IPoint(x, y));
		g -> SetScrollSpeed(scrollSpeed);

		rapidxml::xml_node<> *element = gadget -> first_node("Item");
		int elementsCount = 0;

		while (element)
		{
			SnapGadgetBaseElement::SHARED_PTR e(0);
			
			int type_int = Xml::GetIntAttribute(element, "type");
			if(type_int == 0)
			{
				e = SnapGadgetCaptureElement::SHARED_PTR(new SnapGadgetCaptureElement());
			}else if(type_int == 1)
			{
				e = SnapGadgetReleaseElement::SHARED_PTR(new SnapGadgetReleaseElement());
			}else if(type_int == 2)
			{
				e = SnapGadgetReleaseBorderElement::SHARED_PTR(new SnapGadgetReleaseBorderElement());
			}else{
				Assert(false);
			}			
			e->LoadLevel(element);
		
			g -> AddElement(e);

			element = element -> next_sibling("Item");
			elementsCount += 1;
		}

		if (!elementsCount)
		{
			delete g;
		}
		else 
		{
			AddGadget(g);
		}
		gadget = gadget -> next_sibling("Gadget");
	}
}

void SnapGadgets::CheckAfterLoadReceivers()
{
	if( _gadgets.empty() && !EditorUtils::editor)
	{
		// если ни одной точки привязки нет, создадим одну сами, установленную в центр уровня
		IPoint min, max;
		EditorUtils::GetFieldMinMax(min, max);
		IPoint center( (max.x + min.x + 1) * GameSettings::SQUARE_SIDE / 2, (max.y + min.y + 1) * GameSettings::SQUARE_SIDE / 2);

		if( max.x > min.x && max.y > min.y ) // иначе поле пустое и нечего туда что-то ставить
		{
			SnapGadget *g = new SnapGadget();

			g->SetSnapPoint(center);

			// точки на источниках энергии
			std::vector<IPoint> vec;
			Gadgets::square_new_info.EnergySource_Get(vec);
			if(!vec.empty())
			{
				for(size_t i = 0; i < vec.size(); ++i)
				{
					SnapGadgetCaptureElement::SHARED_PTR e(new SnapGadgetCaptureElement());
					e->SetIndex(vec[i]);
					e->activateType = ACTIVATE_TYPE_OR;
					g->AddElement(e);
				}
			}else{
				Assert(Gadgets::levelSettings.getString("LevelObjective", "energy") != "receivers");
			}

			// точки в приемниках энергии
			Gadgets::receivers.GetReceiversPositions(vec);
			if(!vec.empty())
			{
				for(size_t i = 0; i < vec.size(); ++i)
				{
					SnapGadgetReleaseElement::SHARED_PTR e(new SnapGadgetReleaseElement());
					e->SetIndex(vec[i]);
					e->activateType = ACTIVATE_TYPE_AND;
					g->AddElement(e);
				}
			}
			AddGadget(g);
		}
	}
	_loaded = true;
}

bool SnapGadgets::Editor_MouseMove(const IPoint& mouse_pos, Game::Square *sq)
{
	if(EditorUtils::activeEditBtn !=  EditorUtils::SnapGadgetAdd)
	{
		return false;	
	}
	int x = sq->address.GetCol();
	int y = sq->address.GetRow();

	if(_drag_gadget)
	{
		IPoint to_pos = IPoint(x,y);
		_currentEditorGadget->Editor_MoveElements(IRect(0,0,200,200), to_pos - _fromPos);
		_fromPos = to_pos;
		return true;
	}
	if (!_dragGadget)
	{
		return true;
	}
	if (_dragElementId == -1)
	{
		return true;
	}

	if (_dragElementId == -2)
	{
		IPoint p;

		p.x = mouse_pos.x + GameSettings::FieldCoordMouse().x;
		p.y = mouse_pos.y + GameSettings::FieldCoordMouse().y;

		//Округление к ПОЛОВИНЕ ПОЛОВИНЫ КЛЕТКИ чтобы небыло стартового смещения)
		_dragGadget -> SetSnapPoint(p + (GameSettings::CELL_HALF*0.5f).Rounded());
	}
	else
	{
		if(!Game::isVisible(sq))
		{
			return true;
		}

		SnapGadgetBaseElement::SHARED_PTR e = _dragGadget -> GetElement(_dragElementId);
		e->SetIndex(IPoint(x, y));

	}
	return true;
}

bool SnapGadgets::Editor_MouseWheel(int delta, Game::Square *sq)
{
	if(EditorUtils::activeEditBtn !=  EditorUtils::SnapGadgetAdd)
	{
		return false;	
	}

	if(_currentEditorGadget){
		_currentEditorGadget->Editor_MouseWheel(delta);
	}
	return true;
}

bool SnapGadgets::Editor_ProcessClick(int &captured, SnapGadget *g)
{
	// еcли кликнули по центральному элементу c нажатым VK_CONTROL
	if (captured == -2)
	{
		bool clone =  Core::mainInput.IsControlKeyDown();
		if (clone)
		{
			SnapGadget* new_gadget = new SnapGadget(*g);
			_currentEditorGadget = new_gadget;
			_gadgets.push_back(new_gadget);
			_dragGadget = NULL;
			_dragElementId = -1;
			captured = -1;
			_fromPos = GameSettings::GetMouseAddress(Core::mainInput.GetMousePos()).ToPoint();
			_drag_gadget = true;
			return true;
		}else{
			_currentEditorGadget = g;					
		}
	}else if(captured != -2)
	{
		// Еcли нажат VK_CONTROL и кликнули на элемент входа или
		// элемент выхода, то тогда клонируем его, а cтарый двигаем
		_currentEditorGadget = g;
		SnapGadgetBaseElement::SHARED_PTR e = g -> GetElement(captured);
		if(Core::mainInput.IsShiftKeyDown())
		{
			e->activateType = !e->activateType;
			//Не тащим элемент
			_dragGadget = NULL;
			_dragElementId = -1;
			captured = -1;
			return true;
		}else if (Core::mainInput.IsControlKeyDown() && !SnapGadgetReleaseBorderElement::Is(e.get()))
		{
			SnapGadgetBaseElement::SHARED_PTR e_new = SnapGadgetBaseElement::Clone(e);
			g -> AddElement(e_new);
		}
	}
	_dragGadget = g;
	_dragElementId = captured;
	return false;
}

bool SnapGadgets::Editor_CaptureGadget(const IPoint& mouse_pos)
{
	_drag_gadget = false;
	int captured = -1;

	if(_currentEditorGadget)
	{
		captured = _currentEditorGadget->Editor_CaptureGadget(mouse_pos);
		if (captured != -1) 
		{
			Editor_ProcessClick(captured, _currentEditorGadget);
			return true;
		}
		if(!Core::mainInput.IsShiftKeyDown())
		{
			_currentEditorGadget = 0;
		}
	}

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		SnapGadget *g = _gadgets[i];
		captured = g -> Editor_CaptureGadget(mouse_pos);

		if (captured != -1) 
		{
			Editor_ProcessClick(captured, g);
			return true;
		}
	}
	return false;
}

bool SnapGadgets::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
{
	if(EditorUtils::activeEditBtn !=  EditorUtils::SnapGadgetAdd)
	{
		return false;	
	}
	if(Core::mainInput.IsShiftKeyDown())
	{
		if(_currentEditorGadget)
		{
			if(_currentEditorGadget->Editor_MouseUp(GameSettings::ToFieldPos(mouse_pos)))
			{
				return true;
			}
		}
	}
	 // Не попали по cущеcтвующим элементам. Ставить новый элемент можно только на видимую часть поля.
	if(!_currentEditorGadget && !_isRightMouse && FPoint(_mouseDownPos - mouse_pos).GetDistanceToOrigin() < 10.f && Game::isVisible(sq->address))
	{
		SnapGadget *g = new SnapGadget();
		{
			// Добавляем элемент конца...
			SnapGadgetReleaseElement::SHARED_PTR el = SnapGadgetReleaseElement::SHARED_PTR(new SnapGadgetReleaseElement());
			el->SetIndex(sq->address.ToPoint());
			el->activateType = ACTIVATE_TYPE_AND;
			g -> AddElement(el);
		}
		{
			// Добавляем элемент начала...
			SnapGadgetCaptureElement::SHARED_PTR el = SnapGadgetCaptureElement::SHARED_PTR(new SnapGadgetCaptureElement());
			el->SetIndex(sq->address.ToPoint());
			el->activateType = ACTIVATE_TYPE_AND;
			g -> AddElement(el);
		}

		// Преобразовываем координаты к координатам на поле
		int mx = mouse_pos.x + GameSettings::FieldCoordMouse().x;
		int my = mouse_pos.y + GameSettings::FieldCoordMouse().y;

		// Уcтанавливаем точку центра
		g -> SetSnapPoint(IPoint(mx, my));
		AddGadget(g);
	}else{
		Editor_ReleaseGadget();
	}
	return false;
}

void SnapGadgets::Editor_ReleaseGadget()
{
	_dragGadget = NULL;
	_dragElementId = -1;
	_drag_gadget = false;
}

bool SnapGadgets::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::None || EditorUtils::activeEditBtn == EditorUtils::SnapGadgetAdd )
	{
		_currentEditorGadget = NULL;
		_isRightMouse = true;
		int captured = -1;
		int count = (int) _gadgets.size();
		for (int i = count - 1; i >= 0; i--)
		{
			SnapGadget *g = _gadgets[i];
			captured = g -> Editor_CaptureGadget(mouse_pos);
		
			if (captured != -1)	
			{
				// Еcли (-2), то кликнули на точку привязки 
				// и нужно удалить веcь гаджет целиком. Еcли
				// не (-2), то удаляем только элемент гаджета

				if (captured != -2)
				{
					g -> RemoveElement(captured);

					// Еcли пуcтой гаджет, то...
					if (!g -> GetElementsCount())
						_gadgets.erase(_gadgets.begin() + i);
				}
				else 
				{
					_gadgets.erase(_gadgets.begin() + i);
				}

				Editor_ReleaseGadget();
				break;
			}
		}

		return (captured != -1);
	}
	else
	{
		return false;
	}
}

void SnapGadgets::Editor_CutToClipboard(IRect part)
{
	_editor_moveRect = part;
}

bool SnapGadgets::Editor_PasteFromClipboard(IPoint pos) 
{ 
	Editor_MoveElements(_editor_moveRect, pos - _editor_moveRect.LeftBottom());
	return true;
}

void SnapGadgets::Editor_MoveElements(const IRect &part, const IPoint &delta)
{
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		SnapGadget *g = _gadgets[i];
		g -> Editor_MoveElements(part, delta);
	}	
}

bool SnapGadgets::AcceptMessage(const Message &message)
{
	if(EditorUtils::activeEditBtn != EditorUtils::SnapGadgetAdd)
	{
		return false;
	}
	if(message.is("KeyPress"))
	{
		int key = utils::lexical_cast<int>(message.getData());
		if(key == 'n')
		{
			if(_currentEditorGadget)
			{
				SnapGadgetReleaseBorderElement::SHARED_PTR e_new(new SnapGadgetReleaseBorderElement());
				e_new->SetIndex(GameSettings::underMouseIndex);
				_dragElementId = _currentEditorGadget->AddElement(e_new);
			}
		}else if(key == ' ')
		{
			if(_dragGadget && _dragElementId > -1)
			{
				SnapGadgetBaseElement::SHARED_PTR e = _dragGadget->GetElement(_dragElementId);
				if(SnapGadgetCaptureElement::Is(e.get()))
				{
					SnapGadgetCaptureElement::SHARED_PTR e_new = SnapGadgetCaptureElement::SHARED_PTR(new SnapGadgetCaptureElement());
					e_new->SetIndex(e->GetIndex());
					e_new->activateType = e->activateType;

					_dragGadget->RemoveElement(_dragElementId);
					_dragGadget->AddElement(e_new);
					e.reset();
				}else if(SnapGadgetReleaseElement::Is(e.get()))
				{
					SnapGadgetReleaseElement::SHARED_PTR e_new = SnapGadgetReleaseElement::SHARED_PTR(new SnapGadgetReleaseElement());
					e_new->SetIndex(e->GetIndex());
					e_new->activateType = e->activateType;

					_dragGadget->RemoveElement(_dragElementId);
					_dragGadget->AddElement(e_new);
					e.reset();
				}
			}else{
				_currentEditorGadget = NULL;
				size_t count = _gadgets.size();
				for(size_t i = 0; i < count;i++)
				{
					if(_gadgets[i]->Editor_CaptureGadget(Core::mainInput.GetMousePos()) != -1)
					{
						_currentEditorGadget = _gadgets[i];
						break;
					}
				}
				if(_currentEditorGadget){
					SnapGadgetReleaseElement::SHARED_PTR e_new = SnapGadgetReleaseElement::SHARED_PTR(new SnapGadgetReleaseElement());
					e_new->SetIndex(GameSettings::GetMouseAddress(Core::mainInput.GetMousePos()).ToPoint());
					size_t captured = _currentEditorGadget -> AddElement(e_new);
					_dragGadget = _currentEditorGadget;
					_dragElementId = captured;
				}
			}
			return true;
		}
	}
	return false;
}

SnapGadget* SnapGadgets::FindNearCaptureGadget(IPoint cell, float &finded_dist)
{
	if(_gadgets.empty())
	{
		Assert(false);
		return NULL;
	}
	//Поиск самой ближайшей точки захвата
	float value_to_rect_in_center = Gadgets::squareDist[cell.x][cell.y];
	if(value_to_rect_in_center < 0)
	{
		if(_gadgets.size() > 1)
		{
			EditorAlert("Точка старта/релиза snap стоит не на поле: col=" + Int::ToString(cell.x) + " row=" + Int::ToString(cell.y));
		}
		return _gadgets.front();
	}
	float min_dist = 400000.f;
	SnapGadget *near_snap = NULL;
	for(size_t i = 0; i < _gadgets.size(); i++)
	{
		if(_gadgets[i]->CheckedForIntro())
		{
			continue;
		}
		std::vector<IPoint> capture_els;
		_gadgets[i]->GetCaptureElements(capture_els);
		for(size_t k = 0; k < capture_els.size(); k++)
		{
			float value = Gadgets::squareDist[capture_els[k].x][capture_els[k].y];
			if(value < 0)
			{
				EditorAlert("Точка захвата snap стоит не на поле: col=" + Int::ToString(capture_els[k].x) + " row=" + Int::ToString(capture_els[k].y));
				//_gadgets[i]->SetCheckedForIntro();
				continue;
			}
			float dist = abs(value - value_to_rect_in_center);
			if(min_dist > dist)
			{
				min_dist = dist;
				near_snap = _gadgets[i];
			}
		}
	}
	finded_dist = min_dist;
	return near_snap;
}

bool SnapGadgets::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	_isRightMouse = false;
	_mouseDownPos = mouse_pos;
	if (EditorUtils::activeEditBtn != EditorUtils::SnapGadgetAdd)
	{
		return false;
	}
	bool captured = Editor_CaptureGadget(mouse_pos);
	return captured;
}


SnapGadget* SnapGadgets::FindNearCaptureGadget(SnapGadget *prev_snap)
{
	//Если отталкиваемся от гаджета, то тут все немного сложнее: нужно найти ближайший к точке отпускания
	SnapGadget* near_snap = NULL;
	std::vector<IPoint> release_els;
	prev_snap->GetReleaseElements(release_els);
	float min_dist = 400000.f;
	for(size_t i = 0; i < release_els.size(); i++)
	{
		float dist = 400000.f;
		SnapGadget* gadget = FindNearCaptureGadget(release_els[i], dist);
		if(gadget && min_dist > dist)
		{
			min_dist = dist;
			near_snap = gadget;
		}
	}
	return near_snap;
}

SnapGadget* SnapGadgets::FindFirstGadget()
{
	std::vector<IPoint> e_src;
	Gadgets::square_new_info.EnergySource_Get(e_src);

	for(size_t i = 0; i < _gadgets.size(); i++)
	{
		std::vector<IPoint> c_elem;
		_gadgets[i]->GetCaptureElements(c_elem);
		for(size_t j = 0; j < e_src.size(); ++j)
		{
			if( std::find(c_elem.begin(), c_elem.end(), e_src[j]) != c_elem.end() )
				return _gadgets[i];
		}
	}

	return NULL;
}

int SnapGadgets::CountActiveGadgets() const
{
	int count = _gadgets.size();
	for(size_t i = 0; i < _gadgets.size(); i++)
	{
		if(_gadgets[i]->GetState() == GADGET_STATE_REMOVE )
			--count;
	}
	return count;
}

bool SnapGadgets::IsLoaded() const
{
	return _loaded;
}

void SnapGadgets::FindSnap(const IPoint index, std::vector<SnapGadgetBaseElement::SHARED_PTR> &elements)
{
	elements.clear();
	for(SnapGadget *gadget : _gadgets)
	{
		gadget->FindSnap(index, elements);
	}
}

void SnapGadgets::PrepareLevel()
{
	SnapGadgetReleaseBorderElement::LEVEL_SQUARES_COUNT = 0;
	SnapGadgetReleaseBorderElement::RELEASED_SQUARES = 0;
	for(SnapGadget *gadget : _gadgets)
	{
		gadget->PrepareLevel();
	}
}

bool SnapGadgets::IsEnergyLevelFinished() const
{
	return SnapGadgetReleaseBorderElement::RELEASED_SQUARES == SnapGadgetReleaseBorderElement::LEVEL_SQUARES_COUNT;
}


