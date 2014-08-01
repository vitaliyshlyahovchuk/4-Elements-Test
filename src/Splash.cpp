#include "stdafx.h"
#include "Splash.h"
#include "MyApplication.h"

Splash::Splash(const std::string& name, rapidxml::xml_node<>* xmlElement)
	: Widget(name, xmlElement)
	, timer(1.0f)
	, loadStarted(false)
	, texture(NULL)
{
}

void Splash::Update(float dt) {
	if (dt > 0.05f) {
		dt = 0.05f;
	}
	if (loadStarted) {
		if (timer > 0.f) {
			timer -= dt;
			_color.alpha = (int)(255 * timer);
			if (timer < 0.f) {
				texture->EndUse();
				texture = NULL;
			}
		}
	}
}

void Splash::Draw() {
	if (!loadStarted) {
		// впервые отрисовались
		loadStarted = true;
		texture = Core::resourceManager.Get<Render::Texture>("Splash");
		texture->BeginUse(ResourceLoadMode::Sync);
		IRect br = texture->getBitmapRect();
		position.x = (MyApplication::GAME_WIDTH - br.width) / 2;
		position.y = MyApplication::GAME_HEIGHT - br.height;
	}
	if (texture) {
		texture->Draw(position);
	}
}
