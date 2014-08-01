#include "stdafx.h"
#include "EnergyReceivers.h"
#include "Game.h"
#include "GameInfo.h"
#include "EditorUtils.h"
#include "FreeFront.h"
#include "Match3Gadgets.h"
#include "Energy.h"
#include "SnapGadgetsClass.h"

namespace Gadgets
{
	EnergyReceivers receivers;
}

EnergyReceivers::EnergyReceivers()
	:_lastReceiver(-100, -100)
{
}

void EnergyReceivers::Init(GameField *field, const bool &with_editor)
{
	BaseEditorMaker::Init(field, with_editor);
}

EnergyReceiver* EnergyReceivers::MoreFar()
{
	if(receivers.empty())
	{
		Assert(false);
		return nullptr;
	}
	float min_dist = 40000000.f;
	EnergyReceiver * find_rec = receivers.begin()->get();
	for(Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		IPoint pos = (*itr)->GetIndex();
		if(pos.x < 0 || pos.y < 0)
		{
			Assert(false); //Какой-то странный приемник энергии - находится вне поля.
			continue;
		}
		float dist = Gadgets::squareDist[pos.x][pos.y];
		if(dist < min_dist)
		{
			min_dist = dist;
			find_rec = itr->get();
		}
	}
	return find_rec;
}

void EnergyReceivers::InitLevel()
{
	if(!Gadgets::snapGadgets.IsLoaded())
	{
		Assert(false);
	}
	_energyReceiverSelected = nullptr;

	for (EnergyReceiver::HardPtr er : receivers)
	{
		er->InitLevel();
	}
}

void EnergyReceivers::LoadLevel(rapidxml::xml_node<> *xml_level)
{
	// загрузим инфу о клеточках, которые cчитаютcя проницаемыми для энергии c точки зрения cтрелки, указывающей путь к приёмнику
	rapidxml::xml_node<> *elem = xml_level->first_node("RecArrowSquares");
	if (elem)
	{
		elem = elem->first_node("Square");

		while (elem)
		{
			int x = utils::lexical_cast<int>(elem->last_attribute("x")->value());
			int y = utils::lexical_cast<int>(elem->last_attribute("y")->value());

			GameSettings::recfield[x+1][y+1] = true;

			elem = elem->next_sibling("Square");
		}
	}

	Clear(); // Удаляем приёмники и затем загружаем из файла
	rapidxml::xml_node<>* receiversRoot = xml_level->first_node("EnergyReceivers");
	rapidxml::xml_node<>* xml_receiver = receiversRoot->first_node("EnergyReceiver");
	while (xml_receiver)
	{
		EnergyReceiver::HardPtr rec = boost::make_shared<EnergyReceiver>();
		rec->LoadLevel(xml_receiver);
		receivers.push_back(rec);

		xml_receiver = xml_receiver->next_sibling ("EnergyReceiver");
	}
	ResetForEditor();
}

void EnergyReceivers::Clear()
{
	_energyReceiverSelected = nullptr;
	receivers.clear();
}

EnergyReceiver::HardPtr EnergyReceivers::GetReceiver(IPoint index)
{
	if( Game::isVisible(index.x, index.y) )
	{
		for (EnergyReceiver::HardPtr er : receivers)
		{
			if( !er->IsActivatedByEnergy() && er->GetIndex() == index )
				return er;
		}
	}
	return EnergyReceiver::HardPtr();
}

bool EnergyReceivers::IsReceiverCell(Game::FieldAddress fa) const
{
	if(!fa.IsValid())
	{
		return false;
	}
	IPoint pt = fa.ToPoint();

	for (EnergyReceiver::HardPtr er : receivers)
	{
		if( !er->IsActivatedByEnergy() && er->GetIndex() == pt )
			return true;
	}
	return false;
}

bool EnergyReceivers::IsReceiverCell(const IPoint &mouse_pos) const
{
	Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
	return IsReceiverCell(index);
}

EnergyReceiver *EnergyReceivers::GetReceiverOnSquare(Game::FieldAddress fa) const
{
	IPoint pt = fa.ToPoint();
	for (EnergyReceiver::HardPtr er : receivers)
	{
		if( er->GetIndex() == pt )
			return er.get();
	}
	return nullptr;
}
void EnergyReceivers::DrawDown()
{
	// Выводим приёмники энергии
    Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.f, 0.f, ZBuf::RECD_OWN));	
	for (EnergyReceiver::HardPtr er : receivers)
    {
        if (Game::visibleRect.Contains(er->GetIndex()))
        {
			er->DrawDown();
        }
    }
    Render::device.PopMatrix();
}
void EnergyReceivers::DrawBase()
{
	// Выводим приёмники энергии
	for (EnergyReceiver::HardPtr er : receivers)
    {
        if (Game::visibleRect.Contains(er->GetIndex()))
        {
            er->DrawBase();
        }
    }
}

void EnergyReceivers::DrawUp()
{
	// Выводим приёмники энергии
	for (EnergyReceiver::HardPtr er : receivers)
    {
        if (Game::visibleRect.Contains(er->GetIndex()))
        {
            er->DrawUp();
        }
    }
}

bool EnergyReceivers::FocusNeed() const
{
	return false;
}

IPoint EnergyReceivers::FocusCenter() const
{
	return receivers.front()->GetCenterPos();
}

void EnergyReceivers::Update(float dt)
{
	// обновляем приёмники энергии
	for (Receivers::iterator it = receivers.begin() ; it != receivers.end(); ++it)
	{
		EnergyReceiver::HardPtr er = (*it);
		IPoint cell_pos = er->GetIndex();
		Game::FieldAddress fa(cell_pos);

		er->Update(dt);

		if( !er->IsActivatedByEnergy() )
		{
			bool can_activate_by_square = false;

			if (Energy::field.EnergyExists(fa))
			{
				can_activate_by_square = true;
			}

			if(er->IsOrdered() && !er->GetOrder()->IsActive() && 
				    (Energy::field.EnergyExists(fa, Energy::Square::LEFT_BOTTOM) || Energy::field.EnergyExists(fa, Energy::Square::LEFT_TOP)
				|| Energy::field.EnergyExists(fa, Energy::Square::RIGHT_BOTTOM) || Energy::field.EnergyExists(fa, Energy::Square::RIGHT_TOP)))
			{
				er->GetOrder()->Activate(true);
			}

			if( can_activate_by_square && !er->IsOrdered())
			{
				er->ActivateByEnergy();
				// перемещаем активированные приемники в начало списка
				receivers.push_front(er);
				it = receivers.erase(it);
				--it;

				if( receivers.back()->IsActivatedByEnergy() ) {
					// активировали все приемники, останавливаем мигание счетчика ходов
					Core::LuaDoString("match3Panel:SetMinValue(-1)");
				}
			}
		}
	}
}

void EnergyReceivers::OnEndMove()
{
	for (Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		EnergyReceiver::HardPtr recv = (*itr);
		if( recv->IsWalking() && !recv->IsActivatedByEnergy() && Game::activeRect.Contains((*itr)->GetIndex()))
		{
			Game::FieldAddress fa(recv->GetIndex());

			// ищем клетку куда бы пойти нашему бродячему приемнику
			std::vector<Game::Square*> squares;
			const int dx[4] = {0, 1,  0, -1};
			const int dy[4] = {1, 0, -1,  0};
			for(int i = 0; i < 4; ++i)
			{
				Game::Square *sq = GameSettings::gamefield[fa.Shift(dx[i], dy[i])];
				if( Game::isVisible(sq) && Game::activeRect.Contains(sq->address.ToPoint()) && !sq->IsHardStand() && !sq->IsIce() && !sq->GetChip().IsLock())
				{
					squares.push_back(sq);
				}
			}

			if( !squares.empty() ) {
				Game::Square *sq = squares[ math::random(0u, squares.size()-1) ];
				recv->WalkTo(sq);
			}
		}
	}
}

bool EnergyReceivers::IsLastReceiver(const Game::FieldAddress &address) const
{
	return address.ToPoint() == _lastReceiver;
}

size_t EnergyReceivers::ActiveCount()
{
	// Вообще список сортируется так, чтобы активные приемники были в начале, а неактивные в конце,
	// поэтому можно еще соптимизировать и не просматривать весь список
	size_t count = 0;
	_lastReceiver = IPoint(-100, -100);
	for (Receivers::const_iterator it = receivers.begin() ; it != receivers.end(); ++it)
	{
		if( (*it)->IsFinished() ) {
			++count;
		} else {
			_lastReceiver = (*it)->GetIndex();
		}
	}
	return count;
}

size_t EnergyReceivers::TotalCount() const
{
	return receivers.size();
}

bool EnergyReceivers::IsFlying()
{
	for (Receivers::iterator itr = receivers.begin() ; itr != receivers.end(); ++itr)
	{
		if((*itr)->IsFlying())
		{
			return true;
		}
	}
	return false;
}

void EnergyReceivers::SaveLevel(Xml::TiXmlElement *xml_level)
{
	// запиcываем приёмники энергии
	Xml::TiXmlElement *xml_receivers = xml_level->InsertEndChild(Xml::TiXmlElement("EnergyReceivers"))->ToElement();
	for (Receivers::iterator itr = receivers.begin() ; itr != receivers.end(); ++itr)
	{
		(*itr)->SaveLevel(xml_receivers);
	}
}

void EnergyReceivers::ReloadEffect()
{
	// Обновляем эффекты на алтарях...
	for (Receivers::iterator itr = receivers.begin() ; itr != receivers.end(); ++itr)
	{
		(*itr)->InitEffects();
	}
}

void EnergyReceivers::ResetForEditor()
{
	_energyReceiverSelected = nullptr;
	for (Receivers::iterator itr = receivers.begin() ; itr != receivers.end(); ++itr)
	{
		if(EditorUtils::editor)
		{
			(*itr)->InitLevel();
		}
	}
}

// Проверяем приёмники энергии...
bool EnergyReceivers::Editor_CheckMinigamesArea(const IRect& part)
{
	for (Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		if (part.Contains((*itr)->GetIndex()))
		{
			return false;
		}
	}
	return true;
}

void EnergyReceivers::Editor_MoveElements(const IRect &part, const IPoint &delta)
{
	// Передвигаем приёмники энергии...
	for (Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		if (!part.Contains((*itr)->GetIndex()))
			continue;

		(*itr)->Editor_Move(delta);
	}
}

// Удаляем приёмник энергии...
bool EnergyReceivers::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	_energyReceiverSelected = nullptr;

	for (Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		if ((*itr)->GetIndex() == sq->address.ToPoint())
		{
			if( !(*itr)->SetOrder(Game::Order::HardPtr()) )
			{
				(*itr)->ReleaseSquares();
				receivers.erase(itr);
			}
			return true;
		}
	}
	return false;
}

Game::Order::HardPtr* EnergyReceivers::Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq)
{
	for (Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		if( (*itr)->GetIndex() == sq->address.ToPoint() ) {
			return (*itr)->Editor_GetOrderPtr();
		}
	}
	return nullptr;
}

void EnergyReceivers::DrawEdit()
{
	// Выводим графику для редактора 
	// приёмников энергии и имя приёмника
	for(Receivers::iterator itr = receivers.begin(); itr != receivers.end(); ++itr)
	{
		bool selected = Core::mainInput.GetMouseLeftButton() && (_energyReceiverSelected == itr->get());
		(*itr)->Editor_DrawIndicators(selected);			
	}
}

bool EnergyReceivers::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square* sq)
{
	if (EditorUtils::activeEditBtn == EditorUtils::EnergyReceiverEdit)
	{
		_energyReceiverSelected = GetReceiverOnSquare(sq->address);

		if(_energyReceiverSelected)
		{
			// Начало перетаскивания приемника
			_energyReceiverSelected->ReleaseSquares();
		}
		else
		{
			if(!Game::isSquare(sq))
			{
				Game::FieldAddress address = GameSettings::GetMouseAddress(mouse_pos);
				sq = Game::GetValidSquare(address);
			}
				sq->SetWall(0);
			EnergyReceiver::HardPtr e = boost::make_shared<EnergyReceiver>(sq->address.GetCol(), sq->address.GetRow(), Core::mainInput.IsControlKeyDown());
			e->InitEffects();
			receivers.push_back(e);
			return true;
		}	
	} 
	return false;
}

bool EnergyReceivers::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
{
	// В конце перетаскивания делаем квадраты под приемником фейковыми
	if(_energyReceiverSelected){
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
		_energyReceiverSelected->SetIndex(index.ToPoint());
		_energyReceiverSelected->InitLevel();
		Game::Order::HardPtr order = _energyReceiverSelected->GetOrder();
		if(order)
		{
			order->SetAddress(index.ToPoint());
		}
		_energyReceiverSelected = nullptr;
	}
	return false;
}

bool EnergyReceivers::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
{
	if (EditorUtils::activeEditBtn != EditorUtils::EnergyReceiverEdit)
	{
		return false;
	}

	if(Core::mainInput.GetMouseLeftButton())
	{
		if(_energyReceiverSelected != nullptr)
		{
			EnergyReceiver *r = _energyReceiverSelected;

			Game::FieldAddress prev_index = Game::FieldAddress(r->GetIndex());
			if(prev_index != sq->address)
			{
				IPoint rpos;
				rpos.x = math::clamp(0, GameSettings::FIELD_MAX_WIDTH - 1, sq->address.GetCol());
				rpos.y = math::clamp(0, GameSettings::FIELD_MAX_HEIGHT - 1, sq->address.GetRow());
				r->SetIndex(rpos);
			}
		}
		return true;
	}
	return false;
}

bool EnergyReceivers::AcceptMessage(const Message &message)
{
	for (EnergyReceiver::HardPtr er : receivers)
	{
		if(er->AcceptMessage(message))
		{
			return true;
		}
	}
	return false;
}

void EnergyReceivers::Editor_Reset()
{
	_energyReceiverSelected = nullptr;
}

bool EnergyReceivers::CanMoveCameraTo(FPoint pos) const
{
	std::vector<EnergyReceiver::HardPtr> inactive;

	IRect area = GameSettings::GetVisibleArea();
	for (EnergyReceiver::HardPtr er : receivers)
	{
		if(!er->IsActivatedByEnergy() && area.Contains(er->GetIndex())){
			inactive.push_back(er);
		}
	}

	area.x = int( pos.x / GameSettings::SQUARE_SIDE + 0.7f);
	area.y = int( pos.y / GameSettings::SQUARE_SIDE + 0.7f);

	for (EnergyReceiver::HardPtr er : inactive)
	{
		if( !area.Contains(er->GetIndex()) ) // один из неактивных приемников может уйти за экран, перемещать камеру нельзя
			return false;
	}

	return true;
}

void EnergyReceivers::Editor_CutToClipboard(IRect part)
{
	_clipboard.clear();
	for(Receivers::iterator itr = receivers.begin(); itr != receivers.end(); )
	{
		if( part.Contains((*itr)->GetIndex()) ) {
			(*itr)->Editor_Move( -part.LeftBottom() );
			_clipboard.push_back(*itr);
			itr = receivers.erase(itr);
		} else {
			++itr;
		}
	}
}

bool EnergyReceivers::Editor_PasteFromClipboard(IPoint pos)
{
	for (EnergyReceiver::HardPtr er : _clipboard)
	{
		er->Editor_Move(pos);
		receivers.push_back(er);
	}
	_clipboard.clear();
	return true;
}

void EnergyReceivers::GetReceiversPositions(std::vector<IPoint> &vec) const
{
	vec.clear();
	for (EnergyReceiver::HardPtr er : receivers)
	{
		vec.push_back( er->GetIndex() );
	}
}