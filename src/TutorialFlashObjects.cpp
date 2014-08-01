#include "stdafx.h"
#include "TutorialFlashObjects.h"
#include "MyApplication.h"
#include "Tutorial.h"
#include "GameField.h"

namespace TutorialFlashObjects
{

template<class T>
void LuaArrayToVector(luabind::object const& luaVector, std::vector<T> &vec)
{
	int type = luabind::type(luaVector);
	if( type == LUA_TTABLE )
	{
		// lua-массив элементов
		luabind::iterator itr(luaVector), end;
		for( ; itr != end; ++itr )
		{
			vec.push_back( luabind::object_cast<T>(*itr) );
		}
	}
	else if( type == LUA_TUSERDATA )
	{
		// просто одиночный элемент
		vec.push_back( luabind::object_cast<T>(luaVector) );
	}
}

TutorialFlashAreaHighlighter::TutorialFlashAreaHighlighter(luabind::object const& LuaRects, float opacity)
	: FlashDisplayObject()
	, _lightMap(NULL)
	, _opacity(opacity)
{
	_lightMap = new RenderTargetHolder(Render::device.Width(), Render::device.Height(), true);

	_shader = Core::resourceManager.Get<Render::ShaderProgram>("TutorialHighlight");
	_cellLight = Core::resourceManager.Get<Render::Texture>("CellHighlight");
	local_time = 1.0f;

	std::vector<IRect> rects;
	 LuaArrayToVector<IRect>(LuaRects, rects);

	_current = rects;
}

TutorialFlashAreaHighlighter::~TutorialFlashAreaHighlighter() {
	_lightMap->Purge();
}

void TutorialFlashAreaHighlighter::UpdateTarget()
{
	_lightMap->BeginRendering(Color::WHITE_TRANSPARENT);
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

	Render::device.SetBlendMode(Render::ALPHA);
	_lightMap->EndRendering();
}

void TutorialFlashAreaHighlighter::SetShape(luabind::object const& LuaRects)
{
	std::vector<IRect> rects;
	LuaArrayToVector<IRect>(LuaRects, rects);

	_prev = _current;
	_current = rects;

	local_time = 1.0f;
}

void TutorialFlashAreaHighlighter::DrawArea(const std::vector<IRect>& cells, float scale)
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

void TutorialFlashAreaHighlighter::Hide()
{
	local_time = 1.0f;
	_prev = _current;
	_current.clear();
}

void TutorialFlashAreaHighlighter::render(FlashRender& render)
{
	render.flush();

	UpdateTarget();
	if(_current.empty()) {
		Render::BeginAlphaMul(std::max(local_time, 0.0f));
	} else if(_prev.empty()) {
		Render::BeginAlphaMul(std::min(1.0f - local_time, 1.0f));
	}
	_shader->Bind();
	_shader->setPSParam("opacity", _opacity);
	_lightMap->Draw(IPoint(0,0));
	_shader->Unbind();
	if(_prev.empty() || _current.empty()) {
		Render::EndAlphaMul();
	}

	render.invalidateConstant(FlashTextureCh0);
}

void TutorialFlashAreaHighlighter::update(float dt)
{
	if( local_time > 0.0f ) {
		local_time -= dt * 2.0f;
		if( local_time < 0.0f )
			_prev.clear();
	}
}

bool TutorialFlashAreaHighlighter::hitTest(float x, float y, IHitTestDelegate* hitTestDelegate) {
	return true;
}

bool TutorialFlashAreaHighlighter::getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem) {
	return false;
}

void TutorialFlashAreaHighlighter::release() {
	_lightMap->Purge();
}

TutorialFlashArrow::TutorialFlashArrow(luabind::object const& LuaPoints)
	: FlashDisplayObject()
	, _pulseTimer(0.0f)
{
	_arrowTex = Core::resourceManager.Get<Render::Texture>("TutorialPointArrow");
	local_time = -1.0f;
	const FPoint offset(37.0f, 23.0f);

	std::vector<IPoint> points;
	LuaArrayToVector<IPoint>(LuaPoints, points);

	for(IPoint square : points)
	{
		FRect rect = FRect(_arrowTex->getBitmapRect());
		rect.MoveTo(square);
		_rects.push_back(rect);
	}
}

TutorialFlashArrow::~TutorialFlashArrow() {
}

void TutorialFlashArrow::render(FlashRender& render)
{
	render.flush();

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

	render.invalidateConstant(FlashTextureCh0);
}

bool TutorialFlashArrow::hitTest(float x, float y, IHitTestDelegate* hitTestDelegate) {
	return false;
}

bool TutorialFlashArrow::getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem) {
	return false;
}

void TutorialFlashArrow::update(float dt) {
	_pulseTimer += 4.0f * dt;
	if(local_time < 0.0f) {
		// появление
		local_time = std::min(local_time + 1.0f * dt, 0.0f);
	} else if(local_time > 0.0f) {
		// исчезновение
		local_time = std::min(local_time + 1.0f * dt, 1.0f);
	}
}

} // end of namespace