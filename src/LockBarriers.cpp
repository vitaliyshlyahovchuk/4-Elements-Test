#include "stdafx.h"
#include "LockBarriers.h"
#include "EditorUtils.h"
#include "GameField.h"
#include "ChipsInfo.h"
#include "GameFieldControllers.h"
#include "Match3.h"
#include "Energy.h"

namespace Gadgets
{
	LockBarriers lockBarriers;
}

float LockBarrierBase::YOffset_LOCK = 0.f;

LockBarrierBase::LockBarrierBase(const std::string &type)
	: _type(type)
	, _destroying(false)
	, _destroyed(false)
	, _killTimer(0.f)
	, need_remove(false)
{
	_field = GameField::Get();
}

void LockBarrierBase::InitLevel()
{

}

std::string LockBarrierBase::GetType() const
{
	return _type;
}

void LockBarrierBase::InitCell()
{
	Game::Square *cell =  Game::GetValidSquare(_pos.x, _pos.y);

	cell->Reset();
		
	cell->SetStone(false);
	cell->ice = false;
	cell->SetFake(true);
	cell->SetWall(1);
	cell->GetChip().Reset(true);

	cell->barrierIndex = 1;
	_effectCont.KillAllEffects();
	ParticleEffect* eff = _effectCont.AddEffect("LockLive");
	eff->SetPos(FPoint(0.f, LockBarrierBase::YOffset_LOCK));
	eff->Reset();
	eff->Update(100.f);
	_effectCont.Update(0.f);
}

bool LockBarrierBase::IsDestroyed() const
{
	return _destroyed;
}

void LockBarrierBase::FreeCell(const bool &remove_cell)
{
	Game::Square *sq = GameSettings::gamefield[_pos];
	if( !Game::isBuffer(sq) )
	{
		sq->SetLockWithOrder(HardPtr());

		if(remove_cell)
		{
			sq->DecWall();
		}
		sq->barrierIndex = -1;
		sq->SetFake(false);
		Energy::field.UpdateSquare(sq->address);

		_effectCont.KillAllEffects();

		Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["LockBreak"], FPoint(0.f, LockBarrierBase::YOffset_LOCK) + FPoint(_pos)*GameSettings::SQUARE_SIDEF, FlashAnimationPlayer::DRAW_UP));
		//ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effTopCont, "LockBreak");
		//eff->SetPos(FPoint(0.f, LockBarrierBase::YOffset_LOCK) + FPoint(_pos)*GameSettings::SQUARE_SIDEF);
		//eff->Reset();
	}
}

IPoint LockBarrierBase::GetPos() const
{
	return _pos;
}

void LockBarrierBase::SetPos(IPoint pos)
{
	_pos = pos;
}

bool LockBarrierBase::Update(float dt)
{
	_effectCont.Update(dt);
	return _effectCont.IsFinished();
}

void LockBarrierBase::DrawLockBase(const FPoint pos)
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(pos);
	Game::MatrixSquareScale();
	_effectCont.Draw();
	Render::device.PopMatrix();
}

void LockBarrierBase::Load(rapidxml::xml_node<> *xml_elem)
{
	_pos = xml_elem;
	InitCell();
}

void LockBarrierBase::Save(Xml::TiXmlElement *elem)
{
	//по алфавиту!
	elem->SetAttribute("type", GetType().c_str());
	elem->SetAttribute("x", _pos.x);
	elem->SetAttribute("y", _pos.y);
}

/***************************/
//	LockBarrierWithKey		
/***************************/
LockBarrierWithKey::LockBarrierWithKey()
	: LockBarrierBase("chip_key")
{
	//_texLock = Core::resourceManager.Get<Render::Texture>("LockBarrierOrder");
}


void LockBarrierWithKey::Destroy()
{
	_destroyed = true;
	FreeCell(true);

	Match3::RunFallColumn(_pos.x);
}


bool LockBarrierWithKey::Update(float dt)
{
	LockBarrierBase::Update(dt);
	if(!_destroying)
	{
		Game::Square *sq = GameSettings::gamefield[_pos.x+1][_pos.y+2];
		if(sq->GetChip().IsKey() && sq->GetChip().IsStand())
		{
			_destroying = true;
			Game::AddController(new LockBarrierUnlockEffect(_field, this));

			sq->GetChip().SetInfo(Gadgets::InfoC(), IPoint(-1, -1));
			sq->SetBusyCell(1);
			Match3::RunFallColumn(_pos.x);
			return true;
		}
	}
	return false;
}



void LockBarrierWithKey::Draw()
{
	//if( !_destroyed )
	//{
		FPoint pos = GetPos()*GameSettings::SQUARE_SIDE;
		DrawLockBase(pos);
	//}
}

void LockBarrierWithKey::DrawUp()
{

}

/***************************/
//	LockBarrierOrder		
/***************************/
LockBarrierOrder::LockBarrierOrder()
	: LockBarrierBase("order")
	, _localTime(math::random(0.0f, math::PI*2.0f))
	, _readyTimer(0.f)
{
	//_texFly = Core::resourceManager.Get<Render::Texture>("LockBarrierOrder");
	//_offsetTexture = FPoint(_tex->getBitmapRect().Width()/2.f, _tex->getBitmapRect().Height()/2.f);
	_offsetTexture = FPoint(0.f, 0.f);
}

void LockBarrierOrder::InitLevel()
{
	if(_order)
	{
		_order->InitAppear(1.f, 1.5f);
	}
}

bool LockBarrierOrder::Update(float dt)
{
	if(LockBarrierBase::Update(dt))
	{
		_destroyed = true;
		Match3::RunFallColumn(_pos.x);
	}
	if(_order)
	{
		Game::Square *sq = GameSettings::gamefield[_pos];
		if(Game::isVisible(sq) && sq->IsStayFly())
		{
			_order->Update(dt);
		}
	}
	if(_destroying)
	{
		if(_killTimer < 0)
		{
			_killTimer += dt;
			if(_killTimer >= 0)
			{
				Game::AddController(new LockBarrierUnlockEffect(_field, this));
				return true;
			}
		}
		return false;
	}
	else
	{
		_destroying = !_order || _order->Completed();
		if(_destroying)
			_killTimer = -0.5f;
	}

	_localTime += dt;
	bool ready = _destroying;
	if(ready && _readyTimer < 1){
		_readyTimer += dt*2.f;
		if(_readyTimer >= 1)
		{
			_readyTimer = 1.f;
		}
	}else if(!ready && _readyTimer > 0){
		_readyTimer -= dt*2.f;
		if(_readyTimer <=0)
		{
			_readyTimer = 0.f;
		}
	}
	return false;
}

void LockBarrierOrder::Draw()
{

	//Game::Square *sq = GameSettings::gamefield[_pos.x+1][_pos.y+1];
	IPoint pos = GetPos()*GameSettings::SQUARE_SIDE;
	//FPoint tex_lock_pos = pos;
	if(_readyTimer > 0)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(_offsetTexture + pos);
		Render::device.MatrixRotate(math::Vector3(0.f, 0.f, 1.f), sinf(_localTime*40.f)*_readyTimer*4.f);
		Render::device.MatrixScale(1.f + sinf(_localTime*40.f)*0.1f);
		//_tex->Bind();
		//Render::DrawRect(FRect(-_offsetTexture.x, - _offsetTexture.x + GameSettings::SQUARE_SIDEF, -_offsetTexture.y, -_offsetTexture.y + GameSettings::SQUARE_SIDEF), FRect(0.f, 1.f, 0.f, 1.f));
		DrawLockBase(-_offsetTexture);
		Render::device.PopMatrix();
	}else{
		DrawLockBase(pos);
	}
}

void LockBarrierOrder::DrawUp()
{
	if(_order && !_order->Completed()){
		IPoint pos = GetPos()*GameSettings::SQUARE_SIDE;
		_order->Draw(pos);
	}
}

void LockBarrierOrder::Destroy()
{
	FreeCell(true);
}

bool LockBarrierOrder::SetOrder(Game::Order::HardPtr order)
{
	bool result = (_order != NULL);
	_order = order;
	return result;
}

Game::Order::HardPtr* LockBarrierOrder::GetOrder()
{
	return &_order;
}

void LockBarrierOrder::Save(Xml::TiXmlElement *elem)
{
	LockBarrierBase::Save(elem);

	if(_order)
	{
		Xml::TiXmlElement *orderElem = elem->InsertEndChild(Xml::TiXmlElement("Order"))->ToElement();
		_order->Save(orderElem);
	}
}

void LockBarrierOrder::Load(rapidxml::xml_node<> *elem)
{
	LockBarrierBase::Load(elem);

	rapidxml::xml_node<> *orderElem = elem->first_node("Order");
	if( orderElem ) {
		_order = Game::Order::Load(orderElem);
		if(_order)
			_order->SetAddress(_pos);
	}
}

/*****************************************************************************************/
/***************************/
//	LockBarriers		
/***************************/
LockBarriers::LockBarriers()
	: _selected(NULL)
{
}

void LockBarriers::Clear()
{
	_selected = NULL;
	_barriers.clear();
}

void LockBarriers::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear();

	rapidxml::xml_node<> *barriersElem = root->first_node("LockBarriers");
	if( barriersElem )
	{
		rapidxml::xml_node<> *barrier = barriersElem->first_node("Barrier");
		while( barrier ){
			std::string type = Xml::GetStringAttributeOrDef(barrier, "type", "chip_key");
			if(type == "chip_key")
			{
				_barriers.push_back(LockBarrierWithKey::HardPtr(new LockBarrierWithKey()));
				_barriers.back()->Load(barrier);
				Game::Square *sq = GameSettings::gamefield[_barriers.back()->GetPos()];
				sq->SetLockWithOrder(_barriers.back());
			}else if(type == "order"){
				_barriers.push_back(LockBarrierOrder::HardPtr(new LockBarrierOrder()));
				_barriers.back()->Load(barrier);
				Game::Square *sq = GameSettings::gamefield[_barriers.back()->GetPos()];
				sq->SetLockWithOrder(_barriers.back());
			}
			barrier = barrier->next_sibling("Barrier");
		}
	}	
}

void LockBarriers::SaveLevel(Xml::TiXmlElement *root)
{
	if(_barriers.empty())
	{
		return;
	}
	CheckRepeat();
	Xml::TiXmlElement *barriers = root->InsertEndChild(Xml::TiXmlElement("LockBarriers"))->ToElement();
	for(size_t i = 0; i < _barriers.size(); ++i)
	{
		Xml::TiXmlElement *barrier = barriers->InsertEndChild(Xml::TiXmlElement("Barrier"))->ToElement();
		_barriers[i]->Save(barrier);
	}
}

bool LockBarriers::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	_selected = NULL;
	if (EditorUtils::activeEditBtn == EditorUtils::LockBarrier || EditorUtils::activeEditBtn == EditorUtils::BtnLockBarrierOrder)
	{
		if(Game::isSquare(sq))	
		{
			_selected = BarrierOnCell(sq->address);
		}
		if( _selected )
		{
			// начинаем перетаскивание, освобождаем клетки под замком
			_selected->FreeCell(false);
		}
		else
		{
			// кликнули на пустой клетке, ставим новый замок
			LockBarrierBase::HardPtr b;
			if (EditorUtils::activeEditBtn == EditorUtils::LockBarrier)
			{
				b = boost::make_shared<LockBarrierWithKey>();
				b->SetPos(sq->address.ToPoint());
				b->InitCell();
			}
			else if (EditorUtils::activeEditBtn == EditorUtils::BtnLockBarrierOrder)
			{
				b = boost::make_shared<LockBarrierOrder>();
				b->SetPos(sq->address.ToPoint());
				b->InitCell();
			}
			if(b){
				_barriers.push_back(b);
			}else{
				MyAssert(false);
			}
		}
	}
	return false;
}

bool LockBarriers::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::None
		|| EditorUtils::activeEditBtn == EditorUtils::Null
		|| EditorUtils::activeEditBtn == EditorUtils::LockBarrier
		|| EditorUtils::activeEditBtn == EditorUtils::PlaceOrder )
	{
		// по правому клику удаляем замок
		IPoint sq_pos = sq->address.ToPoint();
		for(Barriers::iterator itr = _barriers.begin(); itr != _barriers.end(); ++itr)
		{
			if( (*itr)->GetPos() == sq_pos )
			{
				//Сначала удаляется заказ, затем сам замок, исключение - режим редактирования EditorUtils::Null. В этом режиме удаляется все подчистую.
				if( !(*itr)->SetOrder(Game::Order::HardPtr()) || EditorUtils::activeEditBtn == EditorUtils::Null)
				{
					(*itr)->FreeCell(false);
					_barriers.erase(itr);
				}
				return true;
			}
		}
	}
	return false;
}

void LockBarriers::CheckRepeat()
{
	if(_barriers.size() <= 1)
	{
		return;
	}
	//Метим те что нужно удалить
	for(Barriers::iterator itr = _barriers.begin(); itr != _barriers.end(); itr++)
	{
		const IPoint &pos = (*itr)->GetPos();
		for(Barriers::iterator itr2 = itr+1; itr2 != _barriers.end(); itr2++)
		{
			(*itr2)->need_remove = (*itr2)->need_remove || (pos == (*itr2)->GetPos());
		}
	}
	for(Barriers::iterator itr = _barriers.begin()+1; itr != _barriers.end();)
	{
		if((*itr)->need_remove){
			itr = _barriers.erase(itr);
		}else{
			itr++;
		}
	}
}

bool LockBarriers::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
{
	if(_selected)
	{
		// закончили переаскивание, инициализируем клетки в новой позиции замка
		_selected->InitCell();
		_selected = NULL;
		CheckRepeat();
		return true;
	}
	CheckRepeat();
	return false;
}

bool LockBarriers::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
{
	if (EditorUtils::activeEditBtn == EditorUtils::LockBarrier || EditorUtils::activeEditBtn == EditorUtils::BtnLockBarrierOrder)
	{
		if( Core::mainInput.GetMouseLeftButton() )
		{
			if(_selected){
				Game::FieldAddress prev = Game::FieldAddress(_selected->GetPos());
				if( prev != sq->address )
				{
					_selected->SetPos(sq->address.ToPoint());
				}
			}
		}
	}
	return false;
}

void LockBarriers::Update(float dt)
{
	for(Barriers::iterator itr = _barriers.begin(); itr != _barriers.end();)
	{
		if(Game::activeRect.Contains((*itr)->GetPos()))
		{
			(*itr)->Update(dt);
		}
		if((*itr)->IsDestroyed()){
			itr = _barriers.erase(itr);
		}else{
			itr++;
		}
	}
}

void LockBarriers::Draw()
{
	for(Barriers::iterator itr = _barriers.begin(); itr != _barriers.end(); ++itr)
	{
		(*itr)->Draw();
	}
}

void LockBarriers::DrawUp()
{
	for(Barriers::iterator itr = _barriers.begin(); itr != _barriers.end(); ++itr)
	{
		(*itr)->DrawUp();
	}
}

LockBarrierBase* LockBarriers::BarrierOnCell(Game::FieldAddress fa) const
{
	IPoint pos = fa.ToPoint();
	for(Barriers::const_iterator itr = _barriers.begin(); itr != _barriers.end(); ++itr)
	{
		if( (*itr)->GetPos() == pos )
			return itr->get();
	}
	return NULL;
}

bool LockBarriers::isLocked(Game::FieldAddress fa) const
{
	IPoint pos = fa.ToPoint();
	for(Barriers::const_iterator itr = _barriers.begin(); itr != _barriers.end(); ++itr)
	{
		if(!(*itr)->IsDestroyed() &&  (*itr)->GetPos() == pos )
		{
			return true;
		}
	}
	return false;
}

Game::Order::HardPtr* LockBarriers::Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq)
{
	LockBarrierBase *barrier = BarrierOnCell(sq->address);
	if( barrier ) {
		return barrier->GetOrder();
	} else {
		return NULL;
	}
}

void LockBarriers::Editor_CutToClipboard(IRect part)
{
	_clipboard.clear();
	for(Barriers::iterator itr = _barriers.begin(); itr != _barriers.end(); )
	{
		if( part.Contains((*itr)->GetPos()) ) {
			(*itr)->SetPos( (*itr)->GetPos() - part.LeftBottom() );
			_clipboard.push_back(*itr);
			itr = _barriers.erase(itr);
		} else {
			++itr;
		}
	}
}

bool LockBarriers::Editor_PasteFromClipboard(IPoint pos)
{
	for(Barriers::iterator itr = _clipboard.begin(); itr != _clipboard.end(); ++itr)
	{
		(*itr)->SetPos((*itr)->GetPos() + pos);
		_barriers.push_back(*itr);
	}
	_clipboard.clear();
	return true;
}

void LockBarriers::InitLevel()
{
	for(auto i : _barriers)
	{
		i->InitLevel();
	}
}

/******************************************************************************************/


LockBarrierUnlockEffect::LockBarrierUnlockEffect(GameField *field, LockBarrierBase *barrier)
	: GameFieldController("LockBarrierUnlock", 1.0f, field)
	, _barrier(barrier)
{
	_keyTex = Core::resourceManager.Get<Render::Texture>("Chips");
}

void LockBarrierUnlockEffect::Update(float dt)
{
	local_time += 1.5f * dt;
}

void LockBarrierUnlockEffect::Draw()
{
	IPoint pos = _barrier->GetPos() * GameSettings::SQUARE_SIDE;
	pos.y += math::round((1.0f - local_time) * GameSettings::SQUARE_SIDE);

	IRect rect(pos.x, pos.y, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
	FRect uv = Game::GetChipRect(Game::KEY, false, false, false);
	_keyTex->Draw(rect, uv);
}

bool LockBarrierUnlockEffect::isFinish()
{
	if(local_time >= 1.0f)
	{
		_barrier->Destroy();
		return true;
	}
	return false;
}
