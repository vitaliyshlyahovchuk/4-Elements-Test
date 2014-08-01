#include "stdafx.h"

#include "RoomGates.h"
#include "EditorUtils.h"
#include "Game.h"
#include "Energy.h"
#include "CellWalls.h"
#include "Match3.h"
#include "GameField.h"

namespace Gadgets
{
	RoomGates gates;
}

static Render::Texture *gatesTex = nullptr;

RoomGate::RoomGate(rapidxml::xml_node<> *xmlElem)
	: _local_time(0.0f)
	, _opened(false)
{
	_pos = IPoint(xmlElem);
	_dir = Xml::GetIntAttributeOrDef(xmlElem, "dir", 0);
	Assert(_dir >= 0 && _dir <= 3);

	rapidxml::xml_node<> *orderElem = xmlElem->first_node("Order");
	if( orderElem ) {
		_order = Game::Order::Load(orderElem);
		if(_order)
			_order->SetAddress( GetActivePos() );
	}

	InitCell();

	StartEffect();
}

RoomGate::RoomGate(IPoint pos, int direction)
	: _local_time(0.0f)
	, _pos(pos)
	, _opened(false)
	, _dir(direction)
{
	Assert(_dir >= 0 && _dir <= 3);

	StartEffect();
}

RoomGate::~RoomGate()
{
	KillEffect();
}

void RoomGate::Save(Xml::TiXmlElement *xmlElem)
{
	xmlElem->SetAttribute("x", _pos.x);
	xmlElem->SetAttribute("y", _pos.y);
	xmlElem->SetAttribute("dir", _dir);

	if(_order)
	{
		Xml::TiXmlElement *orderElem = xmlElem->InsertEndChild(Xml::TiXmlElement("Order"))->ToElement();
		_order->Save(orderElem);
	}
}

bool RoomGate::EnergyReady() const
{
	const IPoint off[] = { IPoint(1,1), IPoint(0,1), IPoint(-1,1), IPoint(-1,0), IPoint(-1,-1), IPoint(0,-1), IPoint(1,-1), IPoint(1,0), IPoint(1,1) };
	int idx = 2 * _dir + 1;
	for(int i = -1; i <= 1; ++i)
	{
		Game::FieldAddress fa(_pos + off[idx+i]);
		if( Energy::field.FullOfEnergy(fa) )
			return true;
	}
	return false;
}

void RoomGate::Update(float dt)
{
	if(!_opened)
	{
		_local_time += dt;

		if(_order) {
			_order->Update(dt);
			if(_order->Completed())
				_order.reset();
		}

		if(!_opened && !_order && EnergyReady())
		{
			FreeCell();
		}
	}

	if(_eff)
		_eff->Update(dt);
}

void RoomGate::Draw(Render::SpriteBatch *batch)
{
	if(!_opened)
	{
		float t = 0.5f + 0.5f * math::sin(3.0f * _local_time);
		const Color color(50, 230, 255, 255);

		Color col = math::lerp(color, Color::WHITE, t);

		Render::device.PushMatrix();
		Render::device.MatrixTranslate( GameSettings::CellCenter(_pos) );
		Render::device.MatrixRotate( math::Vector3::UnitZ, 90.0f * _dir);
		if(_eff)
			_eff->Draw(batch);
		Render::device.PopMatrix();
	}
}

void RoomGate::DrawOrder()
{
	if(!_opened && _order && !_order->Completed())
	{
		IPoint pos = GetActivePos() * GameSettings::SQUARE_SIDE;
		_order->Draw(pos, false);
	}
}

void RoomGate::StartEffect()
{
	if( !_eff )
	{
		_eff = ParticleEffectPtr(new ParticleEffect(effectPresets.getParticleEffect("GateArrow")));
		_eff->SetScale(GameSettings::SQUARE_SCALE);
		_eff->Reset();
	}
}

void RoomGate::KillEffect()
{
	if(_eff)
	{
		_eff->Kill();
		_eff.reset();
	}
}

void RoomGate::InitCell()
{
	_opened = false;
}

void RoomGate::FreeCell()
{
	_opened = true;

	Energy::field.UpdateSquare(Game::FieldAddress(_pos));
}

Game::Order::HardPtr* RoomGate::GetOrder()
{
	return &_order;
}

bool RoomGate::SetOrder(Game::Order::HardPtr order)
{
	bool result = (_order != nullptr);
	_order = order;
	return result;
}

IPoint RoomGate::GetActivePos() const
{
	const IPoint off[] = { IPoint(0,1), IPoint(-1,0), IPoint(0,-1), IPoint(1,0) };
	return _pos + off[_dir];
}

bool RoomGate::Opened() const
{
	return _opened;
}

bool RoomGate::IsOnScreen() const
{
	return Game::activeRect.Contains( GetActivePos() );
}

/****************************************************************/


RoomGates::RoomGates()
	: _dir(0)
{
}

void RoomGates::Clear()
{
	_gates.clear();
	_batch.reset();
}

void RoomGates::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear();

	_batch = Render::SpriteBatchPtr(new Render::SpriteBatch());

	if( !gatesTex )
		gatesTex = Core::resourceManager.Get<Render::Texture>("RoomGate");

	rapidxml::xml_node<> *gatesElem = root->first_node("RoomGates");
	if( !gatesElem )
		return;

	rapidxml::xml_node<> *gateElem = gatesElem->first_node("Gate");
	while( gateElem )
	{
		_gates.push_back( boost::make_shared<RoomGate>(gateElem) );
		gateElem = gateElem->next_sibling("Gate");
	}
}

void RoomGates::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *gatesElem = root->InsertEndChild(Xml::TiXmlElement("RoomGates"))->ToElement();
	for(RoomGate::HardPtr gate : _gates)
	{
		Xml::TiXmlElement *gateElem = gatesElem->InsertEndChild(Xml::TiXmlElement("Gate"))->ToElement();
		gate->Save(gateElem);
	}
}

RoomGate* RoomGates::GateOnCell(Game::FieldAddress fa) const
{
	for(RoomGate::HardPtr gate : _gates)
	{
		if(gate->GetPos() == fa.ToPoint())
			return gate.get();
	}
	return nullptr;
}

void RoomGates::Update(float dt)
{
	for(RoomGate::HardPtr gate : _gates)
	{
		gate->Update(dt);
	}
}

void RoomGates::Draw()
{
	if( !_gates.empty() )
	{
		Assert(_batch);
		_batch->Begin(Render::SpriteSortMode::Deferred, Render::SpriteTransformMode::Auto);
		for(RoomGate::HardPtr gate : _gates)
		{
			gate->Draw(_batch.get());
		}
		_batch->End();

		for(RoomGate::HardPtr gate : _gates)
		{
			gate->DrawOrder();
		}
	}
}

void RoomGates::DrawEdit()
{
	if(EditorUtils::activeEditBtn == EditorUtils::RoomGates)
	{
		Assert(_batch);

		IPoint mouse_pos = Core::mainInput.GetMousePos();
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);

		_batch->Begin(Render::SpriteSortMode::Deferred, Render::SpriteTransformMode::Auto);
		RoomGate gate(fa.ToPoint(), _dir);
		Render::BeginAlphaMul(0.3f);
		gate.Draw(_batch.get());
		Render::EndAlphaMul();
		_batch->End();
	}
}

bool RoomGates::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if(EditorUtils::activeEditBtn == EditorUtils::RoomGates)
	{
		if( !GateOnCell(sq->address) )
		{
			RoomGate::HardPtr gate = boost::make_shared<RoomGate>(sq->address.ToPoint(), _dir);
			gate->InitCell();
			_gates.push_back(gate);
			return true;
		}
	}
	return false;
}

bool RoomGates::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::None
		|| EditorUtils::activeEditBtn == EditorUtils::Null
		|| EditorUtils::activeEditBtn == EditorUtils::RoomGates
		|| EditorUtils::activeEditBtn == EditorUtils::PlaceOrder )
	{
		IPoint sq_pos = sq->address.ToPoint();
		for(auto itr = _gates.begin(); itr != _gates.end(); ++itr)
		{
			if((*itr)->GetPos() == sq_pos)
			{
				//Сначала удаляется заказ, затем сам замок, исключение - режим редактирования EditorUtils::Null. В этом режиме удаляется все подчистую.
				if( !(*itr)->SetOrder(Game::Order::HardPtr()) || EditorUtils::activeEditBtn == EditorUtils::Null)
				{
					(*itr)->FreeCell();
					_gates.erase(itr);
				}
				return true;
			}
		}
	}
	return false;
}

bool RoomGates::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
{
	return false;
}

bool RoomGates::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
{
	return false;
}

bool RoomGates::Editor_MouseWheel(int delta, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::RoomGates )
	{
		delta = math::clamp(-3, 3, delta);
		_dir = (_dir + delta + 4) % 4;
		return true;
	}
	return false;
}

Game::Order::HardPtr* RoomGates::Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq)
{
	RoomGate *gate = GateOnCell(sq->address);
	return gate ? gate->GetOrder() : nullptr;
}

bool RoomGates::IsGateCell(Game::FieldAddress fa) const
{
	return GateOnCell(fa) != nullptr;
}

BYTE RoomGates::EnergyWallsInCell(Game::FieldAddress fa) const
{
	RoomGate *gate_c = nullptr;
	RoomGate *gate_u = nullptr;
	RoomGate *gate_d = nullptr;
	RoomGate *gate_l = nullptr;
	RoomGate *gate_r = nullptr;
	for(RoomGate::HardPtr gate : _gates)
	{
		Game::FieldAddress pos(gate->GetPos());
		if(pos == fa)
			gate_c = gate->Opened() ? nullptr : gate.get();
		else if(pos == fa.Up())
			gate_u = gate->Opened() ? nullptr : gate.get();
		else if(pos == fa.Down())
			gate_d = gate->Opened() ? nullptr : gate.get();
		else if(pos == fa.Left())
			gate_l = gate->Opened() ? nullptr : gate.get();
		else if(pos == fa.Right())
			gate_r = gate->Opened() ? nullptr : gate.get();
	}

	BYTE result = 0;

	if( (gate_c && gate_c->GetDir() != 2) || (gate_u && gate_u->GetDir() != 0) )
		result |= Gadgets::CellWalls::UP;
	if( (gate_c && gate_c->GetDir() != 0) || (gate_d && gate_d->GetDir() != 2) )
		result |= Gadgets::CellWalls::DOWN;
	if( (gate_c && gate_c->GetDir() != 3) || (gate_l && gate_l->GetDir() != 1) )
		result |= Gadgets::CellWalls::LEFT;
	if( (gate_c && gate_c->GetDir() != 1) || (gate_r && gate_r->GetDir() != 3) )
		result |= Gadgets::CellWalls::RIGHT;

	return result;
}

void RoomGates::RemoveVisible()
{
	for(RoomGate::HardPtr gate : _gates)
	{
		if( gate->IsOnScreen() ) {
			gate->FreeCell();
		}
	}
}