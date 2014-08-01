#pragma once

#include "Flash.h"
#include "RenderTargetHolder.h"

class BlurScreen: public FlashDisplayObject
{
public:
	BlurScreen();
	~BlurScreen();
	virtual void render(FlashRender& render);
	virtual bool hitTest(float x, float y, IHitTestDelegate* hitTestDelegate);
	virtual bool getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem);
	virtual bool hasUpdate() { return true; }
	virtual void update(float dt);
	void shoot();
	void addEffect(const std::string& effectName, const FPoint& effectPosition);
	void release();
private:
	RenderTargetHolder* target;
	EffectsContainer effects;
};

class EmptyBlurScreen: public FlashDisplayObject
{
public :
	EmptyBlurScreen();
	~EmptyBlurScreen();
	virtual void render(FlashRender& render);
	virtual bool hitTest(float x, float y, IHitTestDelegate* hitTestDelegate);
	virtual bool getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem);
	void release();
};