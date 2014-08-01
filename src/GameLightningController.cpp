#include "stdafx.h"
#include "GameLightningController.h"
#include "GameField.h"
#include "Match3.h"
#include "BoostKinds.h"
#include "GameChipRemover.h"

extern void clearAffectedCells(AddressVector& _affectedCells);

namespace Game
{
	GameLightningController::GameLightningController(const IPoint &start, int color, float startTime)
		: GameFieldController("LightningController", 1.f, GameField::Get())
		, _startSquaerAddress(start)
		, _color(color)
		, _colorize(GameSettings::chip_settings[color].color_wall)
		, _strip(Color(_colorize.red, _colorize.green, _colorize.blue, 200))
		, _backStrip(Color(200, 250, 250, 160))
	{
		local_time = -startTime;
		MyAssert(_color >= 0);
		finished = false;
		_localTime = 0.f;
		_procesTime = 0.f;
		DoLightning();
		_procesTime = 0.f;
		_stripTex = Core::resourceManager.Get<Render::Texture>("LightningStrip");
		_backStripTex = Core::resourceManager.Get<Render::Texture>("LightningStripBack");
		size_t count = _csquares.size();
		if(count > 1)
		{
			FPoint prev = _csquares[0]->GetCellPos() + GameSettings::CELL_HALF;
			float L = 0.f;
			for (size_t q = 0; q < count; q++)
			{
				FPoint next = _csquares[q]->GetCellPos() + GameSettings::CELL_HALF;
				L += (next - prev).Length();
				_strip.addPathKey(next.x, next.y, 30.f);
				_backStrip.addPathKey(next.x, next.y, 30.f);
			}
			_strip.CalculateBuffer(int(L/30.f));
			_backStrip.CalculateBuffer(int(L/30.f));

			_strip.setStripLength(1.0f);
			_backStrip.setStripLength(1.0f);
			_strip.setTextureScale(1.0f);
			_backStrip.setTextureScale(1.0f);

			_strip.setTextureSpeed(-time_scale*4);
			_backStrip.setTextureSpeed(-time_scale*4);
		}
	}

	GameLightningController::~GameLightningController()
	{
	}

	bool GameLightningController::ValidSquare(Game::Square *sq, const int &color)
	{
		if(!Game::isVisible(sq))
		{
			return false;
		}
		if( sq->IsHardStand())
		{
			return false;
		}
		return sq->IsColor(color) && !sq->GetChip().IsAdapt();
	}

	float GameLightningController::GetTimeScale() const
	{
		return time_scale;
	}

	std::vector<Game::Square*>* GameLightningController::GetSquares()
	{
		return &_csquares;
	}

	void GameLightningController::DoLightning()
	{
		std::map<int, int> random_idx;
		std::vector<Game::Square*> vec;
		_center_explision = math::lerp<FPoint>(Game::activeRect.RightTop(), Game::activeRect.LeftBottom(), 0.5f)*GameSettings::SQUARE_SIDE;
		for (int x = Game::activeRect.LeftBottom().x; x <= Game::activeRect.RightTop().x; x++)
		{
			for (int y = Game::activeRect.LeftBottom().y; y <= Game::activeRect.RightTop().y; y++)
			{
				Game::Square *sq = GameSettings::gamefield[x+1][y+1];
				if(ValidSquare(sq, _color))
				{
					vec.push_back(sq);
					int r_idx = math::random(1, 10000);
					if(sq->address.ToPoint() == _startSquaerAddress)
					{
						r_idx = 0;
					}
					while(random_idx.find(r_idx) != random_idx.end())
					{
						r_idx = math::random(1, 10000);
					}
					random_idx[r_idx] = vec.size() -1;
				}
			}		
		}
		//Перемешиваем
		_csquares.clear();
		if(random_idx.empty())
		{
			return;
		}
		_csquares.resize(vec.size());
		int ptr = 0;
		for(std::map<int, int>::iterator i = random_idx.begin(); i != random_idx.end(); i++)
		{
			_csquares[ptr] = vec[i->second];
			ptr++;
		}
	}

	void GameLightningController::Update(float dt)
	{
		if( local_time < 0.0f)
		{
			local_time += dt;
		}
		else
		{
			_localTime += dt;
			_procesTime += dt*time_scale;
			_strip.setStripTime(_procesTime);
			_backStrip.setStripTime(_procesTime);
			if(_procesTime < 1)
			{
				size_t count = _csquares.size();
				for(size_t i = 0; i < count; i++)
				{
					Game::Square *sq = _csquares[i];
					FPoint pos_cell = sq -> GetCellPos();
					sq -> GetChip().SetOffset( 
						(2.f * (sinf(_localTime * 70.0f + pos_cell.x * 30.0f) + sinf(_localTime * 80.0f + pos_cell.y * 35.6f)))*_procesTime,
						(2.f * (sinf(2.0f + _localTime * 60.0f + pos_cell.y * 153.0f) + sinf(_localTime * 90.0f + pos_cell.x * 131.0f + 1.0f)*_procesTime)));
				}
			} else {
				if (_csquares.size() > 0) {
					AddressVector vec;
					for (auto sq: _csquares) {
						vec.push_back(sq->address);
					}
					clearAffectedCells(vec);
					_csquares.clear();
				}
			}
		}
	}

	void GameLightningController::Draw()
	{
		size_t count = _csquares.size();
		if(count <= 1)
		{
			return;
		}

		_backStripTex->Bind();
		_backStrip.Draw();
		Render::device.SetBlendMode(Render::ADD);		
		_backStrip.Draw();
		Render::device.SetBlendMode(Render::ALPHA);

		_stripTex->Bind();
		_strip.Draw();
		Render::device.SetBlendMode(Render::ADD);		
		_strip.Draw();
		Render::device.SetBlendMode(Render::ALPHA);
	}

	bool GameLightningController::isFinish()
	{
		return _procesTime >= 1;
	}

	// GameSplashLightningController
	void GameSplashLightningController::Swap(IPoint &a, IPoint &b)
	{
		IPoint t = a;
		a = b;
		b = t;
	}

	bool GameSplashLightningController::CmpPolar(PolarPoint a, PolarPoint b)
	{
		return (a.alpha < b.alpha);
	}

	bool GameSplashLightningController::CmpPolarRevers(PolarPoint a, PolarPoint b)
	{
		return (a.alpha > b.alpha);
	}

	GameSplashLightningController::GameSplashLightningController(float time, AddressVector &points, float delay)
		: GameFieldController("SplashLightningController", 1.f, GameField::Get())
		, STATIC_TIME(0.04f)
		, _delay(delay)
	{
		_splash = Core::resourceManager.Get<Render::Texture>("LightningFlash");
		_splashDot = Core::resourceManager.Get<Render::Texture>("LightningFlashDot");

		_length = 0;
		
		Points screenPoints;
		for(auto p : points) {
			IPoint ps = GameSettings::ToScreenPos(p.ToPoint() * GameSettings::SQUARE_SIDE);
			screenPoints.push_back(IPoint(ps.x + GameSettings::SQUARE_SIDE/2, ps.y + GameSettings::SQUARE_SIDE/2));
		}

		FPoint center = FPoint(0, 0);
		for (unsigned int i = 0; i < screenPoints.size(); i++) {
			center = center + screenPoints[i];
		}
		center = center * (1.f/screenPoints.size());
		FPoint effectStartPos(screenPoints[0]);
		FPoint v = (FPoint(screenPoints[0]) - center);
		float alpha;
		if (v.GetDistanceToOrigin() < 1) {
			alpha = 0;
		} else {
			alpha = (FPoint(screenPoints[0]) - center).GetAngle();
		}

		std::vector<PolarPoint> polar;
		for (unsigned int i = 0; i < screenPoints.size(); i++) {
			PolarPoint p;
			p.distance = (FPoint(screenPoints[i]) - center).GetDistanceToOrigin();
			if (p.distance > 0) {
				if (v.GetDistanceToOrigin() < 1) {
					p.alpha = FPoint(1, 0).GetDirectedAngleNormalize(FPoint(screenPoints[i]) - center);
				} else {
					p.alpha = (FPoint(screenPoints[0]) - center).GetDirectedAngleNormalize(FPoint(screenPoints[i]) - center);
				}
			} else {
				p.alpha = 0.1f;
			}
			p.index = i;
			p.fa = points[i];
			polar.push_back(p);
		}

		std::sort(polar.begin(), polar.end(), CmpPolar);

		Points p;
		for (unsigned int i = 0; i < screenPoints.size(); i++) {
			_cells.push_back(polar[i].fa);
			shiftingCells.push_back(false);
			_way.push_back(center + FPoint(polar[i].distance*cos(polar[i].alpha + alpha)
				, polar[i].distance*sin(polar[i].alpha + alpha)));
			if (i > 0) {
				_length += FPoint(_way[i] - _way[i - 1]).GetDistanceToOrigin();
			}
			p.push_back(screenPoints[polar[i].index]);
		}
		screenPoints = p;

		_time = time;
		_timeCounter = _time;

		float timeForOnePoint = (time/2) / screenPoints.size();
		float pointLength = 0.0f;

		for (unsigned int i = 0; i < screenPoints.size(); i++) {
			if (i > 0) {
				pointLength += FPoint(_way[i] - _way[i - 1]).GetDistanceToOrigin();
				_startTimes.push_back(FPoint(0.8f * timeForOnePoint + (i-1)*timeForOnePoint, pointLength));
				_startTimes.push_back(FPoint(i*timeForOnePoint, pointLength));
			} else {
				_startTimes.push_back(FPoint(0.0f, 0.0f));
			}
		}
		_startTimes.push_back(FPoint(time, pointLength));
		_offSet = math::random(0.f, 0.998f);
		_offSetTimeCounter = STATIC_TIME;
		_start = 0;
		_soundId = MM::manager.PlaySample("LightingRun", true);
	}

	GameSplashLightningController::~GameSplashLightningController()
	{
		if (MM::manager.IsValid(_soundId))
			MM::manager.StopSample(_soundId);
	}

	void GameSplashLightningController::DrawAbsolute()
	{
		if (isFinish()) return;
		for (unsigned int i = 0; i < shiftingCells.size(); i++) {
			if (shiftingCells[i]) {
				// дрожжим фишки
				Game::Square *sq = GameSettings::gamefield[_cells[i]];
				FPoint pos_cell = sq -> GetCellPos();
				sq -> GetChip().SetOffset(
					(2.f * (sinf((_time - _timeCounter) * 70.0f + pos_cell.x * 30.0f) + sinf((_time - _timeCounter) * 80.0f + pos_cell.y * 35.6f)))*((_time - _timeCounter)/2),
					(2.f * (sinf(2.0f + (_time - _timeCounter) * 60.0f + pos_cell.y * 153.0f) + sinf((_time - _timeCounter) * 90.0f + pos_cell.x * 131.0f + 1.0f)*((_time - _timeCounter)/2))));
				// нарисуем фишку с открытыми глазами
				if (sq->GetChip().IsSimpleChip()) {
					FPoint chip_pos = sq->GetChip().GetPos() + GameSettings::ToScreenPos(_cells[i].ToPoint() * GameSettings::SQUARE_SIDE) - IPoint(0, 10);
					FRect uv = Game::GetChipRect(sq->GetChip().GetColor(), true, false, false);
					IRect rect = Game::ChipColor::DRAW_RECT.MovedTo(chip_pos.Rounded());
					ChipColor::chipsTex->Draw(rect, uv);
				}
			}
		}
		FRect empty;
		float finish = 0.0f;
		float start = _start;
		float l = 0;// сюда буду копить длину
		for (unsigned int i = 0; i < _way.size() - 1; i++)
		{
			float t = FPoint(_way[i + 1] - _way[i]).GetDistanceToOrigin(); // длина текущего отрезка
			if (!(start < l || finish >= (l + t) || t < 1))
			{
				Render::device.PushMatrix();
				Render::device.MatrixTranslate(math::Vector3(_way[i].x, _way[i].y, 0.f));
				Render::device.MatrixRotate(math::Vector3(0, 0, 1), FPoint(_way[i + 1] - _way[i]).GetAngle()/math::PI*180);
				Render::device.MatrixTranslate(math::Vector3(0, -_splash->getBitmapRect().height/2.f, 0));
				Render::device.SetBlendMode(Render::ADD);
				float size = t/_splash->getBitmapRect().width;
				if (finish >= l && finish < (l + t) && start >= (l + t)) // если конец молнии попадает в этот отрезок, а начало нет
				{
					float f = (finish - l)/t;
					int width = _splash->getBitmapRect().width;
					int height = _splash->getBitmapRect().height;
					_splash->Bind();
					float xStart = width * size * f;
					float xEnd =  width * size;
					float yStart = 0;
					float yEnd = (float)height;
					FRect trect(math::clamp(0.f, 1.f, _offSet -  size * (1 - f)), math::clamp(0.f, 1.f, _offSet),	0,	1);
					FRect f_rect(trect);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = xEnd;
					xEnd += 15.0f;
					f_rect = trect;
					f_rect.xStart = f_rect.xEnd;
					f_rect.xEnd = math::clamp(0.f, 1.f, f_rect.xEnd + 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = width * size * f-15.0f;
					xEnd = width * size * f;
					f_rect = trect;
					f_rect.xEnd = f_rect.xStart;
					f_rect.xStart = math::clamp(0.f, 1.f, f_rect.xEnd - 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);

					_splashDot->Draw(IPoint(int(_splash->getBitmapRect().width*size*f - _splashDot->getBitmapRect().width/2), 0));
					_splashDot->Draw(IPoint(int(t) - _splashDot->getBitmapRect().width/2, 0));

					shiftingCells[i] = true;
				}
				else if (finish < l && start >= l && start < (l + t)) // если конец молнии не попадает в этот отрезок, а начало - попадает
				{
					float s = (start - l)/t;
					int width = _splash->getBitmapRect().width;
					int height = _splash->getBitmapRect().height;
					_splash->Bind();
					float xStart = 0;
					float xEnd =  width * size * s;
					float yStart = 0;
					float yEnd = (float)height;
					FRect trect(math::clamp(0.f, 1.f, _offSet), math::clamp(0.f, 1.f, size*s + _offSet), 0, 1);
					FRect f_rect(trect);
					_splash->TranslateUV(empty, f_rect);

					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),		
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = xEnd;
					xEnd += 15.0f;
					f_rect = trect;
					f_rect.xStart = f_rect.xEnd;
					f_rect.xEnd = math::clamp(0.f, 1.f, f_rect.xEnd + 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = -15.0f;
					xEnd = 0.0f;
					f_rect = trect;
					f_rect.xEnd = f_rect.xStart;
					f_rect.xStart = math::clamp(0.f, 1.f, f_rect.xEnd - 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);

					_splashDot->Draw(IPoint(int(_splash->getBitmapRect().width*size*s - _splashDot->getBitmapRect().width/2), 0));
					_splashDot->Draw(IPoint(-_splashDot->getBitmapRect().width/2, 0));

					shiftingCells[i] = true;
				}
				else if (finish >= l && start < (l + t)) // если конец молнии и начало попадают в этот отрезок
				{
					float f = (finish - l)/t;
					float s = (start - l)/t;
					int width = _splash->getBitmapRect().width;
					int height = _splash->getBitmapRect().height;
					_splash->Bind();

					float xStart = width * size * f;
					float xEnd =  width * size * s;
					float yStart = 0;
					float yEnd = (float)height;
					FRect trect(math::clamp(0.f, 1.f, _offSet), math::clamp(0.f, 1.f, size * (s - f) + _offSet), 0, 1);
					FRect f_rect(trect);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),				
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = xEnd;
					xEnd += 15.0f;
					f_rect = trect;
					f_rect.xStart = f_rect.xEnd;
					f_rect.xEnd = math::clamp(0.f, 1.f, f_rect.xEnd + 15.0f / _splash->getBitmapRect().width);

					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = width * size * f-15.0f;
					xEnd = 0.0f;
					f_rect = trect;
					f_rect.xEnd = f_rect.xStart;
					f_rect.xStart = math::clamp(0.f, 1.f, f_rect.xEnd - 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);

					_splashDot->Draw(IPoint(int(_splash->getBitmapRect().width*size*f - _splashDot->getBitmapRect().width/2), 0));
					_splashDot->Draw(IPoint(int(_splash->getBitmapRect().width*size*s - _splashDot->getBitmapRect().width/2), 0));

				}
				else if (finish < l && start >= (l + t)) // если конец молнии и начало не попадают в этот отрезок, но по разные стороны от него
				{
					int width = _splash->getBitmapRect().width;
					int height = _splash->getBitmapRect().height;

					_splash->Bind();

					float xStart = 0;
					float xEnd = width * size;
					float yStart = 0;
					float yEnd = (float)height;
					FRect trect(math::clamp(0.f, 1.f, _offSet), 
						math::clamp(0.f, 1.f, size + _offSet),
						0,	
						1		 );
					FRect f_rect(trect);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = xEnd;
					xEnd += 15.0f;
					f_rect = trect;
					f_rect.xStart = f_rect.xEnd;
					f_rect.xEnd = math::clamp(0.f, 1.f, f_rect.xEnd + 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);
					xStart = -15.0f;
					xEnd = 0.0f;
					f_rect = trect;
					f_rect.xEnd = f_rect.xStart;
					f_rect.xStart = math::clamp(0.f, 1.f, f_rect.xEnd - 15.0f / _splash->getBitmapRect().width);
					_splash->TranslateUV(empty, f_rect);
					Render::DrawQuad(math::Vector3(xStart, yStart, 0),
						math::Vector3(xEnd, yStart, 0),
						math::Vector3(xStart, yEnd, 0),
						math::Vector3(xEnd, yEnd, 0),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						Color(255, 255, 255, 0),
						Color(255, 255, 255, 255),
						f_rect.xStart, f_rect.xEnd, f_rect.yStart, f_rect.yEnd
						);

					_splashDot->Draw(IPoint(int(t - _splashDot->getBitmapRect().width/2), 0));
					_splashDot->Draw(IPoint(-_splashDot->getBitmapRect().width/2, 0));
				}
				Render::device.SetBlendMode(Render::ALPHA);
				Render::device.PopMatrix();
			}
			l += t;
			if (start >= _length && !shiftingCells.empty()) shiftingCells[shiftingCells.size()-1] = true;
		}
	}


void GameSplashLightningController::Update(float dt)
{
	if (isFinish()) return;
	if(_delay > 0) {
		_delay -=dt;
		return;
	}
	float timeOffset = (_time - _timeCounter);
	for (std::vector<FPoint>::iterator i = _startTimes.begin() ; i != _startTimes.end() ; ++i) {
		if ((i+1) == _startTimes.end()) {
			_start = i->y;
		} else if ((i->x <= timeOffset) && ((i+1)->x >= timeOffset)) {
			_start = math::lerp(i->y, (i+1)->y, math::EaseOutExpo((timeOffset - i->x) / ((i+1)->x - i->x)));
			break;
		}
	}
	_timeCounter -= dt;
	if (_timeCounter <= 0 && !_cells.empty()) {
		clearAffectedCells(_cells);
		_cells.clear();
		shiftingCells.clear();
	}
	_offSetTimeCounter -= dt;
	if (_offSetTimeCounter <= 0 || _timeCounter <=0) {
		_offSet = math::random(0.f, 0.998f);
		_offSetTimeCounter = STATIC_TIME;
	}
}

bool GameSplashLightningController::isFinish()
{
	return (_timeCounter <= 0);
}

} //namespace Game