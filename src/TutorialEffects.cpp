#include "stdafx.h"
#include "TutorialEffects.h"
#include "GameField.h"

namespace Tutorial
{


AreaHighlighter::AreaHighlighter(const std::vector<IRect>& cells, float opacity)
	: GameFieldController("TutorialHighlight", 2.0f, GameField::Get())
	, _lightMap(Render::device.Width(), Render::device.Height(), true)
	, _opacity(opacity)
{
	z = 1;
	_shader = Core::resourceManager.Get<Render::ShaderProgram>("TutorialHighlight");
	_cellLight = Core::resourceManager.Get<Render::Texture>("CellHighlight");

	local_time = 1.0f;
	_current = cells;
}

void AreaHighlighter::UpdateTarget()
{
	_lightMap.BeginRendering(Color::WHITE_TRANSPARENT);
	Render::device.SetBlendMode(Render::ADD);

	_cellLight->Bind();
	float alpha = std::max(local_time, 0.0f);
	float t = math::clamp(0.0f, 1.0f, 1.0f - local_time);

	Render::BeginAlphaMul(alpha);
	DrawArea(_prev, 1.0f + 15.0f * t * t * t);
	Render::EndAlphaMul();

	t = 1.0f - t;
	Render::BeginAlphaMul(1.0f - alpha);
	DrawArea(_current, 1.0f + 15.0f * t * t * t);
	Render::EndAlphaMul();

	// высвечиваем также область над подсветкой зоны поражения
	FPoint offset = GameSettings::ToFieldPos(FPoint(0,0));
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(-offset);
	GameField::Get()->DrawSequenceAffectedZone();
	Render::device.PopMatrix();

	Render::device.SetBlendMode(Render::ALPHA);
	_lightMap.EndRendering();
}

void AreaHighlighter::SetShape(const std::vector<IRect>& cells)
{
	_prev = _current;
	_current = cells;

	local_time = 1.0f;
}

void AreaHighlighter::DrawArea(const std::vector<IRect>& cells, float scale)
{
	for(size_t i = 0; i < cells.size(); ++i)
	{
		FRect rect(cells[i]);
		FPoint center = rect.CenterPoint();
		rect.MoveBy(-center);
		rect.Scale(scale);
		rect.MoveBy(center);

		Render::DrawRect( rect.Rounded() );
	}
}

void AreaHighlighter::Hide()
{
	local_time = 1.0f;
	_prev = _current;
	_current.clear();
}

void AreaHighlighter::DrawAbsolute()
{
	UpdateTarget();

	if(_current.empty()) {
		Render::BeginAlphaMul(std::max(local_time, 0.0f));
	} else if(_prev.empty()) {
		Render::BeginAlphaMul(std::min(1.0f - local_time, 1.0f));
	}

	_shader->Bind();
	_shader->setPSParam("opacity", _opacity);
	_lightMap.Draw(IPoint(0,0));
	_shader->Unbind();

	if(_prev.empty() || _current.empty())
	{
		Render::EndAlphaMul();
	}
}

void AreaHighlighter::Update(float dt)
{
	if( local_time > 0.0f ) {
		local_time -= time_scale * dt;

		if( local_time < 0.0f )
			_prev.clear();
	}
}

bool AreaHighlighter::isFinish()
{
	return (local_time <= 0.0f) && _current.empty();
}

ShowDirectionController::ShowDirectionController(FPoint from, FPoint to)
	: GameFieldController("TutorialShowDir", 1.0f, GameField::Get())
	, _alpha(0.0f)
	, _start(from)
	, _waveTimer(0.0f)
{
	z = 2;
	tex = Core::resourceManager.Get<Render::Texture>("TutorialDirArrow");
	tex->setAddressType(Render::Texture::REPEAT);

	FPoint dir = (to - from);

	float tw = tex->Width() * 0.5f;
	float th = tex->Height() * 0.4f;

	_count = math::floor( dir.Length() / tw );
	_dpos = dir / _count;

	dir.Normalize();
	FPoint n(-dir.y, dir.x);
	vt = dir * tw;
	vn = n * (th * 0.5f);

	local_time = -1.0f;
}

void ShowDirectionController::Start()
{
	local_time = 0.0f;
}

void ShowDirectionController::End()
{
	local_time = 1.0f;
}

float ShowDirectionController::WaveFunc(float t) const
{
	t -= floorf(t); // t = 0..1
	t = (t - 0.5f) * 6.0f; // t = -2..2
	return math::cos( math::clamp(-1.0f, 1.0f, t) * math::PI * 0.5f );
}

void ShowDirectionController::DrawAbsolute()
{
	tex->Bind();

	Render::BeginAlphaMul(_alpha);
	FPoint st = _start;
	for(int i = 0; i < _count; ++i)
	{
		float ph = (1.0f - (float)i/_count) * 0.5f;
		FPoint off = _dpos * WaveFunc(ph + _waveTimer) * 0.1f;
		Render::DrawQuad(st+vn+off, st+vt+vn+off, st-vn+off, st+vt-vn+off);
		st += _dpos;
	}
	Render::EndAlphaMul();
}

void ShowDirectionController::Update(float dt)
{
	_waveTimer += 0.5f * dt;
	if(_waveTimer >= 1.0f)
		_waveTimer = 0.0f;

	if( local_time == 0.0f ) {
		_alpha = std::min(1.0f, _alpha + 2.0f * dt);
	} else if( local_time == 1.0f ) {
		_alpha = std::max(0.0f, _alpha - 2.0f * dt);
	}
}

bool ShowDirectionController::isFinish()
{
	return (_alpha <= 0.0f) && (local_time >= 1.0f);
}




ShowMoveFinger::ShowMoveFinger(std::vector<FPoint> chain, float startTime)
	: GameFieldController("ShowMoveFinger", 1.0f, GameField::Get())
	, _chain(chain)
	, _chainT(chain.size())
{
	Assert(_chain.size() >= 2 );

	z = 2;

	FPoint prev(_chain[0]);
	float length = 0.0f;
	for(size_t i = 0; i < _chain.size(); ++i)
	{
		length += prev.GetDistanceTo(_chain[i]);
		_chainT[i] = length;
		prev = _chain[i];
	}

	for(size_t i = 0; i < _chainT.size(); ++i)
	{
		_chainT[i] /= length;
	}

	const float fadeInDuration = 0.24f;
	const float fadeOutDuration = 0.34f;
	float totalDuration = chain.size() * 0.29f + fadeInDuration + fadeOutDuration;
	time_scale = 1.0f / totalDuration;

	local_time = -startTime * time_scale;

	_tex = Core::resourceManager.Get<Render::Texture>("TutorialFinger");
}

FPoint ShowMoveFinger::getChainPos(float t) const
{
	Assert(t >= 0.0f && t <= 1.0f);

	for(size_t i = 0; i < _chainT.size()-1; ++i)
	{
		if(t >= _chainT[i] && t <= _chainT[i+1])
		{
			float t1 = (t - _chainT[i]) / (_chainT[i+1] - _chainT[i]);
			return math::lerp(_chain[i], _chain[i+1], t1);
		}
	}

	return _chain.back();
}

void ShowMoveFinger::Finish()
{
	local_time = 1.5f;
}

void ShowMoveFinger::DrawAbsolute()
{
	if( local_time >= 0.0f && local_time <= 1.0f )
	{
		const FPoint offset(-16.0f, -30.0f);

		const float fadeInDuration = 0.24f;
		const float fadeOutDuration = 0.34f;

		float fadeInTime = fadeInDuration * time_scale;
		float fadeOutTime = fadeOutDuration * time_scale;

		float alpha, t;
		if( local_time < fadeInTime ) {
			// появление
			alpha = local_time / fadeInTime;
			t = 0.0f;
		} else if( local_time > 1.0f - fadeOutTime ) {
			// исчезновение
			alpha = (1.0f - local_time) / fadeOutTime;
			t = 1.0f;
		} else {
			// движение
			alpha = 1.0f;
			t = (local_time - fadeInTime) / (1.0f - fadeInTime - fadeOutTime);
			t = math::clamp(0.0f, 1.0f, t);
		}

		Render::BeginAlphaMul(alpha);
		_tex->Draw( getChainPos(t) + offset );
		Render::EndAlphaMul();
	}
}

void ShowMoveFinger::Update(float dt)
{
	if( local_time < 0.0f && GameField::Get()->SelectingSequence())
		return;

	if( local_time <= 1.0f ) {
		local_time += time_scale * dt;
		if( local_time > 1.0f )
			local_time = -0.2f * time_scale;
	}
}

bool ShowMoveFinger::isFinish()
{
	return local_time > 1.0f;
}

ShowText::ShowText(FPoint pos, const std::string& textID)
	: GameFieldController("ShowText", 2.0f, GameField::Get())
	, _pos(pos)
	, _hiding(false)
{
	z = 2;
	local_time = 0.0f;
	_text = Core::resourceManager.Get<Render::Text>(textID);
}

void ShowText::Finish()
{
	_hiding = true;
}

void ShowText::DrawAbsolute()
{
	if( local_time >= 0.0f ) {
		Render::BeginAlphaMul(local_time);
		_text->Draw(_pos);
		Render::EndAlphaMul();
	}
}

void ShowText::Update(float dt)
{
	if(!_hiding) {
		local_time = std::min(1.0f, local_time + time_scale * dt);
	} else {
		local_time -= time_scale * dt;
	}
}

bool ShowText::isFinish()
{
	return _hiding && (local_time <= 0.0f);
}



PointArrowController::PointArrowController(const std::vector<IPoint> &squares, bool screenPoints)
	: GameFieldController("PointArrowController", 1.4f, GameField::Get())
	, _pulseTimer(0.0f)
{
	z = 2;
	_arrowTex = Core::resourceManager.Get<Render::Texture>("TutorialPointArrow");
	local_time = -1.0f;
	const FPoint offset(37.0f, 23.0f);

	for(IPoint square : squares)
	{
		FRect rect = FRect(_arrowTex->getBitmapRect());
		if (!screenPoints) {
			FPoint pt(square * GameSettings::SQUARE_SIDE);
			pt += FPoint(GameSettings::SQUARE_SIDEF * 0.5f, GameSettings::SQUARE_SIDEF);
			pt = GameSettings::ToScreenPos(pt - offset);
			rect.MoveTo(pt);
		} else {
			rect.MoveTo(square);
		}
		
		_rects.push_back(rect);
	}
}

void PointArrowController::Finish()
{
	local_time = 0.001f;
}

void PointArrowController::DrawAbsolute()
{
	float alpha = 1.0f - math::abs(local_time);
	Render::BeginAlphaMul(alpha);
	_arrowTex->Bind();
	float offset = 10.0f + 10.0f * math::sin(_pulseTimer);
	float dsize = 2.0f * math::cos(_pulseTimer - math::PI * 0.76f);
	for(FRect rect : _rects) {
		rect.xStart += dsize;
		rect.xEnd -= dsize;
		rect.yStart += (offset - dsize);
		rect.yEnd += (offset + dsize);
		Render::DrawRect(rect, FRect(0,1,0,1));
	}
	Render::EndAlphaMul();
}

void PointArrowController::Update(float dt)
{
	_pulseTimer += 4.0f * dt;

	if(local_time < 0.0f) {
		// появление
		local_time = std::min(local_time + time_scale * dt, 0.0f);
	} else if(local_time > 0.0f) {
		// исчезновение
		local_time = std::min(local_time + time_scale * dt, 1.0f);
	}
}

bool PointArrowController::isFinish()
{
	return (local_time >= 1.0f);
}

} // namespace Tutorial
