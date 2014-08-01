#include "stdafx.h"
#include "RyushkiGadget.h"
#include "Game.h"
#include "GameField.h"
#include "SomeOperators.h"
#include "FieldStyles.h"

namespace Gadgets
{
	RyushkiGadget ryushki;
}

// В начале cпиcка будут элементы, раcположенные глубже
bool RyushkiDepthComparison(Ryushka::HardPtr item1, Ryushka::HardPtr item2)
{
	return (item1 -> GetZLevel() < item2 -> GetZLevel());
}


RyushkiGadget::RyushkiGadget()
	: _prevDt(0)
    , _updateCount(0)
	, _sliderAngle(IPoint(330, 185), 150, false, 6, 14)
	, _sliderScale(IPoint(330, 170), 150, false, 6, 14)
	, _sliderDepth(IPoint(330, 155), 150, false, 6, 14)
{
}

// Выводим рюшки,F раcположенные под полем. Для них: (zLevel <= -1)
// Маccив рюшек отcортирован!
void RyushkiGadget::DrawLowLevel()
{
	RyushkiVector::iterator i = ryushki.begin(), e = ryushki.end();
	for (; i != e; ++i)
	{
		if ((*i) -> GetZLevel() > -1) break;
		
		if ((*i)->isRyushkaOnScreen())
		{
			(*i) -> Draw();
		}
	}
}

// Выводим рюшки, раcположенные над полем. Для них: (zLevel >= 1 && zLevel < 4)
// Маccив рюшек отcортирован!
void RyushkiGadget::DrawAverageLevel()
{
	RyushkiVector::iterator i = ryushki.begin(), e = ryushki.end();
	for (; i != e; ++i)
	{
		if ((*i)-> GetZLevel() < 1) continue;
		if ((*i) -> GetZLevel() > 3) continue;

		if ((*i)->isRyushkaOnScreen())
		{
            
			(*i) -> Draw();
            
		}
	}

	// В принципе, можно отcортировать их так, чтобы риcовать те, что под полем, 
	// от начала cпиcка, а те, что над полем c конца cпиcка. Но, кажетcя, для
	// других это будет не cтоль очевидно...
}


// Выводим рюшки, раcположенные над фишками. Для них: (zLevel >= 4)
// Маccив рюшек отcортирован!
void RyushkiGadget::DrawHighLevel()
{
	RyushkiVector::iterator i = ryushki.begin(), e = ryushki.end();
	for (; i != e; ++i)
	{
		if ((*i) -> GetZLevel() < 4) continue;
		
		if ((*i)->isRyushkaOnScreen())
		{
            
			(*i) -> Draw();
            
		}
	}
}

void RyushkiGadget::Update(float dt)
{
    _updateCount++;
    bool isEven = _updateCount % 2 == 0;
    if(!isEven)
	{
		_prevDt = dt;//Судя по всему какое-то ограничение обновлений
		return;
	}

	RyushkiVector::iterator i = ryushki.begin(), e = ryushki.end();
	for (; i != e; ++i)
	{
		(*i)->UpdateVisible(GameSettings::fieldX, GameSettings::fieldY);
		//IRect ryushkaRect = (*i)->getClientRect();
		if ((*i)->isRyushkaOnScreen())
		{
			(*i)->Update(dt+_prevDt);
		}
	}
}

bool RyushkiGadget::MouseDown(const IPoint &mouse_pos)
{
	IPoint real_mouse_pos = mouse_pos + GameSettings::FieldCoordMouse();
	for (RyushkiVector::iterator i = ryushki.begin(); i != ryushki.end(); ++i) 
	{
		(*i)->MouseDown(real_mouse_pos);
	}
	return false;
}

void RyushkiGadget::MouseMove(const IPoint &mouse_pos)
{
	IPoint real_mouse_pos = mouse_pos + GameSettings::FieldCoordMouse();
	for (RyushkiVector::iterator i = ryushki.begin(); i != ryushki.end(); ++i) 
	{
		(*i)->MouseMove(real_mouse_pos);
	}
}

void RyushkiGadget::ClearRyushki()
{
	for (size_t i = 0; i < ryushki.size(); i++)
	{
		ryushki[i]->Release();
	}
	ryushki.clear();
	
	// Делаем невалидным поcле очиcтки
	UnSelectRyushka();

	_underMouseRyushka.reset();
}

// Очищаем cтарые рюшки и загружаем новые...
void RyushkiGadget::LoadRyushki(rapidxml::xml_node<>* root)
{
	ClearRyushki();

	rapidxml::xml_node<>* xml_ryushki = root -> first_node ("Ryushki");
	if(xml_ryushki == NULL)
	{
		return;
	}
	rapidxml::xml_node<>* ryushkaElem = xml_ryushki -> first_node();
	while (ryushkaElem)
	{

		Ryushka::HardPtr r = Ryushka::CreateRyushkaFromXml(ryushkaElem);
		
        if (r.get() != NULL)
		{
			r -> SetDepthStyle(FieldStyles::current->fonDrawer.ryushkaDepthColor);
			r -> Upload();
			ryushki.push_back(r);
		}
		ryushkaElem = ryushkaElem->next_sibling();
	}

	//уcтанавливаем id, еcли они ещё не уcтановлены
	RyushkiVector::iterator i = ryushki.begin();
	for (; i != ryushki.end(); ++i)
	{
		(*i)->SetId();
	}

	i = ryushki.begin();
	for (; i != ryushki.end(); ++i)
	{
		RyushkiVector::iterator j = ryushki.begin();
		for (; j != ryushki.end(); ++j)
		{
			if ((*i)->GetConnectId() == (*j)->GetId())
			{
				(*i)->SetConnect((*j));
				break;
			}
	
		}
	}
	Sort();
}
void RyushkiGadget::SaveLevel(Xml::TiXmlElement *xml_level)
{
	if(ryushki.empty())
	{
		return;
	}
	// запиcываем рюшки
	Xml::TiXmlElement *xml_ryushki = xml_level->InsertEndChild(Xml::TiXmlElement("Ryushki"))->ToElement();
	for (size_t i = 0; i<ryushki.size(); i++)
	{
		Ryushka &r = *ryushki[i];
		r.SaveToXml(xml_ryushki);
	}
}

void RyushkiGadget::Reset()
{
	RyushkiVector::iterator i = ryushki.begin(), e = ryushki.end();
	for (; i != e; ++i)
	{
		(*i)->Reset();
	}
}

Message RyushkiGadget::QueryState(const Message &message) const
{
	if (message.getPublisher() == "GetSelectRyushkaName") {
		if (_selectedRyushka)
		{
			return Message("", Int::ToString(_selectedRyushka->GetId()));
		}
	}
	return Message("");
}

void RyushkiGadget::AcceptMessage(const Message &message)
{
	if (message.is("SetRyushkaLink"))
	{
		bool reset  = true;

		GUI::Widget* w ;
		Message msg;
		Layer* l = Core::guiManager.getLayer("RyushkaParentLayer");
		w = l->getWidget("RyushkaParentName");
		msg = w->QueryState(Message("Text"));
		std::string  parent = msg.getData();

		RyushkiVector::iterator i = ryushki.begin(), e = ryushki.end();
		for (; i != e; ++i)
		{
			if (Int::ToString((*i)->GetId()) == parent)
			{
				_selectedRyushka->SetConnect((*i));
				reset = false;
				break;
			}

		}
		if (reset)
		{
			_selectedRyushka->ResetConnect();
		}
	}else if(message.is("KeyPress"))
	{
		int key = utils::lexical_cast <int> (message.getData());
		if ((key == 'm' || key == 'M') && _selectedRyushka)
		{
			_selectedRyushka->MirrorX();
		}else if (key == -69 /* e */)
		{
			if (_selectedRyushka)
			{
				_selectedRyushka -> IncRadius();
			}
		}
		//Получается на каждое нажатие любой клавиши это делается
		if (key > 0)
		{
			if(_selectedRyushka)
			{
				_selectedRyushka->SetZLevelByKey(key);
			}
			Sort();
		}
	}else{
		for (size_t i = 0; i < ryushki.size(); i++)
		{
			ryushki[i]->AcceptMessage(message);
		}
	}
}

void RyushkiGadget::Editor_MoveRyushki(const IPoint& delta)
{
	size_t count = ryushki.size();
	for (size_t i = 0; i < count; i++)
	{
		Ryushka::HardPtr ryushka = ryushki[i];
		IRect r = ryushka -> getVisibleRect();
		IPoint p (r.x, r.y);
		
		p.x += delta.x * GameSettings::SQUARE_SIDE;
		p.y += delta.y * GameSettings::SQUARE_SIDE;

		ryushka -> setPosition(p);
	}
}


bool RyushkiGadget::Editor_MouseDown(const IPoint &mouse_pos)
{
	_mouseDownPoint = mouse_pos;
	if (Core::mainInput.GetMouseRightButton())
	{
		//Правая кнопка мыши
		UnSelectRyushka();
	}else{
		if (_selectedRyushka)
		{
			if (_sliderAngle.MouseDown(mouse_pos)) {
				RefreshRyushkaSliders();
				_selectedRyushka->SetAngle(_sliderAngle.GetFactor() * 360);
			} else if (_sliderScale.MouseDown(mouse_pos)) {
				RefreshRyushkaSliders();
				_selectedRyushka->SetScaleByFactor(_sliderScale.GetFactor());
			} else if (_sliderDepth.MouseDown(mouse_pos)) {
				RefreshRyushkaSliders();
				_selectedRyushka->SetDepth(_sliderDepth.GetFactor());
			}
			return true;
		}
		IPoint p = mouse_pos + GameSettings::FieldCoordMouse();
		for (RyushkiVector::reverse_iterator i = ryushki.rbegin(); i != ryushki.rend(); ++i)
		{
			// Если попали в рюшку:
			if ((*i)->getVisibleRect().Contains(p))
			{
				// Есть вероятность, что вывделенная сменилась, поэтому
				// переинициализируем все слайдеры.
				UnSelectRyushka();

				_selectedRyushka = (*i)->SelectRyushka();
				IRect rect = (*i)->getVisibleRect();
				rect.x -= -GameSettings::FieldCoordMouse().x;
				rect.y -= GameSettings::FieldCoordMouse().y;
				
				//_sliderScale.SetPosition(IPoint(rect.x, rect.y - 20));
				//_sliderScale.SetSize(rect.width);
				_sliderScale.SetFactor(_selectedRyushka->GetFactorByScale());
				
				//_sliderAngle.SetPosition(IPoint(rect.x, rect.y - 40));
				//_sliderAngle.SetSize(rect.width);
				_sliderAngle.SetFactor(_selectedRyushka->GetAngle() / 360.0f);
				
				//_sliderDepth.SetPosition(IPoint(rect.x, rect.y - 60));
				//_sliderDepth.SetSize(rect.height);
				_sliderDepth.SetFactor(_selectedRyushka->GetDepth());
				RefreshRyushkaSliders();
				_selectedRyushka -> SetDepthStyle(FieldStyles::current->fonDrawer.ryushkaDepthColor);
				break;
			}
		}
	}
	return false;
}

void RyushkiGadget::Editor_MouseMove(const IPoint &mouse_pos)
{
	if(Core::mainInput.GetMouseLeftButton())
	{
		if (_selectedRyushka)
		{
			// К cожалению, MiniSlider не дает возможноcти понять,
			// какой из cлайдеров cейчаc активен,
			// поэтому обновляем и опрашиваем вcе cлайдеры.
			bool targeting_slider = false;
			targeting_slider = _sliderAngle.MouseMove(mouse_pos) || targeting_slider;
			targeting_slider = _sliderScale.MouseMove(mouse_pos) || targeting_slider;
			targeting_slider = _sliderDepth.MouseMove(mouse_pos) || targeting_slider;

			RefreshRyushkaSliders();

			_selectedRyushka->SetAngle(_sliderAngle.GetFactor() * 360);
			_selectedRyushka->SetDepth(_sliderDepth.GetFactor());
			_selectedRyushka->SetScaleByFactor(_sliderScale.GetFactor());
			if(targeting_slider)
			{
				return;
			}
		}
		if (_selectedRyushka)
		{
			IRect r = _selectedRyushka->getClientRect();
			_selectedRyushka->setPosition(IPoint(r) + mouse_pos - _mouseDownPoint);
			_mouseDownPoint = mouse_pos;
		}
	}else{
		//Правая кнопка мыши

		Ryushka::HardPtr oldUnderMouseRyushka = _underMouseRyushka;
		_underMouseRyushka.reset();
		IPoint p = mouse_pos + GameSettings::FieldCoordMouse();

		for (size_t i = 0; i<ryushki.size(); i++)
		{
			IRect r = ryushki[i]->getVisibleRect();
			if (p.x >= r.x && p.x < r.x + r.width && p.y >= r.y && p.y < r.y + r.height)
			{
				_underMouseRyushka = ryushki[i];
			}
		}
	}
}

void RyushkiGadget::Editor_MouseUp(const IPoint &mouse_pos)
{
	if (_selectedRyushka)
	{
		_sliderAngle.MouseUp(mouse_pos);
		_sliderScale.MouseUp(mouse_pos);
		_sliderDepth.MouseUp(mouse_pos);
	}
}


void RyushkiGadget::Sort()
{
	//Раньше использовалось два варианта сортировки
	// Сортируем рюшки в порядке увеличения zLevel -> проще рисовать
	//std::sort(ryushki.begin(), ryushki.end(), EditorUtils::RyushkiDepthComparison);

	// Сортируем рюшки в порядке увеличения (один раз - в начале) zLevel -> проще риcовать
	std::stable_sort(ryushki.begin(), ryushki.end(), RyushkiDepthComparison);
}


void RyushkiGadget::RefreshRyushkaSliders() {
	if (_selectedRyushka) {
		_sliderScale.SetLabel(std::string("scale : ") + FloatToString(_selectedRyushka->GetScale(), 2));
	}
	_sliderAngle.SetLabel(std::string("angle : ") + Int::ToString(math::round(360 * _sliderAngle.GetFactor())));
	_sliderDepth.SetLabel(std::string("depth : ") + FloatToString(_sliderDepth.GetFactor(), 2));
}

void RyushkiGadget::AddRyushka(Ryushka::HardPtr r)
{
	r -> SetDepthStyle(FieldStyles::current->fonDrawer.ryushkaDepthColor);
	ryushki.push_back(r);

	if (_selectedRyushka) 
	{
		_selectedRyushka = _selectedRyushka->UnSelectRyushka();
	}

}

void RyushkiGadget::DeleteSelectedRyushka()
{
	if (!_selectedRyushka) {
		return;
	}

	RyushkiVector::iterator i = std::find(ryushki.begin(), ryushki.end(), _selectedRyushka);
	Assert(i != ryushki.end());

	_selectedRyushka->Release();

	if (_underMouseRyushka == _selectedRyushka) {
		_underMouseRyushka.reset();
	}

	if (_selectedRyushka){
		_selectedRyushka = _selectedRyushka->UnSelectRyushka();
	}

	ryushki.erase(i);
}

void RyushkiGadget::UnSelectRyushka()
{
	if (_selectedRyushka) 
	{
		_selectedRyushka = _selectedRyushka->UnSelectRyushka();
	}
}

void RyushkiGadget::Editor_Draw()
{
	if(_selectedRyushka)
	{
		_sliderAngle.Draw();
		_sliderScale.Draw();
		_sliderDepth.Draw();
	}
}

void RyushkiGadget::Editor_DrawField()
{
	// обводим рюшку, на которую наведена мышь
	if (_underMouseRyushka != NULL) {
		_underMouseRyushka->DrawRamka();
	}
}