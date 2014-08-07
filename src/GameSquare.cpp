#include "stdafx.h"
#include "Game.h"

#include "GameField.h"
#include "MyApplication.h"
#include "SquareNewInfo.h"
#include "GroundInfo.h"
#include "ChipsInfo.h"
#include "ActCounter.h"
#include "Match3Gadgets.h"
#include "SomeOperators.h"
#include "FieldStyles.h"
#include "Match3.h"
#include "SomeOperators.h"
#include "Match3Spirit.h"
#include "GameFieldControllers.h"
#include "LockBarriers.h"
#include "EffectWay.h"
#include "Energy.h"
#include "EnergyReceivers.h"
#include "PictureGenerator.h"
#include "GameInfo.h"

namespace Game {
	bool Game::Square::HAS_FLYING_SQUARES = false;

	Render::Texture * sandSquareTexture;
	bool Square::_hidingChipsOnLoseLife = false;
	
	Render::Texture* upElementsTexture = NULL;
	Render::Texture* squareIslandTexture = NULL;

	Render::Texture *treasureTexture;

	//Render::Texture *outCloudCircleTexture = NULL;
	//Render::Texture *outCloudElipceTexture = NULL;

	IRect Square::DRAW_RECT;

	Square *bufSquare = NULL;
	Square *bufSquareNoChip = NULL;
	Square *bufSquareNoLicorice = NULL;
	Square *bufSquareNoTimeBomb = NULL;
	Square *bufSquareNoBomb = NULL;

	float Square::SQUARE_START_EFFECT_WALL = 0.25f;
	float Square::SQUARE_HIDE_WALL = 0.5f;
	float Square::WALL_EFFECT_TO_FRONT_TIME = 0.2f;
	float Square::WALL_EFFECT_TO_FRONT_PAUSE = 0.4f;

	float Square::FLY_SQ_APPEAR_ALPHA_PART = 0.2f;
	float Square::FLY_SQ_HIDE_ALPHA_PART = 0.2f;
	float Square::FLY_SQ_DELAY_FOR_MOVE_BEFORE_LAST_HIDEN = 0.4f, Square::FLY_SQ_DELAY_FOR_APPEAR_AFTER_MOVE = 0.5f;

	float Square::FLY_SQ_TIME_AFTER_AND_BEFORE_SCALE, Square::FLY_SQ_TIME_APPEAR_EFFECT;

	float Square::FLY_SQ_CHIP_DELAY_APPEAR, Square::FLY_SQ_CHIP_DELAY_APPEAR_FIRST, Square::FLY_SQ_CHIP_DELAY_HIDE;

	float Square::FLY_SQ_APPEAR_CHIP_ALPHA_PART = 0.1f;

	void Square::InitGame() 
	{
		upElementsTexture = Core::resourceManager.Get<Render::Texture>("UpElements");

		treasureTexture = Core::resourceManager.Get<Render::Texture>("TreasureSquare");

		//outCloudCircleTexture = Core::resourceManager.Get<Render::Texture>("OutCloudCircle");
		//outCloudElipceTexture = Core::resourceManager.Get<Render::Texture>("OutCloudElips");

		Game::sandSquareTexture = Core::resourceManager.Get<Render::Texture>("SandPiece");
		Game::sandSquareTexture->setFilteringType(Render::Texture::BILINEAR);
		Game::sandSquareTexture->setAddressType(Render::Texture::REPEAT);

		squareIslandTexture = Core::resourceManager.Get<Render::Texture>("IslandSquare");

		Square::FLY_SQ_APPEAR_ALPHA_PART = gameInfo.getConstFloat("FLY_SQ_APPEAR_ALPHA_PART", 0.2f);
		Square::FLY_SQ_HIDE_ALPHA_PART = gameInfo.getConstFloat("FLY_SQ_HIDE_ALPHA_PART", 0.2f);

		Square::FLY_SQ_DELAY_FOR_MOVE_BEFORE_LAST_HIDEN = gameInfo.getConstFloat("FLY_SQ_DELAY_FOR_MOVE_BEFORE_LAST_HIDEN", 0.4f);
		Square::FLY_SQ_DELAY_FOR_APPEAR_AFTER_MOVE = gameInfo.getConstFloat("FLY_SQ_DELAY_FOR_APPEAR_AFTER_MOVE", 0.5f);

		Square::FLY_SQ_TIME_AFTER_AND_BEFORE_SCALE = gameInfo.getConstFloat("FLY_SQ_TIME_AFTER_AND_BEFORE_SCALE", 4.f);
		Square::FLY_SQ_TIME_APPEAR_EFFECT = gameInfo.getConstFloat("FLY_SQ_TIME_APPEAR_EFFECT", 0.2f);
		Square::FLY_SQ_CHIP_DELAY_APPEAR = gameInfo.getConstFloat("FLY_SQ_CHIP_DELAY_APPEAR", 0.5f);
		Square::FLY_SQ_CHIP_DELAY_APPEAR_FIRST = gameInfo.getConstFloat("FLY_SQ_CHIP_DELAY_APPEAR_FIRST", 0.5f);		
		Square::FLY_SQ_CHIP_DELAY_HIDE = gameInfo.getConstFloat("FLY_SQ_CHIP_DELAY_HIDE", 0.5f);
		Square::FLY_SQ_APPEAR_CHIP_ALPHA_PART = gameInfo.getConstFloat("FLY_SQ_APPEAR_CHIP_ALPHA_PART", 0.1f);

	}

	void GetUpElementRects(int type, FRect &rect, FRect &frect)
	{
		const float cell_w = 100.0f * GameSettings::SQUARE_SCALE;
		rect = FRect(0.f, cell_w, 0.f, cell_w).MovedTo(GameSettings::CELL_HALF - FPoint(cell_w*0.5f, cell_w*0.5f));
		if(type >= Game::UP_WOODSTAKE_0 && type <= Game::UP_WOODSTAKE_4) {
			rect.MoveBy(FPoint(0.f, Game::ChipColor::YOffset_WOOD[0]));
		}
		float uv_size_x = 0.2f;
		float uv_size_y = 0.2f;
		frect = FRect(0.f, uv_size_x, 0.f,  uv_size_y).MovedTo(FPoint(uv_size_x*float(type % 5), uv_size_y*float(type/5)));
	}

	//bool isBuffer(const Square *sq)
	//{
	//	return (sq == NULL) || (sq == bufSquare) || (sq == bufSquareNoChip) || (sq == bufSquareNoLicorice) || (sq == bufSquareNoTimeBomb) || (sq == bufSquareNoBomb);
	//}

void Square::Reset() 
{
	_flyChipOffset = FPoint(0.f, 0.f);
	_flyArrowAlpha = 0.f;
	_isOnActiveZone = false;
	_rectInIslandCollection.xStart = -1.f;
	_flySquarePause = 0.f;
	_flySquareTime = 0.f;
	_flySquareIslandAplha = 0.f;
	_flySquareAlpha = 0.f;
	 _flyChipAlpha = 0.f;
	_flyType = FLY_NO_DRAW;

	_isHangBuzy = false;

	_lockWithOrder.reset();
	checked = 0;
	_isNoChainOnStart = false;
	_wallRemovedWithSeqFromEnergy = false;
	
	_isFrontFreeChecked = false;
	_isEnergyChecked = false;
	_energyCheckTimer = 0.0f;

	_chip.Reset(true);
	_hangForChip = Hang();
	_isBusyCell = 0;
	_isBusyNear = 0;
	_wood = 0;
	_energy_wood = false;
	_growingWood = false;
	_forbid_bonus = false;
	_treasure = 0;

	portalEnergy = false;
	portalEnergyId = "";

	_fake = false;
	invisible = false;
	ice = 0;
	destroyed = false;
	destroyedTime = 0.f;
	destrEff = false;
	_stone = false;
	wasSeen = false;
	
	_iceDestroyTimer = 0.0f;

	_scale = 1.f;
	active = false;
	barrierIndex = -1;
	_random01 = math::random(0.0f, 1.0f);
	
	_wall = 0;
	_wallColor = 0;
	_isSand = false;
	_sandTime = 0.f;
	_sandHideFromDownToUp = false;

	_indestructible = false;
	_permanent = false;
	_cracked = false;
	_restoringWall = false;
	_growingWall = false;
	_growingWallFabric = false;
	_movesToRestore = 0;

	cellWallsChip = 0;

	_pos = FPoint(address.ToPoint()) * GameSettings::SQUARE_SIDEF;

	_bomb = 0;
	minPathTime = -1;

	_localTime = math::random(0.0f, 10.f);
	_is_short_square = false;

	_growTimer = 1.0f;
	_bgTexture = NULL;

	_toBeDestroyed = false;

	willFallChip = false;

	_killDiamond = false;

	_isCyclops = false;
	_current_cyclops_state = CYCLOPS_STATE_NONE;
	_cyclops_next_needed_state = CYCLOPS_STATE_NONE;
	_cyclops_change_state_timer = 0.f;
	_cyclops_current_action_timer = 0.f;
	_cyclops_current_action_duration = 0.f;

	KillTreasureEffect();
}

Square::Square(const FieldAddress& address_)
	: address(address_)
	, _chip(-1)
	, _flyType(FLY_NO_DRAW)
{
	Reset();
}

Square::~Square()
{
	KillTreasureEffect();
}

void Square::Init(const FieldAddress& address_)
{
	address = address_;
	Reset();
}

void Square::SetAddress(const FieldAddress& address_)
{
	address = address_;
	_pos.x = static_cast<float>(address.GetCol() * GameSettings::SQUARE_SIDE);
	_pos.y = static_cast<float>(address.GetRow() * GameSettings::SQUARE_SIDE);
}
//на клетке стоит что то что мешает сюда прилететь
bool Square::IsHardStand() const
{
	return IsStone() || GetWood() > 0 || (barrierIndex != -1) || _energy_wood || IsCyclops()
		|| Gadgets::receivers.IsReceiverCell(address) 
		|| Gadgets::lockBarriers.isLocked(address);
}

void Square::Update(float dt, bool isOnScreen, bool isOnActiveZone)
{
	_flySquareOffset.x = 0.f;
	_flyChipOffset = FPoint(0.f, 0.f);
	if(_flyType == FLY_NO_DRAW)
	{
		_flySquareTime += dt;
		if(isOnActiveZone)
		{
			Game::Square::HAS_FLYING_SQUARES = true;
		}
		_flyChipAlpha = 0.f;
		_flySquareAlpha = 0.f;
		_flySquareIslandAplha = 0.f;
		_flyArrowAlpha = 0.f;
	}else if(_flyType == FLY_APPEARING)
	{
		if(isOnActiveZone)
		{
			Game::Square::HAS_FLYING_SQUARES = true;
		}
		if(_flySquarePause > 0)
		{
			_flySquarePause -= dt;
			if(_flySquarePause <= 0)
			{
				GetChip().AddDistortion(boost::intrusive_ptr<ChipDistortionFlyAppear>(new ChipDistortionFlyAppear(-_flyTimerChip, _flyTimeFull)));				
			}
		}else{
			if(_flySquareTime < 1)
			{
				_flySquareTime += dt/_flyTimeFull;
				if(_flySquareTime > 1 - Square::FLY_SQ_TIME_APPEAR_EFFECT && !_flyEffectAppear)
				{
					_flyEffectAppear = true;
					//Эффект превращения острова в монолитное поле
					ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effUnderChipsCont, "IslandAfterAppear");
					eff->SetPos(_pos.x + GameSettings::CELL_HALF.x, _pos.y + GameSettings::CELL_HALF.y);
					eff->Reset();
				}
				if(_flySquareTime >= 1)
				{
					_flySquareTime = 1.f;
					GameField::Get()->levelBorder.Calculate();
					Energy::field.UpdateSquare(address);
					Match3::RunFallColumn(address.GetCol());
				}
			}else{
				_flyTimerAfter += dt*Square::FLY_SQ_TIME_AFTER_AND_BEFORE_SCALE;
			}
			if(_flyTimerChip < 1)
			{
				if(_flyTimerChip < 0)
				{
					_flyTimerChip += dt;
					if(_flyTimerChip >=0)
					{
					}
				}else{
					_flyTimerChip += dt/_flyTimeFullChip;
				}
				if(_flyTimerChip >= 1.f)
				{
					GetChip().AddDistortion(boost::intrusive_ptr<ChipDistortionFlyAppearStop>(new ChipDistortionFlyAppearStop(0.f, 0.5f)));
					_flyTimerAfter = 0.f;
					_flySquareTime = 0.f;
					_flyType = FLY_STAY;
					_flyChipAlpha = 1.f;
					_flySquareAlpha = 1.f;
					_flySquareIslandAplha = 0.f;
					_flyArrowAlpha = 1.f;
					return;
				}
			}
		}
		float rt = math::clamp(0.f, 1.f, _flySquareTime*_flyTimeFull); //Время полета острова
		float rt2 =  math::clamp(0.f, 1.f,_flyTimerChip*_flyTimeFullChip); //Время полета фишки

		_flySquareAlpha = math::clamp(0.f, 1.f, _flySquareTime/Game::Square::FLY_SQ_APPEAR_ALPHA_PART);
		_flyChipAlpha = math::clamp(0.f, 1.f, _flyTimerChip/Game::Square::FLY_SQ_APPEAR_ALPHA_PART);
		_flyArrowAlpha = _flySquareAlpha;
		_flySquareIslandAplha = math::clamp(0.f, 1.f, 1 - _flyTimerAfter)*_flySquareAlpha; //Исчезновение (переход в монолит)
				
		_flySquareOffset.y = _flyH - _flyStartV*rt - _flyG*rt*rt/2.f;
		_flyChipOffset.y = _flyH -_flyStartV*rt2 - _flyG*rt2*rt2/2.f;
	}else if(_flyType == FLY_STAY){
		_flySquareOffset.y = 0.f;
		_flySquareTime += dt;
		_flyChipAlpha = 1.f;
		_flySquareAlpha = 1.f;
		_flySquareIslandAplha = 0.f;
		_flyArrowAlpha = 1.f;
	}else if(_flyType == FLY_HIDING)
	{
		if(isOnActiveZone)
		{
			Game::Square::HAS_FLYING_SQUARES = true;
		}
		if(_flySquarePause > 0)
		{
			_flySquarePause -= dt;
			if(_flySquarePause <= 0)
			{
				//Эффект превращения поля в падающий остров
				ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effUnderChipsCont, "IslandBeforeHide");
				eff->SetPos(_pos.x + GameSettings::CELL_HALF.x, _pos.y + GameSettings::CELL_HALF.y);
				eff->Reset();
				Energy::field.SetEnergyFlowing(true);
				GameField::Get()->levelBorder.Calculate();
				Energy::field.UpdateSquare(address);
			}
		}else{
			if(_flySquareTimeBefore < 1)
			{
				_flySquareTimeBefore += dt*Square::FLY_SQ_TIME_AFTER_AND_BEFORE_SCALE;
				if(_flySquareTimeBefore >= 1)
				{
					_flySquareTimeBefore = 1.f;
					GameField::Get()->levelBorder.Calculate();
					Energy::field.UpdateSquare(address);
					GetChip().AddDistortion(boost::intrusive_ptr<ChipDistortionFlyHide>(new ChipDistortionFlyHide(-_flyTimerChip, _flyTimeFullChip)));
				}
			}else{
				_flySquareTime += dt/_flyTimeFull;
				if(_flySquareTime >= 2.f)
				{
					_flySquareTime = 2.f;
				}
				if(_flyTimerChip < 1)
				{
					if(_flyTimerChip < 0)
					{
						_flyTimerChip += dt;
					}else{
						_flyTimerChip += dt/_flyTimeFullChip;
					}
					if(_flyTimerChip >= 1.f)
					{
						_flyTimerAfter = 0.f;
						_flySquareTime = 0.f;
						_flyType = FLY_NO_DRAW;
						return;
					}
				}
			}

		}
		

		float rt = math::clamp(0.f, 1.f, _flySquareTime*_flyTimeFull);
		_flySquareOffset.x = 0.f;
		_flySquareOffset.y = -_flyStartV*rt - _flyG*rt*rt/2.f;

		//Падение фишки
		float rt2 = math::clamp(0.f, 1.f, _flyTimerChip*_flyTimeFullChip);
		_flyChipOffset.y = -_flyStartVChip*rt2 - _flyG*rt2*rt2/2.f;
			
		_flySquareAlpha = math::clamp(0.f, 1.f, 1 - (_flySquareTime - 1.f)/0.5f);
		_flySquareIslandAplha = _flySquareTimeBefore*_flySquareAlpha;
		_flyChipAlpha = math::clamp(0.f, 1.f, (1-_flyTimerChip)/Game::Square::FLY_SQ_HIDE_ALPHA_PART);
		_flyArrowAlpha = math::clamp(0.f, 1.f, 1 - (_flySquareTime - 1 + Game::Square::FLY_SQ_HIDE_ALPHA_PART)/Game::Square::FLY_SQ_HIDE_ALPHA_PART);
		//return;
	}

	if(_energyCheckTimer > 0.0f)
	{
		_energyCheckTimer -= dt;
		if(_energyCheckTimer < 0.0f) {
			_energyCheckTimer = 0.0f;
		}
	}
	_localTime += dt;
	_chip.Update(dt, address, isOnScreen, isOnActiveZone);

	_isOnActiveZone = isOnActiveZone;

	if(_chip.checkFall)
	{
		_chip.checkFall = false;
		Match3::RunFallColumn(address.GetCol()-1);
		Match3::RunFallColumn(address.GetCol());
		Match3::RunFallColumn(address.GetCol()+1);
	}

	active = false;

	if (_hidingChipsOnLoseLife) {
		_scale -= dt * (0.5f + 1.5f * _random01);
		if (_scale < 0.0f) {
			_scale = 0.0f;
		}
	}

	if( Energy::field.EnergyExists(address) )
	{
		if(_chip.IsStarAct())
		{
			_chip.BreakStarArrow();
			_chip.SetInfo(Gadgets::InfoC(), IPoint(-1, -1));
			SetBusyCell(1);
			Match3::RunFallColumn(address.GetCol());
		}

		if(_chip.GetTreasure() > 0 && Energy::field.FullOfEnergy(address))
		{
			const std::string effNames[] = {"DiggedCoin", "DiggedBag", "DiggedChest" };
			int score = GameSettings::score.treasure[_chip.GetTreasure()-1];
			GameField::Get()->RunScore(address, score, 0.1f, 1.1f, Color::WHITE, 1.2f, effNames[_chip.GetTreasure()-1]);

			_chip.SetInfo(Gadgets::InfoC(), IPoint(-1, -1));
			SetBusyCell(1);
			Match3::RunFallColumn(address.GetCol());
		}
	}

	if(_killDiamond && _chip.GetType() == ChipColor::DIAMOND && _chip.IsStand() )
	{
		_chip.SetInfo(Gadgets::InfoC(), IPoint(-1, -1));
		SetBusyCell(1);
		Game::diamondsFound++;
		Game::diamondsOnScreen--;
		Game::Orders::KillCell(Game::Order::DIAMOND, address);
		Game::UpdateDiamondsObjective();
		Match3::RunFallColumn(address.GetCol());
	}

	CheckHangForChip();

	if(_chip.IsEnergyBonus() && _chip.IsStand() && Energy::field.EnergyExists(address) && GameField::Get()->IsStandby())
	{
		_chip.RunBonus(address);
	}

	if(portalEnergy)
	{
		if(Energy::field.EnergyExists(address))
		{
			std::vector<Game::Square*> vec;
			GameSettings::GetOtherPortalsSquares(address, vec);
			size_t count = vec.size();
			for(size_t i = 0; i < count; i++)
			{
				vec[i]->AcivatePortal();
			}
			portalEnergy = false;
		}
	}

	//Апдейт появления земли (растущая земля)
	if(_growTimer < 1.0f)
	{
		_growTimer += 1.5f * dt;
	}

	//Апдейт рузрушения земли на клетке
	if (destroyed)
	{
		destroyedTime += dt;
		Game::has_destroying = true;
		if (destroyedTime >= Game::Square::SQUARE_START_EFFECT_WALL && !destrEff)
		{
			destrEff = true;
//				MyAssert(GetWood() == 0);    почему то иногда тут вылетало, решили пока закомментировать
			if(GetWall() > 0)
			{
				//Убираем насыпь
				Color col;

				if (barrierIndex != -1)
				{
					// Еcли уничтожаетcя cтена внутри головоломки, то это 
					// подложка головоломки. Задаём цвет, похожий на подложку

					col = Color(124, 121, 106);
				}
				else
				{
					switch (GetWall())
					{
						case 1: { col = FieldStyles::current->bgBottomColor; break; }
						case 2: { col = FieldStyles::current->bgMiddleColor; break; }
						case 3: { col = FieldStyles::current->bgTopColor; break; }
					}
				}
				ParticleEffectPtr eff = NULL;
				Utils::EffectDrawer* _contr = new Utils::EffectDrawer("ground_break" + utils::lexical_cast(math::random(1, 4)), eff, (address.GetCol() + address.GetRow())%2 == 1 ? Color(237, 237, 237): Color::WHITE);
				if(eff)
				{
					_contr->SetDrawType(GDP_SMOOTH, Game::Square::WALL_EFFECT_TO_FRONT_TIME, Game::Square::WALL_EFFECT_TO_FRONT_PAUSE);
					eff->SetPos(_pos + GameSettings::CELL_HALF);
					eff->Reset();
					Game::AddController(_contr);
				}

			}
		}

		if (destroyedTime >= SQUARE_HIDE_WALL)
		{
			destroyedTime = SQUARE_HIDE_WALL;
			destroyed = false;

			if(GetWall() > 0 && !_cracked)
			{
				DecWall();
			}
			if(GetWall() == 0)
			{
				//Нужно пускать энергию
				Energy::field.UpdateSquare(address);
			}
		}
	}

	//Обновляем разрушение льда
	if(_iceDestroyTimer < 0)
	{
		_iceDestroyTimer += dt;
		if(_iceDestroyTimer >= 0)
		{
			Match3::RunFallColumn(address.GetCol()-1);
			Match3::RunFallColumn(address.GetCol());
			Match3::RunFallColumn(address.GetCol()+1);
		}
	}

	if(_isSand && _sandTime < 1)
	{
		_sandTime += dt*4.f/1.45f;
		if(_sandTime >= 1.f)
		{
			_sandTime = 1.f;
		}
	}else if(!_isSand && _sandTime > 0)
	{
		_sandTime -= dt*4.f/1.45f;
		if(_sandTime <= 0)
		{
			_sandTime = 0.f;
			if(CanEnergy())
			{
				Energy::field.UpdateSquare(address);
			}
		}
	}
	if(_isSand && _sandTime == 1 && Game::chipsStandby && !Game::has_destroying)
	{
		static const IPoint sand_dirs[3] = {IPoint(0, -1), IPoint(-1, -1), IPoint(1, -1)};
		for(BYTE k = 0; k < 3; k++)
		{
			IPoint next = address.ToPoint() + sand_dirs[k];
			Game::Square *sq1 = GameSettings::gamefield[next];
			if( Game::isVisible(sq1) && !sq1->IsSand() && sq1->GetWall() == 0 && !sq1->IsFake() && !Gadgets::square_new_info.IsEnergySourceSquare(next) && sq1->GetSandTime() == 0){
				if(k > 0)
				{	//Диагональные падения возможныт только если над клетками назначения нет песка
					Game::Square *sq2 = GameSettings::gamefield[next + IPoint(0, 1)];
					if( Game::isVisible(sq2) && sq2->IsSand())
					{
						break;
					}
				}
				DecWall();
				_sandHideFromDownToUp = false;
				//ParticleEffect* eff = NULL;
				if(k == 0)
				{
					//eff = GameField::Get()->_effContUpSquare.AddEffect("SandDown");
					_sandHideFromDownToUp = true;
				}else if(k == 2)
				{
					//eff = GameField::Get()->_effContUpSquare.AddEffect("SandDownRight");
				}
				//if(eff)
				//{
				//	eff->SetPos(FPoint(_pos.x, _pos.y) + GameSettings::CELL_HALF);
				//	eff->Reset();
				//	eff->SetScale(GameSettings::SQUARE_SIDEF/128.f);
				//}
				sq1->SetSand(true);	
				Energy::field.UpdateSquare(sq1->address);
				break;
			}					
		}
		Energy::field.UpdateSquare(address);
	}

	if (IsCyclops())
	{
		UpdateCyclops(dt);
	}

}

void Square::UpdateCyclops(float dt)
{
	if (_cyclops_change_state_timer > 0.f)
	{
		_cyclops_change_state_timer -= dt;
		if (_cyclops_change_state_timer <= 0.f)
		{
			ChangeCyclopsState(_cyclops_next_needed_state);
		}
	}

	switch (_current_cyclops_state)
	{
	case CYCLOPS_STATE_LOOK_LEFT:
		PlayLookLeft(dt);
		break;
	case CYCLOPS_STATE_LOOK_RIGHT:
		PlayLookRight(dt);
		break;
	case CYCLOPS_STATE_LOOK_LEFT_RIGHT:
		PlayLookLeftRight(dt);
		break;
	case CYCLOPS_STATE_LOOK_RIGHT_LEFT:
		PlayLookRightLeft(dt);
		break;
	}
}

void Square::UpdateCyclopsAction(float dt)
{
	if (_cyclops_current_action_timer > 0.f)
	{
		_cyclops_current_action_timer -= dt;
		if (_cyclops_current_action_timer <= 0.f)
			_cyclops_current_action_timer = 0.f;
	}
}

void Square::ChangeCyclopsState(int new_state)
{
	const float min_interval = 2.5f;
	const float max_interval = 5.f;
	switch (new_state)
	{
	case CYCLOPS_STATE_NONE:
	{

	}
		break;
	case CYCLOPS_STATE_GROW:
	{
		_cyclops_change_state_timer = math::random(min_interval, max_interval);
		_cyclops_next_needed_state = math::random(CYCLOPS_STATE_BLINK, CYCLOPS_STATE_LOOK_LEFT_RIGHT);
		GetChip().AddDistortion(boost::intrusive_ptr<ChipAppearFromGround>(new ChipAppearFromGround(0.f, 1.2f)));
	}
		break;
	case CYCLOPS_STATE_APPEAR_WITHOUT_GROW:
	{
		_cyclops_change_state_timer = math::random(min_interval, max_interval);
		_cyclops_next_needed_state = math::random(CYCLOPS_STATE_BLINK, CYCLOPS_STATE_LOOK_LEFT_RIGHT);
	}
		break;
	case CYCLOPS_STATE_BLINK:
	{
		_cyclops_change_state_timer = math::random(min_interval, max_interval);
		_cyclops_next_needed_state = math::random(CYCLOPS_STATE_BLINK, CYCLOPS_STATE_LOOK_LEFT_RIGHT);
		if (IsStandbyState())
			Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["choc_blink"], _pos + FPoint(11.f, 158.f) * GameSettings::SQUARE_SCALE));
	}
		break;
	case CYCLOPS_STATE_LOOK_LEFT:
	case CYCLOPS_STATE_LOOK_RIGHT:
	case CYCLOPS_STATE_LOOK_LEFT_RIGHT :
	case CYCLOPS_STATE_LOOK_RIGHT_LEFT :
	{
		_cyclops_change_state_timer = math::random(min_interval, max_interval);
		_cyclops_current_action_duration = _cyclops_current_action_timer = math::random(1.f, 2.f);
		_cyclops_next_needed_state = math::random(CYCLOPS_STATE_BLINK, CYCLOPS_STATE_LOOK_LEFT_RIGHT);
	}
		break;
	}
	_current_cyclops_state = (CyclopsState)new_state;
}

void Square::PlayLookLeft(float dt)
{
	UpdateCyclopsAction(dt);
	_eye_offset_delta.x = -math::sin((1.f / _cyclops_current_action_duration) * _cyclops_current_action_timer * math::PI) * 3.f;
}

void Square::PlayLookRight(float dt)
{
	UpdateCyclopsAction(dt);
	_eye_offset_delta.x = math::sin((1.f / _cyclops_current_action_duration) * _cyclops_current_action_timer * math::PI) * 3.f;
}

void Square::PlayLookRightLeft(float dt)
{
	UpdateCyclopsAction(dt);
	_eye_offset_delta.x = -math::sin((1.f / _cyclops_current_action_duration) * _cyclops_current_action_timer * math::PI * 2.f) * 3.f;
}

void Square::PlayLookLeftRight(float dt)
{
	UpdateCyclopsAction(dt);
	_eye_offset_delta.x = math::sin((1.f / _cyclops_current_action_duration) * _cyclops_current_action_timer * math::PI * 2.f) * 3.f;
}

void Square::CheckHangForChip()
{
	// в клетке появилась фишка и клетке был назначен бонус - вешаем этот бонус на фишку
	if(_chip.IsExist() && !_hangForChip.IsEmpty() && !_chip.IsFalling())
	{
		//Assert2(!_chip.HasHang(), "Trying to hang bonus on chip: chip already has bonus");

		_chip.SetHang(_hangForChip);

		if (_hangForChip.autoRun) {
			_chip.KillChip(Game::GetCenterPosition(address.ToPoint()), address, false, false, 0.f);
			SetBusyCell(1);
		}
		_hangForChip.Clear();

		// в момент навешивания бонуса может быть уже выделена какая-то цепочка, тогда нужно
		// будет пересчитать каскад бонусов, чтобы включить туда и свежеповешенный
		GameField::Get()->CalcSequenceEffects();

		// agushin: если перед уровнем выбрали суперфишку и "убрать цвет", после пересчёта сломается
		// подсвечивание буста "убрать цвет", чиним:
		if (!GameField::Get()->_currentActiveBoost.expired()) {
			BoostList::HardPtr currentActiveBoost = GameField::Get()->_currentActiveBoost.lock();
			currentActiveBoost->AcceptMessage(Message("UpdateAffectedCells"));
		}
		
	}
}

void Square::AcivatePortal()
{
	Energy::field.StartEnergyInSquare(address);
	portalEnergy = false;
	//portalEnergyId = "";
}

bool Square::IsEnergyWall() const
{
	return _wallRemovedWithSeqFromEnergy;
}

void Square::SetEnergyWall(const bool energy_wall)
{
	_wallRemovedWithSeqFromEnergy = energy_wall;
}

bool Square::IsGrowingWall() const
{
	return _growingWall;
}

bool Square::IsGrowingWallFabric() const
{
	return _growingWallFabric;
}

void Square::SetGrowingWall(bool is_growing, bool is_fabric)
{
	_growingWall = (_wall > 0) && is_growing;
	_growingWallFabric = _growingWall && is_fabric;
}

void Square::KillTreasureEffect()
{
	if(_treasureEffBack)
		_treasureEffBack->Kill();
	if(_treasureEffFront)
		_treasureEffFront->Kill();
	_treasureEffBack.reset();
	_treasureEffFront.reset();
}

void Square::SetInfo(const Gadgets::InfoSquare &info)
{
	//Сброс зависимых переменных
	portalEnergy = false;
	if(_is_short_square)
	{
		_fake = false;
		_is_short_square = false;
	}
	_wallRemovedWithSeqFromEnergy = info.wallRemovedWithSeqFromEnergy;
	if(_wallRemovedWithSeqFromEnergy && GetWall() == 0)
	{
		SetWall(1);
	}
	if(info.en_source > 0)
	{
		SetWall(0);
	}

	_growingWall = info.growingWall;
	_growingWallFabric = info.growingWall_fabric;
	if(_growingWall && GetWall() == 0)
	{
		SetWall(1);
	}

	//Устанавливаем новую инфу
	_forbid_bonus = info.forbid_bonus;
	if(!info.portal.empty())
	{
		portalEnergy = true; 
		portalEnergyId = info.portal;
		portalEnergyColor = Color(portalEnergyId);
	}
	else if(info.is_short_square)
	{
		_is_short_square = true;
		_fake = true;
	}
	_isNoChainOnStart = info.isNoChainOnStart;

	_treasure = info.treasure;
	if(_treasure > 0)
	{
		if(!_treasureEffBack) {
			_treasureEffBack = Game::AddEffect(GameField::Get()->_effUnderChipsCont, "TreasureBack");
			_treasureEffBack->SetPos(_pos + GameSettings::CELL_HALF);
			_treasureEffBack->Reset();
		}
		if(!_treasureEffFront) {
			_treasureEffFront = Game::AddEffect(GameField::Get()->_effCont, "TreasureFront");
			_treasureEffFront->SetPos(_pos + GameSettings::CELL_HALF);
			_treasureEffFront->Reset();
		}
	}
	else if(_treasure == 0 )
	{
		KillTreasureEffect();
	}
	_killDiamond = info.kill_diamonds;
}

bool Square::IsNormal() const
{
	if (IsStone())
	{
		return false;
	} 
	if (_chip.GetType() != ChipColor::CHIP)
	{
		return false;
	}
	if(_wood > 0)
	{
		return false;
	}
	if(_energy_wood)
	{
		return false;
	}
	if (IsCyclops())
	{
		return false;
	}
	return true;
}

bool Square::IsStandbyState() const
{
	return !destroyed && (_iceDestroyTimer >= 0.0f) && _chip.IsStand();
}

bool Square::IsIce() const
{
	return (ice > 0) || (_iceDestroyTimer < 0.0f);
}

bool Square::IsCyclops() const
{
	return _isCyclops;
}

ChipColor& Square::GetChip() 
{
	return _chip;
}

bool Square::IsColor(const int &color)
{
	if(IsHardStand())
	{
		return false;
	}
	return _chip.IsColor(color);
}

bool Square::IsColor(const ChipColor &other)
{
	return _chip.IsColor(other.GetColor());
}

FPoint Square::GetChipPos() const
{
	return GetCellPos() + _chip.GetPos();
}

FPoint Square::GetCellPos() const
{
	return _pos + _flySquareOffset;
}

void Square::KillWall(const float &pause, const bool &it_is_bomb)
{
	int wall_in_square = GetWall();
	if(wall_in_square <= 0)
	{
		return;
	}

	if(_indestructible && !it_is_bomb)
		return;

	if(_growingWallFabric || _permanent || GameField::Get()->GetLevelObjective() == LevelObjective::DIAMONDS)
		return;

	if(_restoringWall)
	{
		_cracked = !_cracked;
		if(_cracked)
			_movesToRestore = 1;
	}

	if(it_is_bomb && !_cracked)
	{
        /*
		Color col;
		switch (GetWall())
		{
			case 1: { col = FieldStyles::current->bgBottomColor; break; }
			case 2: { col = FieldStyles::current->bgMiddleColor; break; }
			case 3: { col = FieldStyles::current->bgTopColor; break; }
		}
		
		ParticleEffectPtr eff = NULL;
		Utils::EffectDrawer* _contr = new Utils::EffectDrawer("ground_break" + utils::lexical_cast(math::random(1, 3)), eff, (address.GetCol() + address.GetRow())%2 == 1 ? Color(237, 237, 237): Color::WHITE);
		if(eff)
		{
			_contr->SetDrawType(GDP_SMOOTH, Game::Square::WALL_EFFECT_TO_FRONT_TIME, Game::Square::WALL_EFFECT_TO_FRONT_PAUSE);						
			eff->SetPos(_pos + GameSettings::CELL_HALF);
			eff->Reset();
			Game::AddController(_contr);
		}

		DecWall();	
		if(CanEnergy())
		{
			//Нужно пускать энергию
			Energy::field.UpdateSquare(address);
		}*/
        
		if(destroyed && wall_in_square > 1)
		{
			SetWall(wall_in_square-1);
		}
		destroyed = true;
		destroyedTime = -pause;
		destrEff = false;
        
	} else {
		if(destroyed && wall_in_square > 1)
		{
			SetWall(wall_in_square-1);
		}
		destroyed = true;
		destroyedTime = -pause;
		destrEff = false;
	}
}
bool Square::IsStone() const
{
	return _stone;
}

void Square::SetStone(bool stone)
{
	_stone = stone;
	_stone_index = math::random(2);
}

bool Square::KillSquareNear(float pause_color)
{
	if(GetWood() > 0)
	{
		//если частокол был восстанавливающийся
		if (IsGrowingWood())
		{
			GameSettings::need_inc_growing_wood = 0; //в этот раз его не ростим
		}
		//Убираем окаменелость
		FPoint offset = FPoint(40.f, 20.f)*GameSettings::SQUARE_SCALE + FPoint(0.f, Game::ChipColor::YOffset_WOOD[GetWood()-1]);
		Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["kol_"+utils::lexical_cast(std::min(2,_wood))], _pos + offset));

		MM::manager.PlaySample("ClearWood");

		_wood--;

		float scoreScale = _goldWood ? 1.1f : 1.0f;
		Color scoreColor = _goldWood ? Color(255, 210, 30) : Color::WHITE;
		float duration = _goldWood ? 1.8f : 1.0f;
		int scoreIdx = (GetWood() > 0) ? 0 : 1;
		int score = _goldWood ? GameSettings::score.gold_wood[scoreIdx] : GameSettings::score.wood[scoreIdx];
		GameField::Get()->RunScore(address, score, _goldWood ? 0.5f : 0.0f, scoreScale, scoreColor, duration);

		if(GetWood() == 0)
		{
			_goldWood = false;
			//Фишке нужно упасть на место освободившейся земли
			_fake = false;
			Game::Orders::KillCell(Game::Order::GROUND, address);
		} else {
			//_fake = true;
		}
		return true;
	}
	if (IsCyclops())
	{
		Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["choc_0"], _pos + FPoint(40.f, 40.f) * GameSettings::SQUARE_SCALE));
		SetCyclops(false);
		_chip.Reset(true);
		Game::Orders::KillCell(Game::Order::GROUND_CYCLOPS, address);
		GameField::Get()->AddScore(address, GameSettings::score.ground_cyclops);
		GameSettings::need_inc_ground_cycops = false;

		return true;
	}

	return _chip.KillChip(GetCellPos() + GameSettings::CELL_HALF, address, false, true, pause_color);
}


void Square::KillSquareNearEnergy()
{
	// Убираем частокол
	if(_energy_wood )
	{
		ParticleEffectPtr eff = NULL;
		Utils::EffectDrawer* _contr = new Utils::EffectDrawer("wood_break_1to0", eff, Color::WHITE);
		if(eff)
		{
			_contr->SetDrawType(GDP_SMOOTH, Game::Square::WALL_EFFECT_TO_FRONT_TIME, Game::Square::WALL_EFFECT_TO_FRONT_PAUSE);						
			eff->SetPos(_pos + GameSettings::CELL_HALF);
			eff->Reset();
			Game::AddController(_contr);
		}

		_energy_wood = false;
		_fake = false;

		Match3::RunFallColumn(address.GetCol()-1);
		Match3::RunFallColumn(address.GetCol());
		Match3::RunFallColumn(address.GetCol()+1);
	}
}

bool Square::KillSquareFull(bool can_clear_stone, bool it_is_bonus, float pause_color, bool chip_is_kill, ColorMask killColors)
{
	//Assert( !IsBusyCell() );   СШ: может нарушаться при бусте. проблема в том что видимо невовремя сбрасывается IsBusyCell

	if(_treasure > 0 && chip_is_kill)
	{
		Assert(_treasure <= 3);
		const std::string effNames[] = {"DiggedCoin", "DiggedBag", "DiggedChest" };
		int score = GameSettings::score.treasure[_treasure-1];
		GameField::Get()->RunScore(address, score, 1.1f, 1.1f, Color::WHITE, 1.2f, effNames[_treasure-1]);
		_treasure = 0;
		KillTreasureEffect();
	}

	if(!IsBusyNear() || it_is_bonus)
	{
		if( KillSquareNear(pause_color) )
		{
			return true;
		}
	}
	if(IsStone())
	{
		if(can_clear_stone)
		{
			//Нужно убить камень!!!
			_stone = false;
			//SetFake(false);
			FPoint pos = GetCellPos() + FPoint(40.f, 20.f) * GameSettings::SQUARE_SCALE + FPoint(0.f, Game::ChipColor::YOffset_WOOD[_stone_index]);
			Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["stone_" + utils::lexical_cast(_stone_index)], pos));

			GameField::Get()->AddScore(address, GameSettings::score.stone);

			MM::manager.PlaySample("ClearStone");
		}
		return true;
	}
	if(IsIce()) //убиваем лед
	{
		MyAssert(!IsFake());
		
		ParticleEffectPtr eff = NULL;
		Utils::EffectDrawer* _contr = new Utils::EffectDrawer("ice_break", eff, Color::WHITE);
		if(eff)
		{
			_contr->SetDrawType(GDP_SMOOTH, Game::Square::WALL_EFFECT_TO_FRONT_TIME, Game::Square::WALL_EFFECT_TO_FRONT_PAUSE);
			eff->SetPos(_pos + GameSettings::CELL_HALF);
			eff->Reset();
			Game::AddController(_contr);
		}

		float pause = 0.f;
		_iceDestroyTimer = -pause;

		MM::manager.PlaySample("ClearIce");
		ice = 0;

		return true;
	}
	if(GetWall() > 0)
	{
		bool can_dec_wall = false;
		if(_wallRemovedWithSeqFromEnergy){
			can_dec_wall = GameSettings::sequenceWithEnergySquare;
		} else {
			can_dec_wall = (GetWallColor() == 0) || ColorInMask(killColors, GetWallColor());
		}
		if(can_dec_wall)
		{
			//Землю убираем когда убрали фишку
			if (Game::timeFromLastBreakingWallSound > 0.1f)
			{
				MM::manager.PlaySample("ClearEarth");
				Game::timeFromLastBreakingWallSound = 0.f;
			}

			if(IsGrowingWall())
				GameSettings::need_inc_wall_squares = 0;

			KillWall(pause_color, it_is_bonus);
		}
		return false;
	}
	else
	{
		Energy::field.UpdateSquare(address);
	}
	return false;
}


void DrawPortal(const float &time, const math::Vector3 &pos, const bool &reverse, const Color &color)
{
	Render::device.SetTexturing(false);
	Render::BeginColor(color);

	for(size_t i = 0; i < 5; i++)
	{
		Render::device.PushMatrix();
		float t = math::clamp(0.f, 1.f, (1+sinf(time*math::PI*2.f + i/5.f*math::PI))*0.5f);
		//t *= 1.3f;
		Render::device.MatrixTranslate(pos + GameSettings::CELL_HALF);
		Render::device.MatrixScale(t);
		Render::device.MatrixTranslate(-GameSettings::CELL_HALF);
		Render::DrawFrame(Square::DRAW_RECT);
		Render::device.PopMatrix();
	}
	Render::EndColor();

	Render::device.SetTexturing(true);
}

void Square::DrawEditor(Render::SpriteBatch *batch) 
{
	if(IsShortSquare())
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(234, 234, 123, 255));
		FPoint p(GetCellPos());
		//Рисуем типа "песочные часы"
		const float size = (float)GameSettings::SQUARE_SIDE;
		Render::DrawLine(p, p + FPoint(size, 0.f));
		Render::DrawLine(p, p + FPoint(size, size));
		Render::DrawLine(p + FPoint(size, 0.f), p + FPoint(0.f, size));
		Render::DrawLine(p + FPoint(0.f, size), p + FPoint(size, size));
		Render::EndColor();
		Render::device.SetTexturing(true);
		return;
	}
	if(_forbid_bonus)
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color::RED);
		FPoint p = GetCellPos() + FPoint(10.0f, 10.0f);
		const float size = (float)GameSettings::SQUARE_SIDE - 20.0f;
		Render::DrawLine(p, p + FPoint(size, size));
		Render::DrawLine(p + FPoint(size, 0.f), p + FPoint(0.f, size));
		Render::EndColor();
		Render::device.SetTexturing(true);
	}
}


void Square::DrawSand(const Color &color, const IRect &rect_texture)
{
	if(_sandTime > 0)
	{
		FRect frect, frect2;

		float t1 = math::clamp(0.f, 1.f, (_sandTime - 0.2f)/0.8f);
		float t2 =  math::clamp(0.f, 1.f, _sandTime/0.8f);
		if(_sandHideFromDownToUp)
		{
			frect = FRect(0.f, 1.f, 0.f, 1.f - t2);
			frect2 = FRect(0.f, 1.f, 1.f - t1, 1.f - t2);
		}else{
			frect = FRect(0.f, 1.f, t2, 1.f);		
			frect2 = FRect(0.f, 1.f, t1, t2);
		}

		//float t1 = math::clamp(0.f, 1.f, (_sandTime - 0.2f)/0.8f);
		//float t2 =  math::clamp(0.f, 1.f, _sandTime/0.8f);
		//t1 *= 1.4f; t1 -= 0.2f; 
		//t2 *= 1.4f; t2 -= 0.2f; 
		//if(_sandHideFromDownToUp)
		//{
		//	frect = FRect(0.f, 1.f, -0.2f, 1.f - t2);
		//	frect2 = FRect(0.f, 1.f, 1.f - t1, 1.f - t2);
		//}else{
		//	frect = FRect(0.f, 1.f, t2, 1.2f);		
		//	frect2 = FRect(0.f, 1.f, t1, t2);
		//}

		Render::BeginColor(color);
		if(true)
		{
			FRect rect = frect.Scaled(GameSettings::SQUARE_SIDEF).MovedBy(FPoint(_pos.x, _pos.y));
			frect = rect / rect_texture;
			Render::DrawRect(rect, frect);

			FRect rect2 = frect2.Scaled(GameSettings::SQUARE_SIDEF).MovedBy(FPoint(_pos.x, _pos.y));
			frect2 = rect2 / rect_texture;
			Render::DrawQuad(
				math::Vector3(rect2.xStart, rect2.yStart, 0.f), 
				math::Vector3(rect2.xEnd, rect2.yStart, 0.f), 
				math::Vector3(rect2.xStart, rect2.yEnd, 0.f), 
				math::Vector3(rect2.xEnd, rect2.yEnd, 0.f), 
				Color::BLACK_TRANSPARENT, Color::BLACK_TRANSPARENT, Color::WHITE, Color::WHITE,
				frect2.xStart, frect2.xEnd, frect2.yStart, frect2.yEnd);
		}
		Render::EndColor();

	}
}

void Square::DrawSquare()
{
	// порталы
	if(portalEnergy)
	{
		DrawPortal(_localTime*0.3f, GetCellPos(), true, portalEnergyColor);
	}

	// "двухступенчатая" земля
	if(IsRestoring())
	{
		Render::BeginAlphaMul(0.5f);
		Render::FreeType::BindFont("debug");
		Render::PrintString(GetCellPos().x, GetCellPos().y, "2", 1.5f, LeftAlign, BottomAlign, false);
		Render::EndAlphaMul();
	}

	if(_treasure > 0)
	{
		FRect rect = FRect(GameSettings::CELL_RECT).MovedTo(GetCellPos());
		rect.Inflate(-4.f);
		treasureTexture->Draw(rect, FRect(0,1,0,1));
	}
}

void Square::DrawChip(int layer, Render::SpriteBatch *batch)
{
	// layer 0 - обычные фишки, колышки, монстрики
	// layer 1 - фишки с бонусами
	// layer 2 - летящие фишки
	// layer 3 - туториальный слой
	
	if( layer == 0 )
	{
		if(_lockWithOrder)
		{
			//Todo: как избежать Flush батча пока не знаю
			//_lockWithOrder - рисуется в виде партиклового эффекта
			if(batch)
			{
				batch->Flush();
			}
			_lockWithOrder->Draw();
		}
		if (IsStone()) 
		{
			FRect rect, frect;
			GetUpElementRects(Game::UP_STONE_0 + _stone_index, rect, frect);
			rect.MoveBy(GetCellPos());
			if(batch)
			{
				batch->Draw(upElementsTexture, Render::ALPHA, rect.MovedBy(FPoint(0.f, ChipColor::YOffset_STONE)), frect);
			}else{
				upElementsTexture->Draw(rect.MovedBy(FPoint(0.f, ChipColor::YOffset_STONE)), frect);
			}
			return;
		}

		// рисуем деревянные столбы
		if(_wood > 0)
		{
			FRect rect, frect;
			if (IsGrowingWood()) //выбираем картинку в зависимости растущий частокол или нет
			{
				GetUpElementRects(Game::UP_WOODSTAKE_GROWING_0 + _wood - 1, rect, frect);
			}
			else if(IsGoldWood())
			{
				GetUpElementRects(Game::UP_WOODSTAKE_GOLD_0 + _wood - 1, rect, frect);
			}
			else
			{
				GetUpElementRects(Game::UP_WOODSTAKE_0 + _wood - 1, rect, frect);
			}
			rect.MoveBy(GetCellPos());
			if(batch)
			{
				batch->Draw(upElementsTexture, Render::ALPHA, rect, frect);
			}else{
				upElementsTexture->Draw(rect, frect);
			}
		}

		if(_energy_wood)
		{
			FRect rect, frect;
			GetUpElementRects(Game::UP_WOODSTAKE_0, rect, frect);
			rect.MoveBy(GetCellPos());
			Render::BeginColor(Color(255,128,255));
			Render::DrawRect(rect, frect);
			if(batch)
			{
				batch->Draw(upElementsTexture, Render::ALPHA, rect, frect);
			}else{
				upElementsTexture->Draw(rect, frect);
			}
			Render::EndColor();
		}
	}

	int chipLayer = 0;
	if(_chip.IsFalling()) 
	{
		chipLayer = 2;
	} else if(_chip.HasDrawHang(address)) {
		chipLayer = _chip.IsChainHighlighted() ? 3 : 1;
	}

	////////// удалять по сюда ////////////
	Render::BeginAlphaMul(_flyChipAlpha);
	if( layer == chipLayer )
	{
		_chip.Draw(_pos + _flyChipOffset, address, batch, IsIce());
		if (IsCyclops())
			_chip.DrawCyclopsEye(_pos + _flyChipOffset + _eye_offset_delta, batch);
	}
	if( layer == 2 )
	{
		_chip.DrawOver();
	}
	if(layer == 1 && _chip.IsThief())
	{
		_chip.GetThief()->Draw(_pos + _flyChipOffset + _chip.GetPos());
	}
	Render::EndAlphaMul();
}

void Square::Init()
{
	_scale = 1;
	_hidingChipsOnLoseLife = false;
	//_chip.Reset();
	_isOnActiveZone = false;
	_flyAddToDownInHide = false;
	Game::FieldAddress down_address = address + Game::FieldAddress(0, -1);
	if(Game::isVisible(down_address))
	{
		float dist_down = Gadgets::squareDist[down_address];
		float dist = Gadgets::squareDist[address];
		_flyAddToDownInHide = dist_down < dist;		
	}
}


void Square::CheckMask()
{
	int mask = MakeMask();
	if(Utils2D::PictureCellGenerator::sq_mask_to_frect.find(mask) != Utils2D::PictureCellGenerator::sq_mask_to_frect.end())
	{	
		_rectInIslandCollection = Utils2D::PictureCellGenerator::sq_mask_to_frect[mask];
	}else{
		Log::Warn("Bad cell mask: " + utils::lexical_cast(mask));
		_rectInIslandCollection.xStart = -1.f;
	}
}

void Square::SwapChips(Game::Square *sq1, Game::Square *sq2)
{

	std::swap(sq1->GetChip(), sq2->GetChip());
	if(sq1->GetChip().IsThief())
	{
		sq1->GetChip().GetThief()->SetIndex(sq1->address.ToPoint());	
	}
	if(sq2->GetChip().IsThief())
	{
		sq2->GetChip().GetThief()->SetIndex(sq2->address.ToPoint());	
	}
}

	void Square::HideAll() {
		_hidingChipsOnLoseLife = true;
	}


	bool isSquare(const Game::Square *sq)
	{
		if(Game::isBuffer(sq))
		{
			return false;
		}
		const Game::FieldAddress &address = sq->address;
		const int x = address.GetCol() + 1;
		const int y = address.GetRow() + 1;
		if (x < 0 || x > GameSettings::fieldWidth ||
			y < 0 || y > GameSettings::fieldHeight) {
			return false;
		}

		return true;
	}

	bool isSquare(const Game::FieldAddress& address)
	{
		return isSquare(GameSettings::gamefield[address]);
	}

	bool isVisible(const Game::Square *sq)
	{
		if(!isSquare(sq)){
			return false;
		}
		if (sq->invisible)
		{
			return false;
		}
		if (sq->IsShortSquare()){
			return false;
		}
		return true;
	}

	bool isVisible(size_t x, size_t y)
	{
		if(x < 1 || x >= static_cast<size_t>(GameSettings::gamefield.Width())-1)
		{
			return false;
		}
		if(y < 1 || y >= static_cast<size_t>(GameSettings::gamefield.Height())-1)
		{
			return false;
		}
		return isVisible(GameSettings::gamefield[x+1][y+1]);
	}

	bool isVisible(const Game::FieldAddress& address)
	{
		return isVisible(GameSettings::gamefield[address]);
	}

	bool Square::IsEmptySquare()
	{
		if(IsStone())
		{
			return false;
		}
		if(barrierIndex != -1)
		{
			return false;
		}
		if(_bomb > 0)
		{
			return false;
		}
		if(IsIce())
		{
			return false;
		}
		if(portalEnergy)
		{
			return false;
		}
		if (IsCyclops())
		{
			return false;
		}
		//MyAssert(!_fake); //Значит не стыкуется с тем что выше или забыли сбросить фейк
		return true; //Можно ставить фишку
	
	}
	bool Square::IsEmptyForChip()
	{
		if(IsStone())
		{
			return false;
		}
		if(barrierIndex != -1)
		{
			return false;
		}
		if(_bomb > 0)
		{
			return false;
		}
		//if(IsIce())
		//{
		//	return false;
		//}
		return true; //Можно ставить фишку
	}

	bool Square::IsBusyCell() const
	{
		return _isBusyCell > 0;
	}

	bool Square::IsBusyNear() const
	{
		return IsBusyCell() || (_isBusyNear > 0);
	}

	void Square::SetBusyCell(int value)
	{
		_isBusyCell = value;
		if(_isBusyCell)
		{
			_isBusyNear = 1;
		}
	}

	void Square::SetBusyNear(int value)
	{
		_isBusyNear = value;
	}

	void Square::SetLockWithOrder(boost::shared_ptr<LockBarrierBase> ptr)
	{
		_lockWithOrder = ptr;
	}

	int Square::GetWood() const
	{
		return _wood;
	}

	void Square::SetWood(int count)
	{
		_wood = count;
		if(_wood > 0)
		{
			if(GetWall() == 0){
				SetWall(1);
			}
		}
	}

	void Square::SetEnergyWood(bool enable)
	{
		_energy_wood = enable;
		if(_energy_wood)
		{
			if(GetWall() == 0){
				SetWall(1);
			}
		}
	}

	bool Square::IsEnergyWood() const
	{
		return _energy_wood;
	}

	void Square::SetGrowingWood(bool enable)
	{
		_growingWood = enable;
		if (_growingWood) //растущий лес всегда высоты 3
		{
			SetWood(3);
		}
	}

	bool Square::IsGrowingWood() const
	{
		return _growingWood;
	}

	void Square::SetGoldWood(bool enable)
	{
		_goldWood = enable;
	}

	bool Square::IsGoldWood() const
	{
		return _goldWood;
	}


	int Square::GetWall() const
	{
		if(IsSand())
		{
			return 1;
		}
		return _wall;
	}
	float Square::GetSandTime() const
	{
		return _sandTime;
	}

	bool Square::IsSand() const
	{
		//return (_wall > 0) && _isSand;
		return _isSand;
	}

	bool Square::IsIndestructible() const
	{
		return (_wall > 0) && _indestructible;
	}

	bool Square::IsPermanent() const
	{
		return (_wall > 0) && _permanent;
	}

	bool Square::IsRestoring() const
	{
		return (_wall > 0) && _restoringWall;
	}

	bool Square::CanEnergy() const
	{
		for(int i = -1; i <= 1; i++)
		{
			Game::Square *sq = GameSettings::gamefield[address + Game::FieldAddress(i, 1)];
			if(sq->IsSand())
			{
				return false;
			}
		}
		return (_wall == 0) && !_isSand && (_wood == 0) && IsStayFly() && !_isCyclops
			&& !_is_short_square && address.IsValid();
	}

	bool Square::IsGoodForFreeFront() const
	{
		if(_isFrontFreeChecked) //Уже была во фронте - больше не используем
		{
			return false;
		}
		return CanEnergy();
	}
	
	void Square::SetEnergyChecked(bool future, float pause)
	{
		_isFrontFreeChecked = true;
		if( !future && !_isEnergyChecked) {
			_isEnergyChecked = true;
			_energyCheckTimer = pause;
		}
	}

	bool Square::IsEnergyChecked(bool future, bool pause) const
	{
		bool pauseOk = !pause || (_energyCheckTimer <= 0.0f);
		if(future){
			return pauseOk && _isFrontFreeChecked;
		}
		return pauseOk && _isEnergyChecked;
	}

	bool Square::IsFake() const
	{
		return _fake;
	}

	void Square::SetFake(const bool &fake)
	{
		_fake = fake;
	}

	void Square::SetWall(int count)
	{
		_wall = count;
		if(_wall <= 0)
		{
			SetSand(false);
			_indestructible = false;
			_permanent = false;
			_wallColor = 0;
			_restoringWall = false;
			_growingWall = false;
			_growingWallFabric = false;
			_wallRemovedWithSeqFromEnergy = false;
		}
	}

	void Square::DecWall()
	{
		MyAssert( !Game::isBuffer(this) );
		_wall = math::clamp(0, 3, _wall-1);
		bool was_sand = IsSand();
		SetSand(false, false);
		if(_wall <= 0 && !was_sand)
		{
			_indestructible = false;
			_permanent = false;
			_wallColor = 0;
			_restoringWall = false;
			_growingWall = false;
			_growingWallFabric = false;
			
			if(_wallRemovedWithSeqFromEnergy)
			{
				_wallRemovedWithSeqFromEnergy = false;
			}
			Gadgets::freeFrontDetector.Update();
			Game::Orders::KillCell( Game::Order::WALL, address );
		}
	}


	void Square::SetSand(bool isSand, const bool &immediately)
	{
		_isSand = isSand;
		if(_isSand)
		{
			if(immediately)
			{
				_sandTime = 1.f;
			}
			_sandHideFromDownToUp = false;
			_wall = 1;
		}else{
			if(immediately){
				_sandTime = 0.f;
			}
		}
	}

	void Square::SetIndestructible(bool indestructible)
	{
		_indestructible = indestructible && (_wall > 0);
	}

	void Square::SetPermanent(bool permanent)
	{
		_permanent = permanent && (_wall > 0);
	}

	void Square::SetRestoring(bool enable)
	{
		_restoringWall = enable && (_wall > 0);
	}

	void Square::SetWallColor(int wallColor)
	{
		_wallColor = wallColor;
	}

	int Square::GetWallColor() const
	{
		return _wallColor;
	}

	bool Square::IsCracked() const
	{
		return (_wall > 0) && _cracked;
	}

	void Square::SetBomb(const int &index)
	{
		_bomb = index;
		if(_bomb > 0)
		{
			_wall = 0;
			_wood = 0;
			_fake = true;
		}else{
			_fake = false;
		}
	}

	bool Square::IsShortSquare() const
	{
		return _is_short_square;
	}

	int Square::GetBombIndex()
	{
		return _bomb;
	}

	void Square::SetHangForChip(Hang hang)
	{
		_hangForChip = hang;
		Assert(IsHangBuzy());
		SetHangBuzy(false);
		CheckHangForChip();
	}

	bool Square::HangBonusAllowed() const
	{
		return !_forbid_bonus;
	}

	bool Square::IsGoodForMovingMonster()  
	{
		return (GetChip().IsSimpleChip() && !IsIce()) || GetChip().IsLicorice();
	}

	void Square::AnimateGrowingWall()
	{
		_growTimer = 0.0f;
		_bgTexture = Core::resourceManager.Get<Render::Texture>("GrowingWall");
	}

	bool Square::GrowingWallAnimationActive() const
	{
		return _growTimer < 1.0f;
	}

	void Square::OnEndMove()
	{
		if(_restoringWall)
		{
			if(_movesToRestore > 0) {
				_movesToRestore--;
			} else {
				_cracked = false;
			}
		}

		_chip.OnEndMove(Game::activeRect.Contains(address.ToPoint()));
	}

	bool Square::IsCanChainsOnStart() const
	{
		return !_isNoChainOnStart;
	}

	void Square::MarkToBeDestroyed(bool flag)
	{
		_toBeDestroyed = flag;
	}

	bool Square::ToBeDestroyed() const
	{
		return _toBeDestroyed;
	}

	char Square::Serialize() const
	{
		if (invisible)
		{
			return 'I';
		} 
		else if (IsFake())
		{
			return 'F';
		}
		else if (IsStone())
		{
			return 'S';
		} 
		else if (_chip.IsMusor())
		{
			if (IsIce())
			{
				switch (GetWall())
				{
					case 0 : { return '*'; }
					case 1 : { return 'n'; }
					case 2 : { return 'N'; }
					case 3 : { return '#'; }
				}
			} 
			else 
			{
				switch (GetWall())
				{
					case 0 : { return '+'; }
					case 1 : { return 'm'; }
					case 2 : { return 'M'; }
					case 3 : { return '-'; }
				}
			}
		}
		// есть более новый формат сохранения хамелеонов в виде ключей в xml, так что данный код не нужен
		// однако соответствующий ему код загрузки пока остается для совместимости со старыми уровнями
		//else if (_chip.IsChameleon())
		//{
		//	if (IsIce())
		//	{
		//		switch (GetWall())
		//		{
		//			case 0 : { return '$'; break; }
		//			case 1 : { return 'a'; break; }
		//			case 2 : { return 'A'; break; }
		//			case 3 : { return '9'; break; }
		//		}
		//	} 
		//	else 
		//	{
		//		switch (GetWall())
		//		{
		//			case 0 : { return '8'; break; }
		//			case 1 : { return 'k'; break; }
		//			case 2 : { return 'K'; break; }
		//			case 3 : { return '?'; break; }
		//		}
		//	}
		//} 
		else 
		{
			if (!IsIce())
			{
				return utils::lexical_cast(GetWall())[0];
			} 
			else 
			{
				return utils::lexical_cast(GetWall() + 4)[0];
			}
		}
		return '.';
	}

	void Square::Deserialize(char ch)
	{
		switch (ch)
		{
			case '0':
				SetWall(0);
				break;
			case 'e':
				SetWall(0);
				// источник энергии в данной клетке устанавливается в GameField::LoadLevelRapid
				break;
			case '1':
				SetWall(1);
				break;
			case '2':
				SetWall(2);
				break;
			case '3':
				SetWall(3);
				break;
			case '4':
				SetWall(0);
				ice = 1;
				break;
			case '5':
				SetWall(1);
				ice = 1;
				break;
			case '6':
				SetWall(2);
				ice = 1;
				break;
			case '7':
				SetWall(3);
				ice = 1;
				break;
			case '.':
				SetWall(-1);
				break;
			case 'S':
				SetWall(1);
				SetFake(false);
				SetStone(true);
				_chip.Reset(true);
				break;
			case 'I':
				SetWall(0);
				SetFake(true);
				invisible = true;
				break;
			case 'F':
				SetWall(0);
				SetFake(true);
				invisible = false;
				break;
			case '+':
				SetWall(0);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case 'm':
				SetWall(1);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case 'M':
				SetWall(2);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case '-':
				SetWall(3);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case '*':
				ice = 1;
				SetWall(0);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case 'n':
				ice = 1;
				SetWall(1);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case 'N':
				ice = 1;
				SetWall(2);
				SetFake(false);
				GetChip().SetMusor(true);
				break;
			case '#':
				ice = 1;
				SetWall(3);
				SetFake(false);
				GetChip().SetMusor(true);
				break;

			case '$' :
			case 'a' :
			case 'A' : 
			case '9' :
			{
				ice = 1;
				SetFake(false);
				GetChip().SetChameleon(true);
				// Фишка-хамелеон и лёд cверху неё
				switch (ch)
				{
					case '$' : { SetWall(0); break; }
					case 'a' : { SetWall(1); break; }
					case 'A' : { SetWall(2); break; }
					case '9' : { SetWall(3); break; }
				}
				break;
			}

			case '8' :
			case 'k' :
			case 'K' : 
			case '?' :
			{
				ice = 0;
				SetFake(false);
				GetChip().SetChameleon(true);
				// Фишка-хамелеон и льда cверху неё нет

				switch (ch)
				{
					case '8' : { SetWall(0); break; }
					case 'k' : { SetWall(1); break; }
					case 'K' : { SetWall(2); break; }
					case '?' : { SetWall(3); break; }
				}

				break;
			}
		}
	}

	int Square::CURRENT_CHECK = 1;

	bool Square::IsChecked() const
	{
		return checked == CURRENT_CHECK;
	}

	void Square::SetChecked(bool set)
	{
		checked = set ? Square::CURRENT_CHECK : 0;
	}

	bool Square::IsHangBuzy() const
	{
		return _isHangBuzy;
	}

	void Square::SetHangBuzy(bool value)
	{
		_isHangBuzy = value;
	}

	bool Square::IsStayFly() const
	{
		if(_flyType == FLY_APPEARING)
		{
			return _flySquareTime >= 1.f;
		}
		else if(_flyType == FLY_HIDING)
		{
			return _flySquareTimeBefore < 1;
		}
		return _flyType == FLY_STAY;
	}

	bool Square::IsFlyType(FlyType value, float time) const
	{
		return _flyType == value && _flySquareTime >= time;
	}

	float SolveBeQuad(const float a, const float b, const float c)
	{
		float D = b*b - 4*a*c;
		if(D >= 0)
		{
			float t1 = (-b + sqrt(D))/2.f/a;
			float t2 = (-b - sqrt(D))/2.f/a;
			return math::max(t1, t2);
		}
		Assert(false);
		return 1.f;
	}

	void Square::SetFlyPause(const float pause)
	{
		_flySquarePause = pause;
		_flySquarePauseStore = pause;
	}

	float Square::SetFlyType(FlyType value, bool first_fill)
	{
		_flySquareTime = 0.f;
		_flySquarePause = 0.f;
		if(value == FLY_APPEARING)
		{
			CheckMask();
			Assert(_flyType == FLY_NO_DRAW);
			_flyType = value;
			_flySquarePause = 0.001f; //Для того чтобы смог запуститься эффект
			_flySquareTime = 0.f;
			_flyTimerAfter = 0.f;
			_flySquareTimeBefore = 0.f;
			_flyEffectAppear = false;
			_flySquareAlpha = 0.f;
			_flyChipAlpha = 0.f;
			_flyArrowAlpha = 0.f;
			_flySquareIslandAplha = 0.f;

			if(first_fill)
			{
				_flyStartV = math::random(gameInfo.getConstFloat("FLY_SQ_START_V_DOWN_APPEAR_START", 0.f), gameInfo.getConstFloat("FLY_SQ_START_V_UP_APPEAR_START", 10.f));
				_flyG = math::random(gameInfo.getConstFloat("FLY_SQ_G_DOWN_APPEAR_START", 80.f), gameInfo.getConstFloat("FLY_SQ_G_UP_APPEAR_START", 120.f));
				_flyH = math::random(gameInfo.getConstFloat("FLY_SQ_H_DOWN_APPEAR_START", 450.f), gameInfo.getConstFloat("FLY_SQ_H_UP_APPEAR_START", 550.f));
				_flyTimerChip = -Square::FLY_SQ_CHIP_DELAY_APPEAR_FIRST;
			}else{
				_flyStartV = math::random(gameInfo.getConstFloat("FLY_SQ_START_V_DOWN_APPEAR", 0.f), gameInfo.getConstFloat("FLY_SQ_START_V_UP_APPEAR", 10.f));
				_flyG = math::random(gameInfo.getConstFloat("FLY_SQ_G_DOWN_APPEAR", 80.f), gameInfo.getConstFloat("FLY_SQ_G_UP_APPEAR", 120.f));
				_flyH = math::random(gameInfo.getConstFloat("FLY_SQ_H_DOWN_APPEAR", 450.f), gameInfo.getConstFloat("FLY_SQ_H_UP_APPEAR", 550.f));
				_flyTimerChip = -Square::FLY_SQ_CHIP_DELAY_APPEAR;
			}
			_flyTimeFull = SolveBeQuad(_flyG/2.f, _flyStartV, -_flyH);	
			_flyTimeFullChip = _flyTimeFull;
			
		}else if(value == FLY_HIDING){
			Assert(_flyType == FLY_STAY);
			CheckMask();
			_flyType = value;
			_flySquarePause = 0.001f; //Для того чтобы смог запуститься эффект
			_flySquareTime = 0.f;
			_flyTimerAfter = 0.f;
			_flySquareTimeBefore = 0.f;
			
			_flyEffectAppear = false;
			_flyChipAlpha = 1.f;
			_flySquareAlpha = 0.f;
			_flyArrowAlpha = 0.f;
			_flySquareIslandAplha = 0.f;
			
			_flyStartV = math::random(gameInfo.getConstFloat("FLY_SQ_START_V_DOWN_HIDE", 0.f), gameInfo.getConstFloat("FLY_SQ_START_V_UP_HIDE", 10.f));
			_flyStartVChip = math::random(gameInfo.getConstFloat("FLY_SQ_START_V_DOWN_HIDE_CHIP", 0.f), gameInfo.getConstFloat("FLY_SQ_START_V_UP_HIDE_CHIP", 0.f));
			_flyG = math::random(gameInfo.getConstFloat("FLY_SQ_G_DOWN_HIDE", 80.f), gameInfo.getConstFloat("FLY_SQ_G_UP_HIDE", 120.f));
			_flyH = math::random(gameInfo.getConstFloat("FLY_SQ_H_DOWN_HIDE", 450.f), gameInfo.getConstFloat("FLY_SQ_H_UP_HIDE", 550.f));
			_flyTimeFull = SolveBeQuad(_flyG/2.f, _flyStartV, -_flyH);	
			_flyTimeFullChip = SolveBeQuad(_flyG/2.f, _flyStartVChip, -_flyH);
			_flyTimerChip = -Square::FLY_SQ_CHIP_DELAY_HIDE;
		}
		return _flySquarePause;
	}

	void Square::DrawFly()
	{
		if(_flySquareAlpha <= 0 && _flySquareIslandAplha <= 0 && _flyChipAlpha <= 0)
		{
			return;
		}
		IPoint pos = address.ToPoint()*GameSettings::SQUARE_SIDE;
		IPoint cut_pos = pos;
		Render::device.PushMatrix();
		bool cut_bottom = false;
		if(_flyType == FLY_APPEARING)
		{
			Game::Square *sq_down = GameSettings::gamefield[address + Game::FieldAddress(0, -1)];
			if(Game::isVisible(sq_down) && sq_down->IsStayFly())
			{
				cut_bottom = true;
			}
		}
		//else if(_flyType == FLY_HIDING)
		//{
		//	for(int i = 1; i <= 7; i++)
		//	{
		//		Game::Square *sq_down = GameSettings::gamefield[address + Game::FieldAddress(0, -i)];
		//		if(Game::isVisible(sq_down) && sq_down->IsStayFly())
		//		{
		//			cut_bottom = true;
		//			cut_pos.y -= (i-1)*GameSettings::SQUARE_SIDE;
		//			break;
		//		}
		//	}
		//}
		if(cut_bottom)
		{
			Render::device.BeginClipping(IRect(cut_pos, 1,1), ClippingMode::BOTTOM);
		}
		
		Render::device.MatrixTranslate(_flySquareOffset);

		if(_flySquareIslandAplha > 0)
		{
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(pos);
			Render::BeginAlphaMul(_flySquareIslandAplha);
			if(_rectInIslandCollection.xStart != -1.f)
			{
				FRect rect(0.f, GameSettings::SQUARE_SIDEF*2.f,  0.f, GameSettings::SQUARE_SIDEF*2.f);
				rect.MoveBy(-GameSettings::CELL_HALF);
				Utils2D::PictureCellGenerator::collectionTexture->Draw(rect, _rectInIslandCollection);
			}
			Game::MatrixSquareScale();
			squareIslandTexture->Draw(-25.f, -37.f);
			Render::EndAlphaMul();

			Render::device.PopMatrix();
		}


		DrawSquare();

		//Тень
		if(GetChip().IsExist() && _flyType != FLY_HIDING)
		{
			bool tmp = false;
			ChipColor::Type chipType = GetChip().GetType();
			int value = GetChip().GetColor();
			if( chipType == ChipColor::LICORICE )
				value = Game::LICORICE;
			else if( chipType == ChipColor::THIEF )
				value = Game::MONSTER;
			else if( chipType == ChipColor::MUSOR )
				value = Game::MUSOR;

			if( value >= 0 )
			{
				FRect rect = Game::ChipColor::DRAW_FRECT;
				FPoint size_rect(rect.Width()/2.f, rect.Height()/2.f);
				rect.MoveBy(FPoint(-size_rect));
				float t_height = math::clamp(0.f, 1.f, 1 - (_flyChipOffset.y + _flySquareOffset.y)/_flyH);
				rect.Scale(math::lerp(0.3f, 1.f, t_height/(1 -_flySquareIslandAplha)));
				rect.MoveBy(FPoint(pos.x + size_rect.x, pos.y + size_rect.y - 10.f));
				FRect uv(Game::GetChipRect(value, (value < 55), false, false));
				ChipColor::chipsTex->TranslateUV(rect, uv);
				ChipColor::chipsTex->Bind();
			
				Render::BeginColor(Color(0, 0, 0, math::lerp(0, 30, t_height)));
				Render::DrawRect(rect, uv);
				Render::DrawRect(rect.Inflated(-size_rect.x*0.1f), uv);
				Render::EndColor();
			}
		}
		Render::device.PopMatrix();

		//Фишка
		Render::BeginAlphaMul(_flySquareAlpha);
		DrawChip(0, 0);
		DrawChip(1, 0);
		DrawChip(2, 0);
		DrawChip(3, 0);
		Render::EndAlphaMul();

		if(cut_bottom)
		{
			Render::device.EndClipping();
		}
	}

	bool Square::FlySort(Game::Square* sq1, Game::Square *sq2)
	{
		return sq1->_pos.y > sq2->_pos.y;
	}


	int Square::MakeMask()
	{
		std::string str;
		if(Gadgets::receivers.IsReceiverCell(address))
		{
			return 1;
		}
		if(IsStone())
		{
			return 4;
		}
		if(GetWood())
		{
			return 5;
		}
		if(IsIce())
		{
			return 6;
		}
		if(GetWall())
		{
			return 7;
		}
		return 0;
	}

	bool Square::IsAddToDownDraw() const
	{
		return _flyType == FLY_HIDING && _flyAddToDownInHide;
	}

	void Square::SetCyclops(bool is_cyclops)
	{
		_isCyclops = is_cyclops;
		ice = _isCyclops ? 1 : 0;
		_wood = 0;
		_wall = 0;
	}
} // namespace Game


