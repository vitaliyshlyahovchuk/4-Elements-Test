#include "stdafx.h"
#include "GameFieldControllers.h"

#include "GameField.h"
#include "Game.h"
#include "Match3Gadgets.h"
#include "GameFillBonus.h"
#include "Match3Loot.h"
#include "Match3.h"
#include "MyApplication.h"
#include "Match3Spirit.h"
#include "EditorUtils.h"
#include "Energy.h"
#include "SnapGadgetsClass.h"
#include "EnergyReceivers.h"
#include "SquareNewInfo.h"

float SPEED_INTRO_MOVE = 600.f;
float SPEED_INTRO_MOVE_FAST = 2500.f;

void GetPathBack(std::vector<IPoint> &path)
{
	IPoint pos = Gadgets::receivers.MoreFar()->GetIndex();
	path.push_back(pos);
			
	//Вычисление кратчайшего пути
	while(true)
	{
		IPoint last = path.back();
		IPoint next_max = IPoint(-1, -1);
		float value = Gadgets::squareDist[last];
		float value_next_max = value;
		for (size_t k = 0; k < Gadgets::checkDirsInfo.count; k++)
		{
			IPoint next = last + Gadgets::checkDirsInfo[k];
			float value_next = Gadgets::squareDist[next];
			if(Game::isVisible(next.x, next.y) && value_next_max < value_next )
			{
				value_next_max = value_next;
				next_max = next;
			}
		}
		//Проверяем порталы
		std::vector<Game::Square*> vec;
		size_t  count = GameSettings::GetOtherPortalsSquares(Game::FieldAddress(last.x, last.y), vec);
		for(size_t i = 0; i < count; i++)
		{
			IPoint next = vec[i]->address.ToPoint();
			float value_next = Gadgets::squareDist[next];
			if(Game::isVisible(vec[i]) && value_next_max < value_next )
			{
				value_next_max = value_next;
				next_max = next;
			}
		}
		if(next_max.x >= 0)
		{
			path.push_back(next_max);
			//Log::log.WriteDebug("<point x=\"" + utils::lexical_cast(next_max.x) + "\" y=\"" + utils::lexical_cast(next_max.y) + "\"/>");
		}else{
			break;
		}
	}
	//Log::log.WriteDebug("GetPathBack finish.");
	MyAssert(!path.empty());
}


void GetPathBack2(std::vector<IPoint> &path)
{
	std::vector<IPoint> vec;
	Gadgets::square_new_info.EnergySource_Get(vec);
	IPoint current_cell = IPoint(-1,-1);
	if(!vec.empty())
	{
		current_cell = vec.front();
	}
	SnapGadget::CHECK_FOR_INTRO++;
	float dist = 400000.f;
	SnapGadget* snap = Gadgets::snapGadgets.FindNearCaptureGadget(current_cell,dist);
	if(snap == NULL)
	{
		return;
	}
	while(snap)
	{
		snap->SetCheckedForIntro(); //Больше этот гаджет не проходим.
		path.insert(path.begin(), snap->_snapPoint);
		snap = Gadgets::snapGadgets.FindNearCaptureGadget(snap);
	}
	FPoint prev(-1,-1);
	for(std::vector<IPoint>::iterator i = path.begin(); i != path.end();)
	{
		while(i != path.end() && FPoint(*i).GetDistanceTo(prev) < GameSettings::SQUARE_SIDEF*0.5f)
		{
			i = path.erase(i);
		}
		if( i != path.end() )
		{
			prev = *i;
			++i;
		}
	}
	//path.clear();
	//path.push_back(FPoint(126, 46)*GameSettings::SQUARE_SIDEF);
	//path.push_back(FPoint(105, 46)*GameSettings::SQUARE_SIDEF);
}

StartFieldMover::StartFieldMover()
	: GameFieldController("StartFieldMover", 1.f, GameField::Get())
	,_timer(-1.f)
	, _started(false)
	, last_time(0.f)
	, collect_dist(0.f)
	, _startValue(0.4f)
{
	gameField->SetFieldMoving(true);
	std::vector<IPoint> vec;
	GetPathBack2(vec);

	for(size_t i = 0; i < vec.size(); i++)
	{
		vec[i] -= GameSettings::FIELD_SCREEN_CENTER;
	}

	_splinePath.Clear();

	_timer = 0.9999f;
	if(vec.size() == 0)
	{
		vec.push_back(FPoint(GameSettings::fieldX, GameSettings::fieldY).Rounded());
	}
	if(vec.size() == 1)
	{
		time_scale = 1.f;
		
		//gameField->CorrectFocusPosition(v.x, v.y);
		IPoint v = vec.front();
		//if(Gadgets::snapGadgets.CheckTargetingFirst(v))
		//{
		//	v -= GameSettings::FIELD_SCREEN_CENTER;
		//}
		GameField::Get()->_destFieldPos.x = v.x;
		GameField::Get()->_destFieldPos.y = v.y;
		Gadgets::snapGadgets.UpdateActive();

		_splinePath.addKey(FPoint(v.x, v.y));
		_splinePath.addKey(FPoint(v.x, v.y));
	}else{
		//Продлеваем. Интерполируем немного для разгона.
		if(vec.size() == 2)
		{
			//Для сплайна безье элементов должно быть больше 2
			vec.insert(vec.begin()+1, math::lerp(vec[0], vec[1], 0.5f));
		}
		FPoint delta_v = vec[0] - vec[1];
		delta_v.Normalize();
		vec[0] +=( delta_v*GameSettings::SQUARE_SIDEF*1.5f).Rounded();


		time_scale = 0.01f; 


		//SplineBezier prev_spline;
		for(size_t i = 0; i < vec.size(); i++)
		{
			prev_spline.addKey(vec[i]);
		}
		prev_spline.CalculateGradient();
		
		_fullLenght = 0.f;
		FPoint prev = FPoint(prev_spline.getGlobalFrame(0.f).x, prev_spline.getGlobalFrame(0.f).y);
		for(size_t i = 1; i < vec.size()*10; i++)
		{
			float t = i/(vec.size()*10.f);
			FPoint p = FPoint(prev_spline.getGlobalFrame(t).x, prev_spline.getGlobalFrame(t).y);
			_fullLenght += prev.GetDistanceTo(p);
			prev = p;
		}
		float period = 1.f/(SPEED_INTRO_MOVE/_fullLenght);
		_startValue = math::min(0.5f/period, 0.4f);

		_splinePath.Clear();

		prev = FPoint(prev_spline.getGlobalFrame(0.f).x, prev_spline.getGlobalFrame(0.f).y);
		_splinePath.addKey(prev);
		float t = 0.f;
		float dist = 0.f;
		float dist_limmit = 0.f;
		const float step = 40.f;
		while(dist < _fullLenght - step)
		{
			t += 20.f;
			FPoint p = FPoint(prev_spline.getGlobalFrame(t/_fullLenght).x, prev_spline.getGlobalFrame(t/_fullLenght).y);
			dist += p.GetDistanceTo(prev);
			dist_limmit += p.GetDistanceTo(prev);
			if(dist_limmit >= step)
			{
				_splinePath.addKey(p);
				dist_limmit = 0.f;
			}
			prev = p;
		}
		_splinePath.addKey(FPoint(prev_spline.getGlobalFrame(1.f).x, prev_spline.getGlobalFrame(1.f).y));
	}

	_splinePath.CalculateGradient();

	
	IPoint first_pos = _splinePath.getGlobalFrame(1.f).Rounded();
	pos_prev = first_pos;

	gameField->_destFieldPos = first_pos;
	
	GameSettings::fieldX = first_pos.x;
	GameSettings::fieldY = first_pos.y;

	_prevGradient = _splinePath.getGlobalGradient(0.f);
	_timeFast = 0.f;
	Game::UpdateVisibleRects();
	Game::UpdateFlySquares(Game::activeRect, Game::activeRect, true);
}

void StartFieldMover::Start()
{
	_started = true;
}

float max_angle = 0.f;
float min_angle = 10000.f;
	
void StartFieldMover::Update(float dt)
{
	if(_timeFast > 0 && _timeFast < 1)
	{
		_timeFast += dt*3.f;
		if(_timeFast >= 1)
		{
			_timeFast = 1.f;
		}
	}
	if(!_started)
	{
		return;
	}
	if(_timer < 0)
	{
		_timer += dt;
		if(_timer > 0)
		{
			_timer = 0.f;
		}
	}
	else if(_timer < 1)
	{
		float period = 0.01f*time_scale;
		float t = math::clamp(0.f, 1.f, _timer);
		FPoint curr_p = GetPos(t);
		FPoint next_p = GetPos(t + period);
		float speed = curr_p.GetDistanceTo(next_p)/period;
		float speed_need = math::lerp(SPEED_INTRO_MOVE, SPEED_INTRO_MOVE_FAST, _timeFast);
		if(_timer < _startValue)
		{
			speed_need = speed_need*(_timer/_startValue) + 50.f;
		}
		if(1 - _timer < _startValue)
		{
			speed_need = speed_need*((1 -_timer)/_startValue*0.8f + 0.2f);
		}
		if(speed > 0.f)
		{
			time_scale = speed_need/speed;
		}
		//FPoint gradient = _splinePath.getGlobalGradient(t);
		//float angle = gradient.GetDirectedAngleNormalize(_splinePath.getGlobalGradient(t+ period*4.f));
		//if(angle > math::PI)
		//{
		//	angle = angle - math::PI*2.f;
		//}
		//
		//max_angle = math::max(max_angle, angle);
		//min_angle = math::min(min_angle, angle);

		//angle = abs(angle);
		//
		//time_scale *= math::clamp(0.4f, 1.f, 1 - (angle/0.13f));
		//_prevGradient = gradient;

		_timer += dt*time_scale;
		if(_timer > 1)
		{
			_timer = 1.f;
		}
	}
	IPoint p = GetPos().Rounded();

	pos_prev = p;
	gameField->_destFieldPos = p;
	
	GameSettings::fieldX = p.x;
	GameSettings::fieldY = p.y;
	Game::UpdateVisibleRects();
	if(_timer >= 1)
	{
		gameField->SetFieldMoving(false);
		Gadgets::snapGadgets.UpdateActive();
		//gameField->OpenEnergySource();
		gameField->_introController = NULL;
		finished = true;
	}
	FPoint dir = _splinePath.getGlobalGradient(math::clamp(0.f, 1.f, _timer));
	if(dir.Length() > 0)
	{
		Game::currentCameraGradient = dir.Normalized();
	}
}


FPoint StartFieldMover::GetPos(float t)
{
	t = math::clamp(0.0f, 1.0f, t);
	return _splinePath.getGlobalFrame(t);
	//float dt = std::min(_startValue * 0.3f * time_scale, std::min(t, 1.0f - t));
	//return  _splinePath.getGlobalFrame(t - dt) * 0.5f + 
	//		_splinePath.getGlobalFrame(t + dt) * 0.5f;
}

FPoint StartFieldMover::GetPos()
{
	return GetPos(_timer);
}

bool StartFieldMover::isFinish()
{
	return _timer >= 1.f;
}

void StartFieldMover::Draw()
{
	if(!EditorUtils::draw_debug)
	{
		return;
	}

	Render::BeginColor(Color::RED);
	Render::device.SetTexturing(false);
	size_t count = _splinePath.keys.size()*10;
	FPoint p_prev = GetPos(0.f) + GameSettings::FIELD_SCREEN_CENTER;
	for(size_t i = 1; i < count; i++)
	{
		float t = i/(count - 1.f);
		FPoint p =  GetPos(t) + GameSettings::FIELD_SCREEN_CENTER;
		Render::DrawLine(p_prev, p);
		p_prev = p;
	}
	Render::EndColor();

	if(!prev_spline.error)
	{
		Render::BeginColor(Color::BLACK);
		for(size_t i = 0; i <= 50; i++)
		{
			float t = i/50.f;
			FPoint p =  FPoint(prev_spline.getGlobalFrame(t).x, prev_spline.getGlobalFrame(t).y) + GameSettings::FIELD_SCREEN_CENTER;
			Render::DrawRect(IRect(p.x, p.y, 0,0).Inflated(5));
			p_prev = p;	
		}
		Render::EndColor();
	}
	Render::device.SetTexturing(true);
}

bool StartFieldMover::MouseUp(const IPoint &mouse_pos)
{
	if(!GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
	{
		return false;
	}
	if(_timeFast == 0 && _timer/time_scale < 1.f)
	{
		_timeFast = 0.001f;
		if(_timer < 0)
		{
			_timer = 0.f;
		}
		return true;
	}
	return false;
}

size_t debug_counter_FieldMover_paths = 0;

FieldMover::FieldMover(GameField* gamefield, float time_scale_)
	: GameFieldController("FieldMover", time_scale_, gamefield)
{
	_stopingTimer = 0.f;

	local_time = 0.f;
	gameField->SetFieldMoving(true);

	_lastDest = gameField->_destFieldPos;
	_paths.push_back(Path(math::Vector3(GameSettings::fieldX, GameSettings::fieldY, 0.f), gameField -> _destFieldPos, time_scale));

	_finished = false;

	//Текущее направление камеры
	FPoint dir = FPoint(GameField::Get()->_destFieldPos) - FPoint(GameSettings::fieldX, GameSettings::fieldY);
	if(dir.Length() > 0)
	{
		Game::currentCameraGradient = dir.Normalized();
	}

	//Исчезновение клеток (должно быть заранее)
	FRect target_rect = FRect(0.f, GameSettings::VIEW_RECT.width, 0.f, GameSettings::VIEW_RECT.height);
	target_rect.MoveTo(FPoint(gameField->_destFieldPos.x, gameField->_destFieldPos.y));
	IRect future_active_rect = Game::CreateActiveRect(target_rect);
	if(future_active_rect.x != Game::activeRect.x || future_active_rect.y != Game::activeRect.y)
	{
		_pauseWhileSquareFlyHiding = Game::UpdateFlySquares(Game::activeRect, future_active_rect, false, Game::Square::FLY_SQ_DELAY_FOR_APPEAR_AFTER_MOVE);
	}

}


FieldMover::~FieldMover()
{
	Gadgets::freeFrontDetector.SetCameraIsStand();
}

void FieldMover::Update(float dt_)
{  
	_pauseWhileSquareFlyHiding -= dt_;
	if(_pauseWhileSquareFlyHiding > 0)
	{
		return;
	}
	//Тормозим камеру принудительно если началось выделение цепочки
	bool need_stop = gameField->SelectingSequence();
	if(need_stop && _stopingTimer < 1)
	{
		_stopingTimer += dt_;
		if(_stopingTimer > 1)
		{
			_stopingTimer = 1.f;
		}
	}else if(!need_stop && _stopingTimer > 0)
	{
		_stopingTimer -= dt_;
		if(_stopingTimer <= 0)
		{
			_stopingTimer = 0.f;
		}
	}
	float scale_dt = math::clamp(0.f, 1.f, 1 - _stopingTimer);
	if(scale_dt == 0)
	{
		//Камера должна полностью остановиться. Вычисления останавливаются.
		return;
	}
	float dt = dt_*scale_dt;


	_finished = true;

	//math::Vector3 curr = gameField->_destFieldPos;

	//float r = sqrt((curr.x-_lastDest.x)*(curr.x-_lastDest.x) + (curr.y-_lastDest.y)*(curr.y-_lastDest.y));

	//if (r > 4.f)
	//{
	//	_paths.push_back(Path(math::Vector3(GameSettings::fieldX, GameSettings::fieldY, 0.f), curr, time_scale));
	//	_pathTimes.push_back(PTime(0.f, _paths.back().t_scale));
	//	_lastDest = curr;
	//	if(debug_counter_FieldMover_paths < _paths.size())
	//	{
	//		debug_counter_FieldMover_paths = _paths.size();
	//		//Log::log.WriteDebug("debug_counter_FieldMover_paths=" + utils::lexical_cast(debug_counter_FieldMover_paths));
	//	}
	//}

	for (size_t i = 0; i < _paths.size(); i++)
	{
		Path &p = _paths[i];
		p.t += dt * p.t_scale;
		
		if (p.t > 1.0f)
		{
			p.t = 1.0f;
		}
		else
		{
			_finished = false;
		}
	}

	for (size_t i = 0; i < _pathTimes.size(); i++)
	{
		float &t = _pathTimes[i].t;
		t += dt * _pathTimes[i].ts;

		if (t > 1.0f)
		{
			t = 1.0f;
		}
		else
		{
			_finished = false;
		}
	}
	math::Vector3 p_future;
	if (_paths.size() == 1)
	{
		Path &p = _paths[0];
		math::Vector3 pos = p.getValue();
		GameSettings::fieldX = pos.x;
		GameSettings::fieldY = pos.y;
		p_future = p.getValue(0.3f*p.t_scale);
	} else {
		math::Vector3 pos = _paths[0].getValue();
		for (size_t i = 0; i<_pathTimes.size(); i++)
		{
			float &t = _pathTimes[i].t;
			float t1 = math::clamp(0.f, 1.f, 0.5f-0.5f*cosf(math::PI*t));
			pos = math::lerp(pos, _paths[i+1].key2, t1);
		}
		p_future = pos;
		GameSettings::fieldX = pos.x;
		GameSettings::fieldY = pos.y;
	}
	Game::UpdateVisibleRects();

	if( Gadgets::levelSettings.getString("TriggerBonusOffScreen") == "true"
		&& Gadgets::levelSettings.getString("MoveBonusWithCamera") == "false" )
	{
		gameField->TriggerOffscreenBonuses();
	}
	FPoint dir = FPoint(GameField::Get()->_destFieldPos) - FPoint(GameSettings::fieldX, GameSettings::fieldY);
	if(dir.Length() > 0)
	{
		Game::currentCameraGradient = dir.Normalized();
	}
}

bool FieldMover::isFinish()
{
	if (_finished)
	{
		gameField->SetFieldMoving(false);
		
		// Явно округляем координаты положения экрана
		GameSettings::fieldX = (float) math::round(GameSettings::fieldX);
		GameSettings::fieldY = (float) math::round(GameSettings::fieldY);
		Game::UpdateVisibleRects();

		gameField->_maybeMoveGadget.NeedUpdate();

		gameField->MoveBonusesToViewRect();

		if(gameField->GetLevelObjective() == LevelObjective::DIAMONDS) {
			Game::UpdateDiamondsCount();
		}
	}

	return _finished;
}

SequenceDestroyer::SequenceDestroyer(const AddressVector& chipSeq_, GameField *gamefield_)
	: GameFieldController("SequenceDestroyer", 1.f/Game::ChipColor::CHIP_REMOVE_DELAY, gamefield_)
	, _chipSeq(chipSeq_)
	, _chipSeqCopy(chipSeq_)
	, _boom(false)
	, _scoreAdded(false)
	, _chipNum(0)
    , _isOnPauseBeforeFalling(false)
    , _pauseTime(0.0f)
    , _pauseBeforeFalling(0.7f)
    , _isAnythingDestroyed(false)
{
	z = 3;

	local_time = 0.f;

	_lengthBonus = gameField->getBonusLevelForSeq(_chipSeq);

	Init();

	for(Game::Square *sq : GameSettings::squares)
	{
		sq->GetChip().ClearOffset();
	}

	if (_chipSeq.size() >= 3)
		GameSettings::need_inc_ground_cycops = true;
}

void SequenceDestroyer::Init()
{
	Assert(_chipSeq.size() >= 2);

	//_energyTex = Core::resourceManager.Get<Render::Texture>("Wonder1");
	//_energyTex->setFilteringType(Render::Texture::BILINEAR);
	//_energyTex->setAddressType(Render::Texture::REPEAT);

	//for (size_t q = 0; q < _chipSeq.size(); q++)
	//{
	//	float r = 10.f;
	//	if (q > 0 && q < _chipSeq.size()-1)
	//	{
	//		r = 20.f;
	//	}
	//	FPoint v = GameSettings::gamefield[_chipSeq[q]]->GetChipPos() + GameSettings::CELL_HALF;
	//	_strip.addPathKey(v.x +  math::random(-8.f, 8.f), v.y + math::random(-8.f, 8.f), r);
	//}
	//size_t count = _chipSeq.size();
	//_strip.CalculateBuffer(count*20/4);

	//_strip.setStripLength(0.6f);
	//_strip.setTextureScale(1.0f);

	//_stripTimeScale = time_scale / count;
	//_stripTime = 0.1f;
	//_strip.setTextureSpeed(-_stripTimeScale*4);

	GameSettings::sequenceWithEnergySquare = false;
	for(AddressVector::iterator itr = _chipSeq.begin(); itr != _chipSeq.end(); ++itr )
	{
		Game::Square *sq = GameSettings::gamefield[*itr];
		sq->GetChip().GetHang().Clear();
		sq->MarkToBeDestroyed();
		if(Energy::field.FullOfEnergy(sq->address))
		{
			GameSettings::sequenceWithEnergySquare = true;
		}
	}
}

SequenceDestroyer::~SequenceDestroyer()
{
}

void SequenceDestroyer::Update(float dt)
{
	//_stripTime += dt*_stripTimeScale;
	//_strip.setStripTime(_stripTime);

	local_time += dt*time_scale;
	
	if (local_time >= 0 && _chipSeq.size() > 0)
	{
		local_time -= 1.f;
        
		Game::FieldAddress pos = *(_chipSeq.begin());
        
		_chipSeq.erase(_chipSeq.begin());
        
		Game::Square *s = GameSettings::gamefield[pos];
		Game::Square *s_chip = s;
        
		if(_chipSeq.empty())
		{
			_isOnPauseBeforeFalling = true;
            
			if (_lengthBonus > 0)
			{
				//Если убивать последнюю фишку не надо, то очищаем тут
				if(Gadgets::levelSettings.getString("ChainChipDieType") == "chip")
				{
					s_chip = NULL;
                    
					// пусть даже последняя фишка не убивается, все равно нужно зачесть ее для заказов
					Game::Orders::KillCell( Game::Order::Objective(s->GetChip().GetColor()), pos );
				}
			}
		}
		float pause = 0.0f;//_chipNum * Game::ChipColor::CHIP_REMOVE_DELAY + 0.1f;
        
		int score = GameSettings::score.chip_base + GameSettings::score.chip_add * (_chipNum / GameSettings::score.chip_num);
		_chipNum++;
        
		// запускаем навешиваемый бонус
		if(s_chip && s_chip->GetChip().IsFutureHang())
		{
			// Почти копипаста из GameField::EndMove. Делаем одно и то же в нескольких разных местах из-за того, что
			// поддерживаем десятки разных настроек влияющих на то как, куда и когда вешаются бонусы =(
			int bonusLevel = gameField->getBonusLevelForSeq(_chipSeqCopy);
			if( bonusLevel > 0 )
			{
				const Game::FieldAddress last0 = _chipSeqCopy[_chipSeqCopy.size() - 2];
				const Game::FieldAddress last1 = _chipSeqCopy[_chipSeqCopy.size() - 1];
                
				IPoint d = (last0.ToPoint() - last1.ToPoint());
				bool straightChain = (math::abs(d.x) + math::abs(d.y) == 1);
                
				std::string type = gameField->getBonusType(bonusLevel, straightChain);
                
				if( !GameField::Get()->_tutorialHangBonusTypes.empty() )
				{
					type = gameField->_tutorialHangBonusTypes.front();
					gameField->_tutorialHangBonusTypes.pop_front();
				}
				std::string bonusChip = Gadgets::levelSettings.getString("ChainChipDieType");
                
				if( bonusChip == "screen" ) // с дргими вариантами этой настройки бонусы взрываются в конце хода (GameField::EndMove)
				{
					std::string transform = Gadgets::levelSettings.getString("ChainChipTransform");
					Game::Hang::TransformChip tr = Game::Hang::NONE;
					if( transform == "chameleon" )
						tr = Game::Hang::CHAMELEON;
					else if( transform == "energy_bonus" )
						tr = Game::Hang::ENERGY_BONUS;
					IPoint dir(1, 0);
					std::string bonus_number = utils::lexical_cast(math::clamp(0, 2, bonusLevel-1));
					std::string rstr = straightChain ? "Radius" : "DRadius";
					int radius = Gadgets::levelSettings.getInt(rstr + bonus_number);
					int count = Gadgets::levelSettings.getInt("BCount" + bonus_number);
                    
					_seqHang = Game::Hang(type, radius, bonusLevel, last1.ToPoint() - last0.ToPoint(), tr, false);
				}
			}
            
			FPoint fpos = FPoint(pos.ToPoint()) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
            
			//Духа родим позже в момент убийства фишки
			s_chip->GetChip().SetSpirit(_seqHang);
		}
        
		bool isSomethingDestroyed = false;
		Game::ClearCell(s, s_chip, Game::GetCenterPosition(pos), false, pause, false, ColorToMask(s->GetChip().GetColor()), score, false, false, &isSomethingDestroyed);
        
		if (isSomethingDestroyed) {
			_isAnythingDestroyed = true;
		}
        
        if (_chipSeq.empty())
        {
            if (!_isAnythingDestroyed)
            {
                _pauseBeforeFalling = 0.1f;
                
                if (_lengthBonus > 0)
                {
                    _pauseBeforeFalling = 0.3f;
                }
            }
            
            if (!gameField->_bonusCascade.empty())
            {
                _pauseBeforeFalling = -1.f;
            }
            
        }
	}
    
	if (_isOnPauseBeforeFalling)
	{
		_pauseTime += dt;
	}
}



bool SequenceDestroyer::isFinish()
{
	bool isPauseFinished = false;
	if (_chipSeq.empty() && local_time >= 0 && !_scoreAdded)
	{
		_scoreAdded = true;
	}
    
	if (_isOnPauseBeforeFalling && _pauseTime >= _pauseBeforeFalling)
	{
		//Фишки падают
		for(size_t i = 0; i < _chipSeqCopy.size(); i++)
		{
			int col = _chipSeqCopy[i].GetCol();
            
			Match3::RunFallColumn(col-1);
			Match3::RunFallColumn(col);
			Match3::RunFallColumn(col+1);
		}
		
        gameField->OnSequenceDestroyed();
        
		isPauseFinished = true;
	}
    
	return isPauseFinished;
}

void SequenceDestroyer::Draw()
{
	//_energyTex->Bind();
	//Render::device.SetBlendMode(Render::ADD);

	//Render::device.SetCurrentMatrix(Render::MODELVIEW);
	//for (int i = 0; i < 3; i++)
	//{
	//	Render::device.PushMatrix();
	//	Render::device.MatrixTranslate(math::Vector3(math::random(-2.f, 2.f), math::random(-2.f, 2.f), 0.f));
	//	_strip.setStripTime(_stripTime + math::random(0.f, 0.01f));
	//	_strip.Draw();
	//	Render::device.PopMatrix();
	//}
	//Render::device.SetBlendMode(Render::ALPHA);
}

ChipPiecesFly::ChipPiecesFly(math::Vector3 pos, Render::Texture *tex_, Color color_, float startTime, float kV, int col_count, int row_count, int width, int height )
	: GameFieldController("ChipPiecesFly", 1.2f, GameField::Get())
	, _tex(tex_)
{
	local_time = -1.f;

	int num = col_count*row_count;

	float rh = width*GameSettings::SQUARE_SIDE/2.f-8;
	float rv = height*GameSettings::SQUARE_SIDE/2.f-8;

	for (int n = 0; n<num; n++)
	{
		SqPiece piece;

		piece.color = color_;
		piece.color.alpha = 0;
		if (startTime >= 0.f)
		{
			piece.color.alpha = 255;
			piece.alpha = 255.f;
		}

		piece.local_time = startTime;

		piece.angle = math::random(-180.f, 180.f);

		float x = math::random(-rh, rh);
		float y = math::random(-rv, rv);

		piece.x = pos.x + width*GameSettings::SQUARE_SIDE/2 + x;
		piece.y = pos.y + height*GameSettings::SQUARE_SIDE/2 + y;

		piece.dx = x*8*kV;
		piece.dangle = math::random(-180.f, 180.f);
		piece.dy = y*2.8f + math::random(10.f, 30.f)*kV;
		piece.d2y = math::random(-300.f, -400.f);

		piece.dx /= 1.f;
		piece.dy /= 1.f;

		piece.alpha = 255.f;

		piece.u1 = (n%col_count)/(col_count*1.f);
		piece.u2 = piece.u1+1.f/(col_count*1.f);
		piece.v1 = 1.f - (n/col_count+1)/(row_count*1.f);
		piece.v2 = piece.v1 + 1.f/(row_count*1.f);

		_pieces.push_back(piece);
	}

	_texName = _tex->textureID;
}

void ChipPiecesFly::Update(float dt)
{
	local_time += dt*time_scale;

	for (size_t i = 0; i<_pieces.size(); i++)
	{
		SqPiece &p = _pieces[i];

		if (p.local_time < 0.f)
		{
			p.local_time += dt*4;
			if (p.local_time >= 0.f)
			{
//				p.local_time = 0.f;
				p.color.alpha = 255;
			} else if (p.local_time >= -1.f) {
				p.color.alpha = math::lerp(0, 230, 1.f + local_time);
			}
		} else {
			p.local_time += dt*time_scale;

			p.angle += p.dangle*dt;
			p.x += p.dx*dt;
			p.y += p.dy*dt;
			p.dy += p.d2y*dt;
			p.dx /= (1.f + dt/2.f);

			if (p.local_time >= 0.4f)
			{
				p.alpha -= dt*256.f;
				if (p.alpha<0.f)
				{
					p.alpha = 0.f;
				}
				p.color.alpha = static_cast<int>(p.alpha);
			}
		}
	}
}

bool ChipPiecesFly::isFinish()
{
	return (local_time >= 1.f);
}

void ChipPiecesFly::Draw()
{
	for (size_t i = 0; i<_pieces.size(); i++)
	{
		Render::device.SetCurrentMatrix(Render::MODELVIEW);

		SqPiece &p = _pieces[i];

		Render::BeginColor(p.color);

		Render::device.PushMatrix();

		Render::device.MatrixTranslate(math::Vector3(p.x, p.y ,0));
		Render::device.MatrixRotate(math::Vector3(0,0,1), p.angle);
	
		_tex->Draw(IRect(-16, -16, 32, 32), p.u1, p.u2, p.v1, p.v2);

		Render::device.PopMatrix();
		Render::EndColor();
	}
}

DelaySendMessage::DelaySendMessage(std::string  layer, std::string  widget, std::string  message, float delay, GameField *gamefield_)
	: GameFieldController("DelaySendMessage", 1.f, gamefield_)
	, _layerName(layer)
	, _widgetName(widget)
	, _message(message)
{
	if (delay < 0.0001f)
	{
		time_scale = 10000.f;
	} else {
		time_scale = 1.f/delay;
	}
}

DelaySendMessage::DelaySendMessage(std::string  layer, std::string  widget,const Message &message, float delay, GameField *gamefield_)
	: GameFieldController("DelaySendMessage", 1.f, gamefield_)
	, _layerName(layer)
	, _widgetName(widget)
	, _message(message)
{
	if (delay < 0.0001f)
	{
		time_scale = 10000.f;
	} else {
		time_scale = 1.f/delay;
	}
}

void DelaySendMessage::Update(float dt)
{
	local_time += dt*time_scale;
}

bool DelaySendMessage::isFinish()
{
	if (local_time >= 1.f)
	{
		Core::guiManager.getLayer(_layerName)->getWidget(_widgetName)->AcceptMessage(_message);
		return true;
	} else {
		return false;
	}
}

ReshuffleChipFly::ReshuffleChipFly(Game::ChipColor &chip, Game::Square *sq)
	: GameFieldController("ReshuffleChipFly", 1.0f, GameField::Get())
	, _chip(chip)
{
	_chip.SetFly();

	FPoint center = GameSettings::GetCenterOnField() - GameSettings::CELL_HALF - sq->GetCellPos();
	//_splinePath.addKey(chip.GetPos());

	float start_angle = 0.f;
	if(chip.GetPos().Length() > 1)
	{
		start_angle = chip.GetPos().GetDirectedAngle(center);
	}
	int max_count = math::random(4, 10);
	for(int i = 0; i <= max_count; i++)
	{
		float angle = start_angle + i*math::PI/3.f;
		FPoint rotate_pos = center + FPoint(math::cos(angle), math::sin(angle)) * GameSettings::SQUARE_SIDEF*math::random(0.2f, 1.f); 
		float t = (i+ 0.f)/max_count;
		FPoint dir_pos = math::lerp(chip.GetPos(), FPoint(0.f, 0.f), t);
		float smooth = math::clamp(0.f, 1.f, math::sin(t*math::PI)*4.f);

		_splinePath.addKey(math::lerp(dir_pos, rotate_pos, smooth));
		//_splinePath.addKey(rotate_pos);
	}


	//float angle = math::random(0.0f, 2.0f * math::PI);
	//_splinePath.addKey(center + FPoint(math::cos(angle), math::sin(angle)) * GameSettings::SQUARE_SIDEF * math::random(0.0f, 1.0f));

	//angle = math::random(0.0f, 2.0f * math::PI);
	//_splinePath.addKey(center + FPoint(math::cos(angle), math::sin(angle)) * GameSettings::SQUARE_SIDEF * math::random(0.0f, 1.0f));

	//_splinePath.addKey(FPoint(0,0));
	_splinePath.CalculateGradient();

	local_time = -math::random(0.0f, 0.35f);
	time_scale = 1.f;
}

void ReshuffleChipFly::Update(float dt)
{
	if( local_time < 0.0f )
	{
		local_time += dt;
	}
	else if( local_time < 1.0f )
	{
		_chip.SetPos(_splinePath.getGlobalFrame(math::ease(local_time, 0.2f, 0.2f)));
		local_time += time_scale * dt;
		if( local_time >= 1.0f )
		{
			_chip.ResetFly();
			_chip.SetPos(FPoint(0,0));
			gameField->_reshuffleChipsFlying--;
		}
	}
}

bool ReshuffleChipFly::isFinish()
{
	return (local_time >= 1.0f);
}

FlyFillBonusEffect::FlyFillBonusEffect(GameField *gamefield, FPoint from)
	: GameFieldController("FlyFillBonusEffect", 1.0f, gamefield)
	, start(GameSettings::ToScreenPos(from))
	, end(Match3GUI::FillBonus::Get().GetCenterPosition())
{
	_trailEffect = gamefield->_effContUp.AddEffect("SeqFire");
	_trailEffect->SetPos(start);
	_trailEffect->Reset();
}

void FlyFillBonusEffect::Update(float dt)
{
	if( local_time <= 1.0f)
	{
		local_time += time_scale * dt;

		if( local_time >= 1.0f )
		{
			_trailEffect->Finish();
			Match3GUI::FillBonus::Get().ChangeCounter(1);
		}
		else
		{
			float t = local_time * local_time;
			_trailEffect->SetPos( math::lerp(start, end, t) );
		}
	}
	_trailEffect->Update(dt);
}

void FlyFillBonusEffect::DrawAbsolute()
{
}

bool FlyFillBonusEffect::isFinish()
{
	return (local_time >= 1.0f) && _trailEffect->isEnd();
}


FlyFiresEffect::FlyFiresEffect(GameField *gamefield, FPoint pos)
	: GameFieldController("FlyFiresEffect", 12.0f, gamefield)
	, _pos(pos)
	, _moveTimer(0.0f)
	, _moveTime(0.0f)
	, _count(0)
	, _fillBonus(false)
{
	_fires[0].reset();
	_fires[1].reset();
	_fires[2].reset();
}

void FlyFiresEffect::Update(float dt)
{
	time_scale = 14.0f - 2.0f * _count;
	local_time += dt * time_scale;

	FPoint pos = _pos;

	if(_moveTimer > 0.0f){
		_moveTimer = std::max(0.0f, _moveTimer - dt);
		pos = math::lerp(_dest, _pos, _moveTimer/_moveTime);
	} else {
		_pos = _dest;
		if(_fillBonus)
		{
			_fillBonus = false;
			Match3GUI::FillBonus::Get().ChangeCounter(_count);
			SetFires(0);
		}
	}

	float radius = GameSettings::SQUARE_SIDEF * 0.8f;
	if(_fillBonus) {
		radius *= math::lerp(0.0f, 1.0f, _moveTimer/_moveTime);
	}

	//float da = (_count > 0) ? 2.0f * math::PI / _count : 0.0f;

	// анимация кручения огоньков вокруг точки
	float t = local_time;
	for(int i = 0; i < 3; i++)
	{
		if(_fires[i].get())
		{
			//float t = local_time + da * i;
			//float r = radius * (1.0f + 0.1f * i * math::sin(t * 1.4f));
			
			float r = radius * (1.0f + 0.2f * i);

			FPoint fire_pos(r * math::cos(t), r * math::sin(t));
			
			_fires[i]->SetPos(pos + fire_pos);

			t *= -0.7f;
		}
	}

	_effCont.Update(dt);
}

void FlyFiresEffect::DrawAbsolute()
{
	_effCont.Draw();
}

bool FlyFiresEffect::isFinish()
{
	return _effCont.IsFinished();
}

void FlyFiresEffect::FlyToPos(FPoint pos, float time)
{
	_moveTimer = _moveTime = time;
	_dest = pos;
}

void FlyFiresEffect::SetFires(int fires)
{
	Assert(fires >= 0 && fires <= 3);
	_count = fires;
	// зажигаем недостающие огоньки
	for(int i = 0; i < _count; i++)
	{
		if( !_fires[i].get()) {
			_fires[i] = Game::AddEffect(_effCont, "SeqFire");
			_fires[i]->Reset();
		}
	}
	// тушим лишние огоньки
	for(int i = _count; i < 3; i++)
	{
		if(_fires[i].get()) {
			_fires[i]->Finish();
		}
		_fires[i].reset();
	}
}

void FlyFiresEffect::FlyToFillBonus()
{
	_fillBonus = true;
	FlyToPos(Match3GUI::FillBonus::Get().GetCenterPosition(), 1.0f);
}


AddScoreEffect::AddScoreEffect(FPoint pos, int score, float scale, Color color, float duration, float delay, const std::string &effName)
	: GameFieldController("AddScoreEffect", 1.0f / duration, GameField::Get())
	, _score(score)
	, _scale(scale)
	, _color(color)
{
	z = 3;
	_pos = pos;
	local_time = -delay * time_scale;

	if( !effName.empty() )
		_eff = Game::AddEffect(gameField->_effCont, effName);
}

void AddScoreEffect::Update(float dt)
{
	if(local_time < 0.0f)
	{
		local_time += time_scale * dt;
		if( local_time >= 0.0f && _eff)
		{
			_eff->SetPos(_pos);
			_eff->Reset();
		}
	}
	else if(local_time < 1.0f)
	{
		local_time += time_scale * dt;

		if(local_time >= 1.0f) {
			Match3GUI::LootPanel::AddScore(_score);
		}
	}
}

void AddScoreEffect::Draw()
{
	if( local_time >= 0.0f )
	{
		FPoint offset(0.0f, 20.0f * local_time);
		Color col = _color;
		col.alpha = math::clamp(0, 255, math::round(1.5f * 255.0f * math::sin(math::PI * local_time)));

		Render::BeginColor(col);
		Render::BindFont("Score");
		Render::PrintString(_pos + offset, utils::lexical_cast(_score), _scale, CenterAlign, CenterAlign);
		Render::EndColor();

		if(_eff)
			_eff->SetPos(_pos + offset);
	}
}

bool AddScoreEffect::isFinish()
{
	return (local_time >= 1.0f);
}



ChipEatenRemover::ChipEatenRemover(Game::Square* sq, float timeToEat)
	: GameFieldController("ChipEatenRemover", 1/timeToEat, GameField::Get())
{
	local_time = 0.f;

	int color=0;
	if (sq->GetChip().IsLicorice())
	{
		color = Game::LICORICE;
	} else if (sq->GetChip().IsSimpleChip())
	{
		color = sq->GetChip().GetColor();
	}

	_uv = Game::GetChipRect(color, false, false, false);

	_pos = sq->GetCellPos();

	_sizeCoeff = 1;
	_dsize = 1;

	_dx = GameSettings::SQUARE_SIDE/2;
	_dy = GameSettings::SQUARE_SIDE/2;
}

void ChipEatenRemover::Update(float dt)
{
	local_time += dt * time_scale;
	_sizeCoeff -= _dsize * dt * time_scale;

	//сначала двинаем просто вниз, а потом еще и влево
	if (local_time < 0.5)
	{
		_pos.x += _dx * dt * time_scale;
	} else {
		_pos.x -= _dx * dt * time_scale;
	}
}

void ChipEatenRemover::Draw()
{
	Render::device.PushMatrix();

	Render::device.MatrixTranslate(_pos);

	Render::BeginAlphaMul(math::clamp(0.f, 1.f, 1 - local_time));

	Render::device.MatrixScale(_sizeCoeff);

	Game::ChipColor::chipsTex->Draw(Game::ChipColor::DRAW_FRECT, _uv);

	Render::device.PopMatrix();

	Render::EndAlphaMul();
}

bool ChipEatenRemover::isFinish()
{
	return local_time > 1;
}

/*
* FlashAnimationPlayer
*
*/

FlashAnimationPlayer::FlashAnimationPlayer(const Game::AnimRes &res, FPoint pos, DrawType draw_type)
	: GameFieldController("FlashAnimationPlayer", 1.f, GameField::Get())
	, _animation(new FlashAnimation(res.lib, res.source))
	, _pos(pos)
	, _mirror(res.mirror)
	, _draw_type(draw_type)
{
	_animation->Reset();
	_animation->GetMovieClip()->setPlaybackOperation(GameLua::getPlayOnceOperation());
	_animation->SetHotSpot(res.inner_offset.x, res.inner_offset.y);
}

void FlashAnimationPlayer::Update(float dt)
{
	_animation->Update(dt);
}

void FlashAnimationPlayer::Draw()
{
	if(_draw_type == DRAW_UP)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(_pos);
		Game::MatrixSquareScale();
		_animation->Draw(0.f, 0.f, _mirror);
		Render::device.PopMatrix();
	}
}

void FlashAnimationPlayer::DrawAbsolute()
{
	if(_draw_type == DRAW_ABSOLUTE)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(_pos);
		Game::MatrixSquareScale();
		_animation->Draw(0.f, 0.f, _mirror);
		Render::device.PopMatrix();
	}
}

bool FlashAnimationPlayer::isFinish()
{
	return _animation->IsLastFrame();
}


