#pragma once

#include "Flash.h"
#include "RenderTargetHolder.h"

namespace TutorialFlashObjects
{

class TutorialFlashAreaHighlighter: public FlashDisplayObject
{
public:
	TutorialFlashAreaHighlighter(luabind::object const& LuaRects, float opacity);
	~TutorialFlashAreaHighlighter();

	virtual void render(FlashRender& render);
	virtual bool hitTest(float x, float y, IHitTestDelegate* hitTestDelegate);
	virtual bool getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem);
	virtual bool hasUpdate() { return true; }
	virtual void update(float dt);
	void release();

	void SetShape(luabind::object const& LuaRects);
	void Hide();

private:
	RenderTargetHolder* _lightMap;
	Render::ShaderProgram *_shader;
	Render::Texture *_cellLight;

	std::vector<IRect> _prev;
	std::vector<IRect> _current;
	float _opacity;
	float local_time;

	void UpdateTarget();
	void DrawArea(const std::vector<IRect>& cells, float scale);
};


class TutorialFlashArrow : public FlashDisplayObject
{
private:
	std::vector<FRect> _rects;
	Render::Texture *_arrowTex;
	float _pulseTimer;
	float local_time;

public:
	TutorialFlashArrow(luabind::object const& LuaPoints);
	~TutorialFlashArrow();

	virtual void render(FlashRender& render);
	virtual bool hitTest(float x, float y, IHitTestDelegate* hitTestDelegate);
	virtual bool getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem);
	virtual bool hasUpdate() { return true; }
	virtual void update(float dt);
};


} // end of namespace