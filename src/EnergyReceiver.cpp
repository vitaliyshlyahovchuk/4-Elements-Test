#include "stdafx.h"
#include "EnergyReceiver.h"
#include "EditorUtils.h"
#include "Energy.h"
#include "ReceiverEffects.h"
#include "Tutorial.h"
#include "GameFieldControllers.h"
#include "GameField.h"
#include "Match3.h"
#include "SnapGadgetsClass.h"
#include "LockBarriers.h"
#include "GameInfo.h"
#include "EnergyReceivers.h"

/*
*	WallState
*/


void WallState::Read(Game::Square *sq)
{
	wall = sq->GetWall();
	energy_wall = sq->IsEnergyWall();
}

void WallState::Apply(Game::Square *sq)
{
	sq->SetWall(wall);
	sq->SetEnergyWall(energy_wall);
}

/*
*	EnergyReceiver
*/

const FPoint CRYSTAL_POS_ANIM_ON_SQUARE = FPoint(-82.f, -80.f);
int UID_NEXT = 0;

void EnergyReceiver::Init(int row, int col, bool can_walk)
{
	_timerForRunChipFall = -1.f;
	_cell = 0;
	_textureBaseDown = Core::resourceManager.Get<Render::Texture>("Receiver_down");
	_crystalAnim = Render::StreamingAnimation::Spawn("CrystalRotate");
	SetIndex(IPoint(row, col));
	_can_walk = can_walk;
	_state = ER_INIT;

	_timeAppear = -0.2;
	_crystalAnim->SetPlayback(true);
	_crystalAnim->SetCurrentFrame(math::random(_crystalAnim->GetFirstPlayedFrame(), _crystalAnim->GetLastPlayedFrame()));
	_partUp = NULL;
	_fly_offset_y = 0.f;
	_localTime = 0.f;
	_firstShowed = false;
	_lockLiveEffectShowed = false;
	_timerForAppearOrder = -1.f;
	_hideByThiefTime = 0.f;
	_hideByThief = false;

	//_timerLight = math::random(10.f, 30.f);
}

EnergyReceiver::EnergyReceiver()
{
	Init(0,0,false);
}

EnergyReceiver::EnergyReceiver(int x, int y, bool walking)
{
	Init(x,y, walking);
}

IPoint EnergyReceiver::GetIndex() const
{
	return _index;
}

void EnergyReceiver::SetIndex(IPoint index)
{
	_index = index;
	if(Game::FieldAddress(index).IsValid())
	{
		_cell = GameSettings::gamefield[_index];
	}else{
		_cell = 0;
	}
	if(_order) {
		_order->SetAddress(_index);
	}

}


EnergyReceiver::~EnergyReceiver()
{
	ReleaseEffects();
}


void EnergyReceiver::InitEffects()
{
	ReleaseEffects();

	//if(IsFinished())
	//{
	//	return;
	//}

	//if(_order)
	//{
	//	ParticleEffect* eff = _effectCellCont.AddEffect("LockLive");
	//	eff->SetPos(FPoint(0.f, LockBarrierBase::YOffset_LOCK));
	//	eff->Reset();
	//	eff->Update(100.f);
	//	_order->InitAppear(-1.0f, -1.0f);
	//}
	//_firstShowed = false;
	//_lockLiveEffectShowed = false;
	//_timeAppear = 4.f;
}

void EnergyReceiver::ReleaseEffects()
{
	if(_partUp.get())
	{
		_partUp->Finish();
	}
	_partUp.reset();
	_effContUpStatic.KillAllEffects();
	_effContUp.KillAllEffects();
	_effContDown.KillAllEffects();
	_effectCellCont.KillAllEffects();
	_effectCellCont.Update(0.f);
}

void EnergyReceiver::LoadLevel(rapidxml::xml_node<>* root)
{
	_can_walk = Xml::GetBoolAttributeOrDef(root, "walk", false);
	SetIndex(IPoint(root->first_node("Position")));

	rapidxml::xml_node<>* orderElem = root->first_node("Order");
	if( orderElem )
	{
		_order = Game::Order::Load(orderElem);
		if(_order) {
			_order->SetAddress(_index);
		}
	}
	ReleaseEffects();
}

void EnergyReceiver::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *element = root->InsertEndChild(Xml::TiXmlElement("EnergyReceiver"))->ToElement();

	if(_can_walk)
	{
		element->SetAttribute("walk", utils::lexical_cast(_can_walk));
	}

	Xml::TiXmlElement *position = element->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	position->SetAttribute("x", _index.x);
	position->SetAttribute("y", _index.y);

	if(_order)
	{
		Xml::TiXmlElement *orderElem = element->InsertEndChild(Xml::TiXmlElement("Order"))->ToElement();
		_order->Save(orderElem);
	}
}

void EnergyReceiver::InitLevel()
{
	if(IsWalking() && !EditorUtils::editor)
	{
		//Собираем точки привязки
		Gadgets::snapGadgets.FindSnap(GetIndex(), _shapElements);
	}

	// Под приёмниками энергии не может быть фишек или cтен. 
	// Функция делает ячейки fake и явно задаёт нулевой уровень

	Game::Square *sq = GameSettings::gamefield[GetIndex()];
	if(!Game::isSquare(sq) )
	{
		sq = Game::GetValidSquare(GetIndex().x, GetIndex().y);
	}
	sq->SetFake(true);
	sq->SetWall(0);

	_timerForRunChipFall = -1.f;
	_crystalTimeScale = 0.6f;
	_uid = utils::lexical_cast(UID_NEXT++);

	ReleaseEffects();
	InitEffects();
}

void EnergyReceiver::ReleaseSquares()
{
	Game::Square *sq = GameSettings::gamefield[GetIndex()];
	if( !Game::isBuffer(sq) )
	{
		sq->SetFake(false);
	}
}

void EnergyReceiver::WalkTo(Game::Square *sq)
{
	Game::Square *old_sq = GameSettings::gamefield[GetIndex()];

	// переставляем фишку с новой клетки
	Game::Square::SwapChips(old_sq, sq);

	// восстанавливаем старую клетку
	_saved_wall.Apply(old_sq);
	old_sq->SetFake(false);
	Energy::field.UpdateSquare(old_sq->address);

	//Тащим за собой точки привязки 
	for(size_t i = 0; i < _shapElements.size(); i++)
	{
		Assert(_shapElements[i]->GetIndex() == GetIndex());
		_shapElements[i]->SetIndex(sq->address.ToPoint()); 
	}


	// меняем новую клетку
	_saved_wall.Read(sq);
	sq->SetWall(0);
	sq->SetFake(true);
	Energy::field.UpdateSquare(sq->address);

	SetIndex(sq->address.ToPoint());

	if(_order) {
		_order->SetAddress(sq->address.ToPoint());
	}
}

bool EnergyReceiver::IsWalking() const
{
	return _can_walk;
}

void EnergyReceiver::SetWalking(bool enable)
{
	_can_walk = enable;
}

bool EnergyReceiver::IsActualForCell() const
{
	return _state == ER_ACTIVATED_BREAK || _state == ER_ACTIVATED_BREAK_FINISHFLY;
}

bool EnergyReceiver::IsActivatedByEnergy() const
{
	return _state >= ER_ACTIVATED_BREAK;
}

bool EnergyReceiver::IsFlying() const
{
	return _state == ER_ACTIVATED_BREAK;
}

bool EnergyReceiver::IsFinished() const
{
	return _state == ER_ACTIVATED_BREAK_FINISHFLY;
}


bool EnergyReceiver::IsOrdered() const
{
	return _order.get() && !_order->Completed() && (_order->IsActive() || EditorUtils::editor);
}



void EnergyReceiver::ActivateByEnergy()
{
	if(_state <= ER_STAND && !_hideByThief)
	{
		_state = ER_ACTIVATED_BREAK;


		MM::manager.PlaySample("ReceiverActivated");

		ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effContUpField, "RecReady");
		eff->SetPos(GetCenterPos());
		eff->Reset();

		Game::AddController(new Receiver::ReceiverEffect(FPoint(GetIndex())*GameSettings::SQUARE_SIDEF + FPoint(0.f, _fly_offset_y * GameSettings::SQUARE_SCALE), _uid, _crystalAnim->GetCurrentFrame()));
		_crystalAnim.reset();

		//_frame1 = _crystalAnim->GetCurrentFrame();
		//_frame2 = (_crystalAnim->GetLastPlayedFrame() - _crystalAnim->GetFirstPlayedFrame())*3 + 3;
		//_crystalTime = 0.f;
		//_crystalTimeScale = 1.f/1.5f;

		_timerForRunChipFall = gameInfo.getConstFloat("TIME_FOR_FOR_RUN_CHIP_AFTER_REMOVE_RECEIVER", 2.f);
		
		if(_partUp.get())
		{
			_partUp->Finish();
			_partUp.reset();
		}
		_effContDown.Finish();
		Tutorial::luaTutorial.AcceptMessage( Message("OnReceiverActivated") );
	}
}

bool EnergyReceiver::SetOrder(Game::Order::HardPtr order)
{
	bool result = (_order != NULL);
	_order = order;
	return result;
}

Game::Order::HardPtr EnergyReceiver::GetOrder()
{
	return _order;
}

Game::Order::HardPtr* EnergyReceiver::Editor_GetOrderPtr()
{
	return &_order;
}

IPoint EnergyReceiver::GetCenterPos() const
{
	return GetIndex()*GameSettings::SQUARE_SIDE + IPoint(GameSettings::SQUARE_SIDE / 2, GameSettings::SQUARE_SIDE / 2);
}


void EnergyReceiver::Editor_DrawIndicators(bool selected)
{
	// подложка под приемником
	Render::device.SetBlendMode(Render::ADD);

	if (selected) 
		Render::BeginColor(Color(128, 255, 200, 48));
	else 
		Render::BeginColor(Color(255, 100, 200, 48));

	Render::device.SetTexturing(false);
	IRect draw = GameSettings::CELL_RECT.Inflated(-2);
	draw.MoveTo(GetIndex()*GameSettings::SQUARE_SIDE);

	Render::DrawRect(draw);

	Render::EndColor();
	Render::device.SetBlendMode(Render::ALPHA);
}


void EnergyReceiver::Editor_Move(IPoint delta)
{
	SetIndex(GetIndex() + delta);
}

bool EnergyReceiver::IsCellDraw()
{
	return _cell && _cell->IsStayFly();
}

void EnergyReceiver::DrawDown()
{
	if(!IsCellDraw())
	{
		return;
	}
	if(!IsFinished()) //Можно рисовать или не рисовать, все равно сверху энергия рисуется (лучше не рисовать)
	{
        Render::device.PushMatrix();
		Render::device.MatrixTranslate(GetIndex()*GameSettings::SQUARE_SIDE);
		if(GameSettings::NEED_SCALE)
		{
			Render::device.MatrixScale(GameSettings::SQUARE_SCALE);
		}
		_textureBaseDown->Draw(-15.f, -13.f);
        Render::device.PopMatrix();
	}
}


void EnergyReceiver::DrawBase()
{
	if(!IsCellDraw())
	{
		return;
	}
	Render::BeginAlphaMul(math::clamp(0.f, 1.f, 1 + _timerForAppearOrder));	
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(GetIndex()*GameSettings::SQUARE_SIDE);
		Game::MatrixSquareScale();
		_effContDown.Draw();
		Render::device.PopMatrix();	
	}
	Render::EndAlphaMul();
}


void EnergyReceiver::DrawUp()
{
	if(!IsCellDraw())
	{
		return;
	}
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(GetIndex()*GameSettings::SQUARE_SIDE);
	Game::MatrixSquareScale();

	if(_crystalAnim.get())
	{
		if(_timeAppear < 1)
		{
			Render::device.BeginClipping(IRect(-10, 4, 20, 20), ClippingMode::BOTTOM);
		}
		////Хитрости для постепенного появления кристала из отверстия
		//FRect frect_crystal = FRect(0.f, 1.f, 0.f, 1.f);
		//FRect rect_crystal = FRect(0.f, _crystalAnim->GetOrigWidth(), 0.f, _crystalAnim->GetOrigHeight());
	
		//rect_crystal.MoveBy(FPoint(17.f, 17.f + _fly_offset_y));

		//FRect cut_rect = rect_crystal.CutBy(FRect(0.f, GameSettings::SQUARE_SIDEF, 5.5f, 200.f));

		//frect_crystal.yStart = 1.f - cut_rect.Height() / rect_crystal.Height();
		//rect_crystal = cut_rect;

		float alpha_cristall = math::clamp(0.f, 1.f, 1 + _timerForAppearOrder);
		if(_hideByThief)
		{
			//rect_crystal.yEnd = math::lerp(rect_crystal.yEnd, rect_crystal.yStart, _hideByThiefTime);
			//frect_crystal.yEnd = math::lerp(frect_crystal.yEnd, frect_crystal.yStart, _hideByThiefTime);
			alpha_cristall *= math::clamp(0.f, 1.f, 1 - (_hideByThiefTime - 0.5f)/0.2f);
		}

		Render::BeginAlphaMul(alpha_cristall);
		{
			if( IsWalking() ) {
				Render::BeginColor( Color(100, 255, 100) );
			}
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(CRYSTAL_POS_ANIM_ON_SQUARE + FPoint(0.f, _fly_offset_y));
			_crystalAnim->Draw();
			Render::device.PopMatrix();
			//_crystalAnim->Bind();
			//Render::DrawRect(rect_crystal.MovedBy(CRYSTAL_POS_ANIM_ON_SQUARE), frect_crystal);
			if( IsWalking() ) {
				Render::EndColor();
			}
		}
		Render::EndAlphaMul();
		if(_timeAppear < 1)
		{
			Render::device.EndClipping();
		}
	}

	Render::BeginAlphaMul(math::clamp(0.f, 1.f, 1 + _timerForAppearOrder));	

	Render::device.PushMatrix();
	_effContUpStatic.Draw();
	Render::device.MatrixTranslate(math::Vector3(0.f, _fly_offset_y , 0.f));
	_effContUp.Draw();
	Render::device.PopMatrix();

	Render::EndAlphaMul();

	FPoint offset(0.f, 0.f);
	if(_order && _order->IsReadyToBreak())
	{
		offset.x += math::sin(_localTime*30.f);
	}

	if(_state != ER_INIT)
	{
		_effectCellCont.Draw();
		if(IsOrdered())
		{
			_order->Draw(offset.Rounded());
		}
	}
	Render::device.PopMatrix();
}



void EnergyReceiver::Update(float dt)
{	
	if( _state == ER_INIT || !GameSettings::gamefield[_index]->IsFlyType(Game::Square::FLY_STAY))
	{
		return;
	}

	_localTime += dt;
	if(_localTime < 0)
	{
		return;
	}

	_fly_offset_y = 6.0f * math::sin(_localTime * 1.2f);
	float t = math::clamp(0.f, 1.f, _timeAppear);
	float ampl = math::clamp(0.f, 1.f, (1.f - _timeAppear/4.f));
	ampl = 60.0f * ampl * ampl; //Очень быстрое затухание
	_fly_offset_y += -40.f * (1.0f - t) + ampl * math::sin(_timeAppear*math::PI - math::PI*0.5f);
	_fly_offset_y *= GameSettings::SQUARE_SCALE;

	if(_hideByThief)
	{
		if(_hideByThiefTime < 0)
		{
			_hideByThiefTime += dt;
		}
		else if(_hideByThiefTime < 1)
		{
			_hideByThiefTime += dt*1.5f;
			if(_hideByThiefTime >= 1)
			{
				_hideByThiefTime = 1.F;
			}
		}
	}

	if(_firstShowed)
	{
		if(_timeAppear < 0) {
			_timeAppear += dt;
		} else {
			_timeAppear += dt*1.5f;
		}
		if(_timeAppear >= 1.f && _state == ER_APPEAR)
		{
			//_timeAppear = 1.f;
			_state = ER_STAND;
		}
	}

	if(!_firstShowed && Game::activeRect.Inflated(1).Contains(GetIndex()))
	{
		_firstShowed = true;
		//Акцентирующий эффект при первом появлении на экране
		ParticleEffect *eff2 = 0;
		if(IsOrdered()) {
			eff2 = _effectCellCont.AddEffect("RecStartAccentLock");
		} else {
			eff2 = _effectCellCont.AddEffect("RecStartAccent");
		}
		eff2->SetPos(FPoint(0.0f, 0.0f));
		eff2->Reset();

		MM::manager.PlaySample("ReceiverShow");

		_timerForAppearOrder = gameInfo.getConstFloat("ShowOrderOnRecDelay", 3.f);
	}
	if(!IsOrdered())
	{
		_timerForAppearOrder = 0.f;
	}
	else if(_timerForAppearOrder > -1)
	{
		if(_timerForAppearOrder > 0)
		{
			_timerForAppearOrder -= dt;
		}else{
			_timerForAppearOrder -= dt*7.f;
		}
		if(_timerForAppearOrder <= 0.5f)
		{
			if(IsOrdered() && !_lockLiveEffectShowed)
			{
				ParticleEffect* eff = _effectCellCont.AddEffect("LockLive");
				eff->SetPos(FPoint(0.f, LockBarrierBase::YOffset_LOCK));
				eff->Reset();
				_effectCellCont.Update(0.f);
				_order->InitAppear(0.8f, 0.7f);
				_lockLiveEffectShowed = true;

				MM::manager.PlaySample("ReceiverOrderAppear");
			}
		}
		if(_timerForAppearOrder < -1)
		{
			_timerForAppearOrder = -1;
		} 
	}
	if(_order)
	{
		_order->Update(dt);
		if(!IsOrdered()) //Все больше не залочены
		{
			SetOrder(Game::Order::HardPtr());
			Energy::field.UpdateSquare(Game::FieldAddress(GetIndex()));
			_effectCellCont.KillAllEffects();

			Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["LockBreak"], FPoint(GetIndex())*GameSettings::SQUARE_SIDEF + FPoint(0.f, LockBarrierBase::YOffset_LOCK), FlashAnimationPlayer::DRAW_UP));

			MM::manager.PlaySample("ReceiverOrderComplete");
		}
	}

	_effectCellCont.Update(dt);
	_effContUpStatic.Update(dt);
	_effContUp.Update(dt);
	_effContDown.Update(dt);

	if(_crystalAnim.get())
	{
		if(_state == ER_STAND)
		{
			_crystalAnim->Update(dt*_crystalTimeScale);
		}else if(_state == ER_ACTIVATED_BREAK){
			Receiver::ReceiverEffect::keep_camera = true;
			//_crystalTime += dt*_crystalTimeScale;
			//if(_crystalTime >= 1)
			//{
			//	_crystalTime = 1.f;
			//	_crystalAnim.reset();
			//	Game::AddController(new Receiver::ReceiverEffect(FPoint(GetIndex())*GameSettings::SQUARE_SIDEF + FPoint(0.f, _fly_offset_y * GameSettings::SQUARE_SCALE), _uid));

			//	ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effCont, "RecBreak");
			//	eff->SetPos(GetCenterPos());
			//	eff->Reset();

			//	ParticleEffectPtr eff2 = Game::AddEffect(GameField::Get()->_effTopCont, "RecBreak_2");
			//	eff2->SetPos(GetCenterPos());
			//	eff2->Reset();
			//}else{
			//	int frames_count = _crystalAnim->GetLastPlayedFrame() - _crystalAnim->GetFirstPlayedFrame();
			//	float t = math::ease(_crystalTime, 0.f, 1.f);
			//	int frame = _crystalAnim->GetFirstPlayedFrame() + (math::lerp(_frame1, _frame2, t) % frames_count);
			//	_crystalAnim->SetCurrentFrame(frame);
			//}
		}
	}

	//if(!HasOrder() && _state <= ER_ACTIVATED_BREAK)
	//{
	//	if(_timerLight > 0)
	//	{
	//		_timerLight -= dt;
	//		if(_timerLight < 0)
	//		{
	//			ParticleEffect *eff = _effContUp.AddEffect("RecLight");
	//			eff->SetPos(FPoint(0.0f, 0.0f));
	//			eff->Reset();
	//			_timerLight = math::random(10.f, 30.f);
	//		}
	//	}
	//}
	if(_timerForRunChipFall >= 0)
	{
		_timerForRunChipFall -= dt;
		if(_timerForRunChipFall <= 0)
		{
			Gadgets::receivers.AcceptMessage(Message("JumpFinished", _uid));
			_timerForRunChipFall = -1.f;
		}
	}
}

bool EnergyReceiver::AcceptMessage(const Message &message)
{
	if(message.is("DestroyFinished"))
	{
		if(message.getData() == _uid)
		{
			if(_state == ER_ACTIVATED_BREAK) {
				_state = ER_ACTIVATED_BREAK_FINISHFLY;
			} else {
				Assert(false);
			}
			return true;
		}else{
			//На остальных приемниках запускаем акцентирующий эффект реакции на прилет другого приемника в лут
			if(!IsOrdered() && !IsActivatedByEnergy())
			{
				ParticleEffect *eff = 0;
				eff = _effContUp.AddEffect("RecOnLoot");
				eff->SetPos(FPoint(0.0f, 0.0f));
				eff->Reset();
			}
			return false;
		}
	}
	else if(message.is("JumpFinished", _uid))
	{
        ReleaseSquares();
		Match3::RunFallColumn(GetIndex().x);
	}
	else if(message.is("Start"))
	{
		if(_order)
		{
			_order->InitAppear(100.f, 100.f);
		}
		ReleaseEffects();
		if(message.getData() == "from_tutorial")
		{
			_localTime = -GameSettings::gamefield[_index]->_flySquarePauseStore;
		}else{
			_localTime = -gameInfo.getConstFloat("CRISTAL_DELAY_APPEAR_ON_RECIVER", 0.f);
		}

		ParticleEffect *eff1 = _effContDown.AddEffect("RDown");
		eff1->SetPos(FPoint(0.0f, 0.0f));

		_partUp = _effContUpStatic.AddEffect("RUp");
		_partUp->SetPos(FPoint(0.0f, 0.0f));
		_partUp->Reset();

		_firstShowed = false;
		_lockLiveEffectShowed = false;
		_timeAppear = -0.2f;
		_timerForAppearOrder = -1.f;
		_state = ER_APPEAR;
		_hideByThief = false;
	}	
	return false;
}

void EnergyReceiver::HideByThief(float pause)
{
	Assert(!IsActivatedByEnergy());
	_hideByThief = true;
	_hideByThiefTime = -pause;
	ReleaseEffects();
}