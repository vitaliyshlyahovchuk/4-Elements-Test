#include "stdafx.h"
#include "Match3Spirit.h"
#include "GameField.h"
#include "Energy.h"
#include "stdafx.h"
#include "StripEffect.h"
#include "SomeOperators.h"
#include "MyApplication.h"
#include "SplineBezier.h"
#include "EditorUtils.h"

namespace Game
{
	const float WING_SPEED = 4.5f*(math::PI*2.f); //Частота маха

	static int waitingBonusCount = 0;
	static std::set<Game::FieldAddress> flyBonuses;

	void Spirit::ResetCounters()
	{
		flyBonuses.clear();
		waitingBonusCount = 0;
	}

	bool Spirit::BonusIsFlyingToCell(Game::FieldAddress fa)
	{
		return flyBonuses.count(fa);
	}

	bool Spirit::BonusIsFlying()
	{
		return (waitingBonusCount > 0) || !flyBonuses.empty();
	}

	/*
		SpiritStrip
	*/

	SpiritStrip::SpiritStrip()
		: _stripTime(0.0f)
		, _numSectors(0)
		, _usedStripLenght(0.2f)
		, _currentStripLenght(0.2f)
	{
	}

	void SpiritStrip::Clear()
	{
		_buffer.Reinit(0, 0);
		_buffer._ibuffer.clear();
		_buffer._buffer.clear();
	}

	void SpiritStrip::SetSplinePath(SplinePath<math::Vector3> &path, float strip_relative_lenght)
	{
		_path = path;
		_fullLenght = 0.f;
		FPoint prev(_path.getGlobalFrame(0.f).x, _path.getGlobalFrame(0.f).y); 
		for(size_t i = 1; i <= 100; i++)
		{
			float t = float(i)/100.f;
			FPoint p(_path.getGlobalFrame(t).x, _path.getGlobalFrame(t).y);
			_fullLenght += p.GetDistanceTo(prev);
			prev = p;
		}
		_currentStripLenght = strip_relative_lenght;

		_numSectors = 20;
		_buffer._buffer.resize((_numSectors + 1)*2);

		int numTriangles = _numSectors * 2;
		_buffer._ibuffer.reserve(numTriangles * 3);
		for (int i = 0; i < numTriangles ; ++i) {
			_buffer._ibuffer.push_back(i);
			_buffer._ibuffer.push_back(i+1);
			_buffer._ibuffer.push_back(i+2);
		}
		_buffer.numVertices = _buffer._buffer.size();
		_buffer.numIndices = _buffer._ibuffer.size();
	}


	void SpiritStrip::setStripTime(float stripTime, float scale_lenght, float offset_back)
	{
		_usedStripLenght = math::lerp(_currentStripLenght, _currentStripLenght*2.f, stripTime)*scale_lenght;
		_stripTime = stripTime - offset_back*_usedStripLenght;

		for (int i = 0; i <= _numSectors; i++)
		{
			float t = float(i)/_numSectors;
			Render::QuadVert &qv0 = _buffer._buffer[i*2];
			Render::QuadVert &qv1 = _buffer._buffer[i*2+1];
			qv0.z = 0.f;
			qv1.z = 0.f;

			//1. Самое простое вычислить текстурные координаты
			qv0.u = 0.f; qv0.v = t;
			qv1.u = 1.f; qv1.v = t;

			//2. Вычисляем пространственное положение
			float t_in = math::clamp(0.f, 1.f, _stripTime + _usedStripLenght*t);
			math::Vector3 v = _path.getGlobalFrame(t_in);
			FPoint dir = FPoint(_path.getGlobalGradient(t_in).x, _path.getGlobalGradient(t_in).y).Normalized();
			FPoint dir0 = dir.Rotated( math::PI*0.5f)*v.z*0.5f;
			FPoint dir1 = dir.Rotated(-math::PI*0.5f)*v.z*0.5f;
			qv0.x = v.x + dir0.x;
			qv0.y = v.y + dir0.y;

			qv1.x = v.x + dir1.x;
			qv1.y = v.y + dir1.y;
			//3. Цвет
			Color clr = Color::WHITE;
			clr.alpha = math::lerp<unsigned int>(0, clr.alpha, 1 - (t_in - 0.85f)/0.15f);
			//clr.alpha = math::lerp<unsigned int>(0, clr.alpha, t_in/0.1f);
			qv0.color = clr;
			qv1.color = clr;
		}
	}

	float SpiritStrip::getHeadTime()
	{
		return math::clamp(0.f, 1.f, _stripTime + 0.95f*_usedStripLenght);
	}

	FPoint SpiritStrip::getStripPartPosition(float t_part)
	{
		float t = math::clamp(0.f, 1.f, _stripTime + _usedStripLenght*t_part);
		math::Vector3 v = _path.getGlobalFrame(t);
		return FPoint(v.x, v.y);	
	}

	FPoint SpiritStrip::getStripPosition()
	{
		float t = math::clamp(0.f, 1.f, _stripTime + 0.95f*_usedStripLenght);
		math::Vector3 v = _path.getGlobalFrame(t);
		return FPoint(v.x, v.y);
	}

	FPoint SpiritStrip::getStripGradient()
	{
		return getStripGradient(_stripTime, 1.f);
	}

	FPoint SpiritStrip::getStripGradient(float time, float limit)
	{
		float t = math::clamp(0.f, 1.f, math::clamp(0.f, limit, time + 0.95f*_usedStripLenght));
		math::Vector3 v = _path.getGlobalGradient(t);
		return FPoint(v.x, v.y);
	}

	void SpiritStrip::Draw(FRect frect, float alpha, bool need_morphing, FRect rect , FPoint morphing_pos, float morphing_angle )
	{
		static VertexBufferIndexed tmp;

		tmp.isStatic = true;
		tmp._buffer = _buffer._buffer;
		tmp._ibuffer = _buffer._ibuffer;
		tmp.numIndices = _buffer.numIndices;
		tmp.numVertices = _buffer.numVertices;
		tmp._quadBuffer = _buffer._quadBuffer;
		//tmp.vertexAttribs = _buffer.vertexAttribs;
    
		//FPoint v0, v1, v2, v3;
		float t_transform = math::clamp(0.f, 1.f, _stripTime/0.05f);
		if(need_morphing)
		{
			rect.MoveBy(morphing_pos);
		//	float agnle = morphing_angle*math::clamp(0.f, 1.f, t_transform*4.f)*4.f;
		//	v0 = rect.LeftBottom().Rotated(agnle) + morphing_pos;
		//	v1 = rect.LeftTop().Rotated(agnle) + morphing_pos;
		//	v2 = rect.RightBottom().Rotated(agnle) + morphing_pos;
		//	v3 = rect.RightTop().Rotated(agnle) + morphing_pos;
		}

		for (int i = 0; i <= _numSectors; ++i)
		{
			float t = float(i)/_numSectors;

			Render::QuadVert &qv0 = tmp._buffer[i*2];
			Render::QuadVert &qv1 = tmp._buffer[i*2+1];

			if(need_morphing)
			{
				//qv0.x = math::lerp(v0.x, qv0.x, t_transform);
				//qv0.y = math::lerp(math::lerp(v0.y, v1.y, t), qv0.y, t_transform);

				//qv1.x = math::lerp(v2.x, qv1.x, t_transform);
				//qv1.y = math::lerp(math::lerp(v2.y, v3.y, t), qv1.y, t_transform);

				qv0.x = math::lerp(rect.xStart, qv0.x, t_transform);
				qv0.y = math::lerp(math::lerp(rect.yStart, rect.yEnd, t), qv0.y, t_transform);

				qv1.x = math::lerp(rect.xEnd, qv1.x, t_transform);
				qv1.y = math::lerp(math::lerp(rect.yStart, rect.yEnd, t), qv1.y, t_transform);
			}

			qv0.u = math::lerp(frect.xStart, frect.xEnd, qv0.u);
			qv1.u = math::lerp(frect.xStart, frect.xEnd, qv1.u);

			qv0.v = math::lerp(frect.yStart, frect.yEnd, qv0.v);
			qv1.v = math::lerp(frect.yStart, frect.yEnd, qv1.v);

			qv0.color.alpha = math::lerp<unsigned int>(0, qv0.color.alpha, alpha);
			qv1.color.alpha = qv0.color.alpha;
		}

		tmp.UploadVertex();
		tmp.UploadIndex();
		Render::device.DrawIndexed(&tmp);

		tmp.Reinit(0, 0);
		tmp._ibuffer.clear();
		tmp._buffer.clear();

	}

	/*
		Spirit
	*/

	Spirit::EyeBehaviour::EyeBehaviour(float t, FPoint p, bool change_eye_textures_)
		: time(t)
		, dir(p)
		, change_eye_textures(change_eye_textures_)
	{			
	}

	Spirit::Spirit(FPoint pos, IPoint index_from,const std::string& bonusChip, const Game::Hang& bonus, const AddressVector& chipSeq, Game::FieldAddress to, float pause, int color, Game::ChipColor chip, bool in_ice_chip)
		: GameFieldController("Spirit", 1.f, GameField::Get())
		, _localTime(0.f)
		, _texBody(Core::resourceManager.Get<Render::Texture>("SpiritBody"))
		, _texWing(Core::resourceManager.Get<Render::Texture>("SpiritWing"))
		, _texFlap(Core::resourceManager.Get<Render::Texture>("SpiritFlap"))
		, _texEyeLeft(Core::resourceManager.Get<Render::Texture>("SpiritEyeLeft"))
		, _texEyeRight(Core::resourceManager.Get<Render::Texture>("SpiritEyeRight"))
		, _texEyeLeft2(Core::resourceManager.Get<Render::Texture>("SpiritEyeLeft2"))
		, _texEyeRight2(Core::resourceManager.Get<Render::Texture>("SpiritEyeRight2"))
		, _texStrip(Core::resourceManager.Get<Render::Texture>("SpiritStrip"))	
		, _texBrow(Core::resourceManager.Get<Render::Texture>("SpiritBrow"))	
		, _state(SPIRIT_DELAY)
		, _bonusChip(bonusChip)
		, _bonus(bonus)
		, _posFrom(pos)
		, _timerState(0.f)
		, _timerScale(1.f/(pause + Game::ChipColor::CHIP_START_HIDE + 0.1f))
		, _startWaitOffset(math::random(0.f, 100.f))
		, _destSquare(0)
		, _hangIsSend(false)
		, _finishEffectIsRun(false)
		, _endEffect(0)
		, _stripEffect(0)
		, _eyesDistance(0.f)
		, _eyesAngle(0.f)
		, _bodyByEyeOffset(0.f, 0.f)
		, _eyeOutro(false)
		, _eyePosLast(0.f, 0.f)
		, _eyePos(0.f, 0.f)
		, _timerEyeVariant(0.f)
		, _eyeVariant2Enable(false)
		, _strip()
		, _showChip(color >= 0)
		, _chip(chip)
		, _chipInIce(in_ice_chip)
		, _autoTrigger(false)
	{
		_chipFRect = Game::GetChipRect(color, true, false, false);
		
		//Эта последовательность нужна для механизма определения клетки в которую полетит бонус (который зависит от настроек уровня)
		if(chipSeq.size() >= 2 ) {
			_chipSeq.push_back( chipSeq[chipSeq.size()-2] );
			_chipSeq.push_back( chipSeq[chipSeq.size()-1] );
		} else {
			Assert( to.IsValid() );
		}

		if( to.IsValid() ){
			_destSquare = GameSettings::gamefield[to];
			_destSquare->SetHangBuzy(true);
		}

		_spline.Clear();
		float angle_start = 90.f;
		float angle_full = 360.f;
		if( (math::random(100) < 50 &&  _posFrom.x - 150.f > GameSettings::FIELD_SCREEN_CONST.LeftBottom().y ) || _posFrom.x + 150 > GameSettings::FIELD_SCREEN_CONST.RightTop().x )
		{
			angle_full = -angle_full;
		}

		//Разнообразим полет: справа, слева, но с проверкой на заезды за края экрана
		if(_posFrom.y + 300.f > GameSettings::FIELD_SCREEN_CONST.LeftTop().y)
		{
			//Летим вниз
			angle_full *= -0.5f; //Если дух близко к верхней границе, то ему нужно пролететь половину пути, плюс тогда меняется знак угла (в какую сторону лететь вправо? влево?) для того чтобы не залететь за границу
		}

		for(size_t i = 0; i <= 20; i++)
		{
			float t = (i + 0.f)/20.f;
			float anlge = math::lerp(angle_start, angle_start + angle_full, t);
			float r = math::lerp(0.f, 200.f, math::sqrt(t));
			float t2 = math::clamp(0.f, 1.f, t/0.4f);
			t2 = math::sin(t2*math::PI*0.5f);
			_spline.addKey(_posFrom + FPoint(r, 0.f).Rotated(anlge*math::PI/180.f) + FPoint(0.f, 40.f + 50.f*t2));
		}
		_spline.CalculateGradient();

		DistributeByLong(_spline);

		_posFly = _spline.getGlobalFrame(1.f);

		waitingBonusCount++;

		_lastAppearDir =  _spline.getGlobalGradient(1.f).Normalized()*100.f*(_posFly.GetDistanceTo(_posFrom)/300.f);
		FPoint center = math::lerp(GameSettings::FIELD_SCREEN_CONST.LeftBottom(), GameSettings::FIELD_SCREEN_CONST.RightTop(), 0.5f);

		_startAcceleration.Clear();
		SplinePath<math::Vector3> path;
		size_t count = 20;

		for(size_t i = 0; i <= count; i++)
		{
			float t = (i+0.f)/count;
			FPoint p = _spline.getGlobalFrame(math::clamp(0.f, 1.f, t));

			path.addKey(math::Vector3(p.x, p.y, math::lerp(GameSettings::SQUARE_SIDEF, 40.f, (t - 0.1f)/0.05f)));
			float ta = 0.f;
			if(t > 0.5f)
			{
				ta = sinf(math::clamp(0.f, 1.f, (t - 0.5f)/0.2f)*math::PI);
			}
			_startAcceleration.addKey(ta);
		}
		////Инертная остановка
		//for(size_t i = 1; i <= 7;i++)
		//{
		//	float t = float(i)/7.f;
		//	FPoint p =  _spline.getGlobalFrame(1.f) + _lastAppearDir*sinf(t*math::PI);
		//	path.addKey(math::Vector3(p.x, p.y, 40.f));
		//}
		path.CalculateGradient();

		_strip.SetSplinePath(path, 5.f/_spline.keys.size());

		_startAcceleration.CalculateGradient();
		InitEye(0.1f);
		_eyeTimer = -1.f; //След. анимацию включим через 1с. И дух тоже успеет приготовиться (0.5с) к исчезновению.

	}

	Spirit::~Spirit()
	{
		if(_endEffect)
		{
			_endEffect = 0;
		}
		if(_stripEffect)
		{
			_stripEffect = 0;
		}
		if(_state == SPIRIT_APPEAR || _state == SPIRIT_DELAY)
		{
			waitingBonusCount--;
		}
	}

	bool Spirit::isFinish()
	{
		return _state == SPIRIT_FINISHED && _endEffect && _endEffect->isEnd();
	}

	void Spirit::Update(float dt)
	{
		_localTime += dt;
		
		_effCont.Update(dt);
		if(_eyeVariant2Enable && _timerEyeVariant < 1)
		{
			_timerEyeVariant += dt*2.f;
			if(_timerEyeVariant >= 1.f)
			{
				_timerEyeVariant = 1.f;
			}
		}
		if(_state == SPIRIT_DELAY)
		{
			_timerState += dt*_timerScale;
			if(_timerState > 1)
			{
				_timerState = 0.f;
				_state = SPIRIT_APPEAR;
				_idSample = MM::manager.PlaySample("SpiritAppear");
				//Эффект появления
				ParticleEffectPtr eff = Game::AddEffect(gameField->_effTopCont, "SpiritAppear");
				eff->SetPos(GameSettings::ToFieldPos(_posFrom));
				eff->Reset();
			}
			_strip.setStripTime(0.f, 1.f, 0.f);
		}else if(_state == SPIRIT_APPEAR)
		{
			if(true)
			{
				_timerState += dt*1.f;
				if(_timerState > 1)
				{
					_state = SPIRIT_FLY;
					_idSample = MM::manager.PlaySample("SpiritFly", true);
					_timerState = 0.f;
					_stripEffect = Game::AddEffect(_effCont, "SpiritFly");
					_stripEffect->SetPos(_strip.getStripPosition());
					_stripEffect->Reset();
				}
			}else{
				_timerState = float(Core::mainInput.GetMousePos().x)/MyApplication::GAME_WIDTH;
			}
			float t = math::ease(_timerState, 0.2f, 0.4f);
			_strip.setStripTime(t, 1.f, 0.f);
		}else if(_state == SPIRIT_FLY){
			_timerState += dt;
			UpdateEyes(dt);
			if(_timerState > 0.05f && gameField->IsStandby())
			{
				bool camera_stand = Gadgets::freeFrontDetector.CameraIsStand() || !Energy::field.EnergyIsFlowing();					
				if(camera_stand)
				{
					if(!_eyeOutro)
					{
						_eyePosLast = _eyePos; //Запоминаем последнюю позицию глаз
						_eyeBehaviour.clear();
						//Сначала оглядываемся
						int rnd = math::random(100);
						int count = 100/2;
						if(rnd < count*1.5)
						{
							_eyeBehaviour.push_back(EyeBehaviour(math::random(0.25f, 0.25f), FPoint(-200.f, 50.f)));
							_eyeBehaviour.push_back(EyeBehaviour(math::random(0.25f, 0.25f), FPoint( 200.f, 50.f)));
						}else{
							_eyeBehaviour.push_back(EyeBehaviour(math::random(0.25f, 0.25f), FPoint(50.f, -200.f)));
							_eyeBehaviour.push_back(EyeBehaviour(math::random(0.25f, 0.25f), FPoint(50.f,  200.f)));
						}
						FPoint _toPos = _posFly;
						if(CheckSquareDest())
						{
							_toPos = GameSettings::ToScreenPos(FPoint(_destSquare->address.ToPoint())*GameSettings::SQUARE_SIDEF);
						}
						_eyeBehaviour.push_back(EyeBehaviour(0.6f, _toPos - _posFly, true));
						_eyeTimer = 0.f;
						_eyeTimeScale = 1.f;
						_eyeOutro = true;						
					}else if(_eyeTimer < 0) //Ждем пока проиграется анимация глаз
					{ //Потом улетаем
						if(InitHangState())
						{
							_state = SPIRIT_DISAPPEAR;
							//заменяем звук
							if (_idSample) 
							{
								MM::manager.FadeSample(_idSample, 0.5f);
								_idSample = 0;
							}
							MM::manager.PlaySample("SpiritLanded");
							waitingBonusCount--;
							_timerState = 0.f;
						}else{
							_eyeOutro = false;
							_eyeVariant2Enable = false;
							_timerEyeVariant = 0.f;
						}
					}
				}
			}
		}else if(_state == SPIRIT_DISAPPEAR)
		{
			_timerState += dt*1.5f;
			if(true)
			{
				if(_timerState > 0.8f && !_finishEffectIsRun)
				{
					_finishEffectIsRun = true;
					_endEffect = Game::AddEffect(gameField->_effCont, "SpiritFinish");
					_endEffect->SetPos(GameSettings::ToFieldPos(_finishPos));
					_endEffect->Reset();
				}
				if(!_hangIsSend)
				{	
					if(!Game::CanHangBonusOnSquare(_destSquare, true) && !_autoTrigger)
					{
						// с фишкой, на которую летели, что-то случилось, пытаемся найти другую
						flyBonuses.erase(_destSquare->address);
						if(_destSquare)
						{
							_destSquare->SetHangBuzy(false);
							_destSquare = 0;
						}
						if( !InitHangState() ) {
							// другой фишки не нашлось, переходим опять в режим ожидания,
							// чтобы когда поле успокоится попробовать снова
							_timerState = 0.0f;
							_state = SPIRIT_FLY;
							waitingBonusCount++;
						}
					}
					if(_timerState > 0.98f)
					{
						_hangIsSend = true;
						if(_autoTrigger)
						{
							Game::AddController(new CombinedBonus(_destSquare->address, _bonus, GameField::Get(), false));
						}
						else if( Game::CanHangBonusOnSquare(_destSquare, true) )
						{
							flyBonuses.erase(_destSquare->address);
							_destSquare->SetHangForChip(_bonus);
							//Проверим правильно ли приземлились на всякий случай, и запустим перелет если нужно.
							GameField::Get()->MoveBonusesToViewRect();
						}
					}
				}
			}else{
				_timerState = float(Core::mainInput.GetMousePos().x)/MyApplication::GAME_WIDTH;
			}
			float t = math::ease(_timerState, 0.5f, 0.5f);
			_strip.setStripTime(t, math::clamp(0.f, 1.f, (t-0.2f)/0.3f), 1.f);
			if(_timerState > 1)
			{
				_state = SPIRIT_FINISHED;
				_timerState = 1.f;
			}		
		}
	}

	FPoint Spirit::GetNearOffset()
	{
		float t = _localTime;
		FPoint pos(0.f, 0.f);
		pos.x += 30.f*cosf(t*0.4f + math::PI*2.f/_startWaitOffset);
		pos.y += 20.f*sinf(t*0.3f + _startWaitOffset);
		pos.y -= 20.f*sinf(t*0.2f + _startWaitOffset*2.f);

		pos.y += 5.f*sinf(t*6.f);
		return pos;
	}

	void Spirit::DrawWingsRight(float t_inert, float t_acceletate)
	{
		float angleWing = 40.f*sinf(_localTime*WING_SPEED) - 20.f;
		float angleFlap = 30.f*sinf(_localTime*WING_SPEED - math::PI*0.2f) + 10;

		angleWing = math::lerp(angleWing, 60.f, t_inert);
		angleFlap = math::lerp(angleFlap, 130.f, t_inert);

		angleWing = math::lerp(angleWing, -70.f, math::clamp(0.f, 1.f, t_acceletate*2.f));
		angleFlap = math::lerp(angleFlap, -50.f, math::clamp(0.f, 1.f, t_acceletate*2.f));
		
		Render::device.MatrixScale(1.2f);
		{//Подкрылок
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(math::Vector3(15.f, 0.f, 0.f));
			Render::device.MatrixRotate(math::Vector3(0.f, 0.f, 1.f), angleFlap);
			_texFlap->Draw(FPoint(-9,-25));
			Render::device.PopMatrix();
		}	
		{//Большое крыло
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(math::Vector3(13.f, 0.f, 0.f));
			Render::device.MatrixRotate(math::Vector3(0.f, 0.f, 1.f), angleWing);
			_texWing->Draw(FPoint(-8,-8));
			Render::device.PopMatrix();
		}
	}

	void Spirit::InitEye(float max_wait_time)
	{
		_eyeTimer = -math::random(0.2f, 4.f)*max_wait_time;
		int rnd = math::random(100);
		int count = 100/12;
		if(rnd < count*1)
		{
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(-200.f, 50.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint( 200.f, 50.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, FPoint(0.f, 0.f)));
		}else if(rnd < count*2){
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(0.f, 200.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(0.f, -200.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, FPoint(0.f, 0.f)));
		}else if(rnd < count*3){
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(-200.f, 200.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(200.f, -200.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, FPoint(0.f, 0.f)));
		}else if(rnd < count*4){
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(-200.f, -200.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(200.f, 200.f)));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, FPoint(0.f, 0.f)));
		}else if(rnd < count*5){
			_eyeBehaviour.push_back(EyeBehaviour(0.3f, FPoint(math::random(-200.f, 200.f), math::random(-200.f, 200.f))));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, FPoint(0.f, 0.f)));
		}else if(rnd < count*8){
			FPoint p = FPoint(0.f, math::random(50.f, 300.f)).Rotated(math::random(math::PI*2.f));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, p));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, p.Rotated(math::PI)));
		}else{
			FPoint p = FPoint(0.f, math::random(50.f, 300.f)).Rotated(math::random(math::PI*2.f));
			_eyeBehaviour.push_back(EyeBehaviour(0.4f, p));
		}
		_eyeTimeScale = math::random(0.2f, 1.2f);
	}

	void Spirit::UpdateEyes(float dt)
	{
		FPoint center = _pos;
		FPoint view_pos = Core::mainInput.GetMousePos();
		_eyePos = view_pos - center;

		if(true)
		{
			if(_eyeTimer < 0)
			{
				_eyeTimer += dt;
				_eyePos = FPoint(0.f, 0.f);
			}else{
				if(_eyeBehaviour.empty())
				{
					InitEye(1.f);
				}else{
					EyeBehaviour &behaviour = _eyeBehaviour.front();
					_eyeTimer += dt/behaviour.time;
					float t = math::clamp(0.f, 1.f, _eyeTimer);
					_eyePos = math::lerp(_eyePosLast, behaviour.dir, t);
					if(_eyeTimer > 1)
					{
						_eyeTimer = 0.f;
						_eyeBehaviour.pop_front();
						_eyePosLast = _eyePos;
						if(!_eyeBehaviour.empty() && _eyeBehaviour.front().change_eye_textures)
						{
							_eyeVariant2Enable = true; //Пора включить второй вариант глаз (когда смотрим на клетку в которую сейчас полетим)
						}
					}
				}
			}
		}
		
		_eyesDistance = math::clamp(0.f, 1.f, _eyePos.GetDistanceToOrigin()/100.f);
		if(_eyesDistance > 0)
		{
			_eyesAngle = FPoint(1.f, 0.f).GetDirectedAngleNormalize(_eyePos);
		}
	}

	void RenderSprite(Render::Texture *tex, float scale, FPoint offset, FPoint inner_offset)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(offset);
		Render::device.MatrixScale(scale);
		tex->Draw(inner_offset);
		Render::device.PopMatrix();	
	}

	FPoint Spirit::DrawEyes()
	{

		FPoint pos_left(-8.f, -4.f);
		FPoint pos_right(8.f, -4.f);


		FPoint dir = FPoint(1.f, 0.f).Rotated(_eyesAngle);
		FPoint offset = _eyesDistance*2.f*dir;
		offset.x *= 2.f;
		offset.y *= 5.f;

		float scale_left  = 1.f + 0.6f*math::clamp(0.f, 1.f, -dir.x)*_eyesDistance;
		float scale_right = 1.f + 0.6f*math::clamp(0.f, 1.f,  dir.x)*_eyesDistance;

		pos_left  += offset*scale_left;
		pos_right += offset*scale_right;
		
		FPoint pos_center = math::lerp(pos_left, pos_right, 0.5f);
		pos_center.y *= 0.7f; //По оси оУ сильно водить лицом не надо
		RenderSprite(_texBrow, 0.9f, pos_center, FPoint(-30.f, -15.f));
		if(_timerEyeVariant < 1)
		{
			Render::BeginAlphaMul(math::clamp(0.f, 1.f, 1 - _timerEyeVariant));
			RenderSprite(_texEyeLeft, scale_left, pos_left, FPoint(-8,-10));
			RenderSprite(_texEyeRight, scale_right, pos_right , FPoint(-12,-10));
			Render::EndAlphaMul();
		}
		if(_timerEyeVariant > 0)
		{
			Render::BeginAlphaMul(math::clamp(0.f, 1.f, _timerEyeVariant));
			RenderSprite(_texEyeLeft2, scale_left, pos_left, FPoint(-8,-10));
			RenderSprite(_texEyeRight2, scale_right, pos_right , FPoint(-12,-10));
			Render::EndAlphaMul();
		}
		return pos_center*0.5f;
	}

	void Spirit::DrawAbsolute()
	{
		FPoint near_offset(0.f, 0.f);
		near_offset.y += 3.f*sinf(_localTime*WING_SPEED);
		float scale_x = 1.f, scale_y = 1.f;
		float alpha = 0.f;
		float alpha_effect = 0.f;
		float t_acceletate = 0.f;

		float t_inert =  0.f; 
		float correct_angle_inert = 0.f;
		float t_vibr = 1.f;

		float alpha_chip = 0.f;
		float alpha_strip = 0.f;

		float chip_scale_y = 1.f;

		FPoint chip_pos = _posFrom;

		FPoint eyes_offset = FPoint(0.f, 2.f*sinf(_localTime*WING_SPEED));
		if(_state == SPIRIT_DELAY)
		{
			alpha_chip = 1.f;

			float t = math::clamp(0.f, 1.f, (_timerState - 0.6f)/0.4f);
			//t = math::ease(t, 1.f, 0.f);
			t = t*t;

			chip_scale_y -= 0.3f*math::sin(t*math::PI);

			chip_pos.y += 40.f*t;
			chip_pos.y -= 30.f*math::sin(t*math::PI);
		}else if(_state == SPIRIT_APPEAR)
		{
			float part = math::clamp(0.f, 1.f, _timerState/0.3f);
			chip_pos = _strip.getStripPartPosition(part);

			float t = math::ease(_timerState, 0.2f, 0.4f);

			chip_scale_y = math::lerp(1.f, 2.f, t*10.f);

			alpha_chip  = math::clamp(0.f, 1.f, 1 - (t - 0.2f)/0.25f);
			alpha_strip = math::clamp(0.f, 1.f, (t-0.1f)*4.f);

			_pos = _strip.getStripPosition();
			
			float t_scale = math::clamp(0.f, 1.f, t*1.6f);
			scale_x = t_scale;
			scale_y = t_scale + 0.3f*sinf(t_scale*math::PI);

			alpha = math::clamp(0.f, 1.f, (_timerState - 0.25f)*3.f);
			alpha_effect = alpha;
			if(_timerState < 0.6f)
			{
				t_inert = 1.f;
			}else{
				t_inert = 1 - (_timerState - 0.6f)/0.3f;
			}
			float angle_dir = FPoint(0.f, -1.f).GetDirectedAngleNormalize(_strip.getStripGradient(t, 1.f))*180.f/math::PI;
			if(angle_dir > 180.f)
			{
				angle_dir -=  360.f; //Идем по кратчайшему развороту
			}
			correct_angle_inert = math::lerp(angle_dir, 0.f, (t - 0.6f)/0.1f); // В конце возвращаем угловое положение на место
			eyes_offset *= _timerState;
			t_acceletate = _startAcceleration.getGlobalFrame(t);

			t_vibr = math::clamp(0.f, 1.f, (t - 0.5f)/0.4f);

			eyes_offset *= t_vibr;
			eyes_offset += FPoint(0, 10.f)*t_acceletate;

		}else if(_state == SPIRIT_FLY)
		{
			_pos = _posFly + math::lerp(FPoint(0.f, 0.f), GetNearOffset(), _timerState);
			alpha = 1.f;
			alpha_effect = 1.f;		
			alpha_chip = 0.f;
			alpha_strip = 0.f;
		}else if(_state == SPIRIT_DISAPPEAR)
		{
			float t = math::ease(_timerState, 0.5f, 0.5f);
			_pos = _strip.getStripPosition();
			alpha = 1 - math::clamp(0.f, 1.f, (t - 0.8f)/0.15f);
			alpha_effect = 1 - t;
			t_inert = _finishAcceleration.getGlobalFrame(t);
			if(_hangIsSend)
			{
				alpha = 0.f;
			}
			float t_angle = _strip.getHeadTime();
			correct_angle_inert = _angles.getGlobalFrame(t_angle);
			t_acceletate = _startAcceleration.getGlobalFrame(t);
			t_vibr = math::clamp(0.f, 1.f, 1 - t*4.f);
			eyes_offset *= t_vibr;
			eyes_offset += FPoint(0, 10.f)*t;

			scale_x = 1 - t_inert*0.4f; 
			scale_y = 1 + t_inert*0.2f; 
			alpha_strip = math::clamp(0.f, 1.f, 1 - (t - 0.96f)/0.04f);
			alpha_chip = 0.f;
		}

		if(t_acceletate > 0)
		{
			scale_y *= (1 + t_acceletate*0.25f);
			scale_x *= (1 - t_acceletate*0.1f);
			t_vibr *= 1 - sinf(t_acceletate*math::PI);
		}

		if(alpha_chip > 0 && _showChip)
		{
			if(_state == SPIRIT_DELAY && _chip.IsExist())
			{
				Render::BeginAlphaMul(math::clamp(0.f, 1.f, 1 - (_timerState-0.4f)/0.3f));
				_chip.DrawWhiteBorder(chip_pos - GameSettings::CELL_HALF, _chipInIce);
				Render::EndAlphaMul();
			}
			ChipColor::chipsTex->Bind();			
			
			FRect rect = Game::ChipColor::DRAW_FRECT;
			rect.yEnd = rect.yStart + rect.Height()*chip_scale_y;
			_strip.Draw(_chipFRect, alpha_chip, true, rect.MovedBy(-GameSettings::CELL_HALF), chip_pos, correct_angle_inert*math::PI/180.f);
		}

		if(alpha_strip > 0)
		{
			_texStrip->Bind();
			_strip.Draw( FRect(0.f, 1.f, 0.f, 1.f), alpha_strip);
		}

		//return;

		Render::BeginAlphaMul(alpha_effect);
		if(_stripEffect.get())
		{
			_stripEffect->SetPos(_pos);
		}
		_effCont.Draw();
		Render::EndAlphaMul();

		Render::BeginAlphaMul(alpha);

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(_pos + near_offset*t_vibr);
		Render::device.MatrixRotate(math::Vector3(0.f, 0.f, 1.f), correct_angle_inert);
		Game::MatrixSquareScale();
		Render::device.MatrixScale(scale_x, scale_y, 1.f);

		//Тело
		Render::device.PushMatrix();
		Render::device.MatrixScale(0.9f);
		_texBody->Draw(FPoint(-35,-31) + _bodyByEyeOffset +  FPoint(0.f, 4.f*sinf(_localTime*WING_SPEED - math::PI*0.8f)*t_vibr));
		_texBody->Draw(FPoint(-35,-31) + _bodyByEyeOffset);
		Render::device.PopMatrix();

		if(true)
		{
			//Крылья справа
			Render::device.PushMatrix();
			DrawWingsRight(t_inert, t_acceletate);
			Render::device.PopMatrix();


			//Крылья слева
			Render::device.PushMatrix();
			Render::device.MatrixScale(-1.f, 1.f, 1.f);
			DrawWingsRight(t_inert, t_acceletate);
			Render::device.PopMatrix();
		}

		//Глаза
		Render::device.PushMatrix();
		Render::device.MatrixScale(0.9f);
		Render::device.MatrixTranslate(eyes_offset + _bodyByEyeOffset);
		_bodyByEyeOffset = DrawEyes();
		Render::device.PopMatrix();


		Render::device.PopMatrix();
		Render::EndAlphaMul();
	}

	bool Spirit::CheckSquareDest()
	{
		if(_chipSeq.empty())
		{
			return false;
		}

		if(!_destSquare || !Game::CanHangBonusOnSquare(_destSquare, true) || !Game::activeRect.Contains(_destSquare->address.ToPoint()))
		{
			std::vector<Game::Square*> bonusSquares = Game::ChooseSquaresForBonus(_bonusChip, 1, _chipSeq);
			if( bonusSquares.empty() )
			{
				return false;
			}
			//if(EditorUtils::draw_debug && Game::isVisible(114, 44))
			//{
			//	bonusSquares[0] = GameSettings::gamefield[IPoint(114, 44)];
			//}
			_destSquare = bonusSquares[0];
			_destSquare->SetHangBuzy(true);
		}
		return true;
	}


	bool Spirit::InitHangState()
	{
		if(_destSquare && Game::activeRect.Contains(_destSquare->address.ToPoint()))
		{ //Если клетка есть и она на экране, то летим туда
		
		}else if(!CheckSquareDest())
		{
			//Иначе придется найти другую клетку
			return false;		
		}
		
		Assert(_destSquare);

		flyBonuses.insert(_destSquare->address);

		_finishPos = GameSettings::ToScreenPos(FPoint(_destSquare->address.ToPoint()) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF);

		float distance = _pos.GetDistanceTo(_finishPos);

		_spline.Clear();
		float max_distance = 300.f;
		float scale_dir = 1.f;
		if(distance > max_distance)
		{
			//Если расстояние позволяет(достаточно далеко)- то движемя по прямой
			_spline.addKey(_pos);
			_spline.addKey(_finishPos);
		}else{
			//Если не хватает, движемся по вписанной в крайние точки окружности, по дуге длинной max_distance

			//Для того чтобы двигаться по окружности нужно знать центр окружности
			//Чтобы знать центр окружности нужно знать радиус окружности
			//Радиус окружности будем вычислять по приближенной формуле (см R)
			//Идеальная формула получаетс явида  sin( f1(R)) = f2(R)) из которой R не выражается
			//Пользуясь формулой приближенного вычисления синуса второй итерации sin(x) = x - x^3/6, выразим R
			//Данного приближения вполне необходимо и достаточно
			//В идеальной формуле мы бы получили (angle_full == d_angle)
			//В реальности же (из-за приближения) их разница является показателем погрешности

			max_distance *= 1.5f; //Хак на то чтобы дуга была несколько больше (траектория будет красивее)

			float B = 0.5f*max_distance;
			float R = pow(B, 3.f)/6.f/(B - 0.5*distance);
			Assert(R > 0);
			R = math::sqrt(R);

			
			float h = math::sqrt(R*R - 0.25f*distance*distance);
			float angle_full = max_distance/R;
						

			FPoint circle_center = math::lerp(_pos, _finishPos, 0.5f);
			FPoint N = (_finishPos - _pos).Normalized();
			
			
			{//Центра как известно - может быть два, выбираем ближайший к центру
				FPoint c1 = circle_center + h*N.Rotated(math::PI*0.5f);
				FPoint c2 = circle_center + h*N.Rotated(math::PI*0.5f);
				FPoint center = math::lerp(GameSettings::FIELD_SCREEN_CONST.LeftBottom(), GameSettings::FIELD_SCREEN_CONST.RightTop(), 0.5f);
				if(center.GetDistanceTo(c1) < center.GetDistanceTo(c2))
				{
					circle_center = c1;
				}else{
					circle_center = c2;
				}				
			}

			float angle0 = (_pos - circle_center).GetAngle();
			float angle1 = (_finishPos - circle_center).GetAngle();

			if(angle1 < angle0)
			{
				angle1 += math::PI*2.f;
			}
			float d_angle = angle1 - angle0;
			float dd_angle1 = abs(d_angle - angle_full);
			float dd_angle2 = abs(d_angle - (2.f*math::PI -angle_full));
			if(dd_angle1 > dd_angle2) //Выбираем по какомй дуге требуется пойти, по маленькой или по большой
			{
				angle0 = angle0 + 2.f*math::PI;
			}

			//Строим плавный сплайн движения от одной точки до другой по дуге
			for(size_t i = 0; i <= 20; i++)
			{
				float t = (i + 0.f)/20.f;
				float angle = math::lerp(angle0, angle1, t);
				_spline.addKey(circle_center + FPoint(R, 0.f).Rotated(angle));
			}
			scale_dir = 2.f;
		}
		_spline.CalculateGradient();

		_strip.Clear();
		SplinePath<math::Vector3> path;
		_angles.Clear();
		_startAcceleration.Clear();
		_finishAcceleration.Clear();
		//2 Общитываем шлейф
		//2.1 разгон
		FPoint start_point = _spline.getGlobalFrame(0.f);
		FPoint dir_start = _spline.getGlobalGradient(0.f).Normalized()*60.f*(distance/max_distance)*scale_dir;
		float angle_prev = FPoint(0.f, -1.f).GetDirectedAngleNormalize(dir_start)*180.f/math::PI;
		if(angle_prev > 180.f)
		{
			angle_prev -= 360.f;
		}
		float count_start = 7.f;
		for(size_t i = 0; i < count_start; i++)
		{
			float t = i/count_start;
			float start_t = sinf(t*math::PI);
			path.addKey(math::Vector3( start_point.x - dir_start.x*start_t,  start_point.y - dir_start.y*start_t , 40.f));
			_angles.addKey(angle_prev*math::clamp(0.f,1.f, t*5.f));
			_startAcceleration.addKey(start_t);
			_finishAcceleration.addKey(0.f);
		}

		//2.2 Основной путь
		for(size_t i = 0; i <= 20; i++)
		{
			float t = (i+0.f)/20.f;
			FPoint p = _spline.getGlobalFrame(t);
			path.addKey(math::Vector3(p.x, p.y, 40.f));
			float angle = FPoint(0.f, -1.f).GetDirectedAngleNormalize(_spline.getGlobalGradient(t))*180.f/math::PI;
			while(abs(angle - angle_prev) > 180.f)
			{
				if(angle > angle_prev)
				{
					angle -= 360.f;
				}else{
					angle += 360.f;
				}
			}
			_angles.addKey(angle);
			_startAcceleration.addKey(0.f);
			_finishAcceleration.addKey(math::clamp(0.f, 1.f, t*2.f));
			angle_prev = angle;
		}
		path.CalculateGradient();
		_strip.SetSplinePath(path, 0.2f);
		_strip._currentStripLenght = GameSettings::SQUARE_SIDEF*3.f/_strip._fullLenght;
		_strip.setStripTime(0.f, 0.f, 1.f);
		_angles.CalculateGradient();
		_startAcceleration.CalculateGradient();
		_finishAcceleration.CalculateGradient();

		_hangIsSend = false;
		_finishEffectIsRun = false;
		return true;
	}

	void Spirit::SetAutoTrigger()
	{
		_autoTrigger = true;
		_pos = _posFrom;
		InitHangState();
		_state = SPIRIT_DISAPPEAR;
	}

}//namespace Game