#include "stdafx.h"
#include "BlurScreen.h"
#include "MyApplication.h"

#include "Core/TextureLoader.h"

BlurScreen::BlurScreen()
	: FlashDisplayObject()
	,target(NULL)
{
	if (Render::device.ContentScaleFactor() == 1) {
		target = new RenderTargetHolder(64, 128, true);
	} else {
		// в ретине таргеты в два раза больше, но шейдер блюра делает муар на удвоенном таргете
		target = new RenderTargetHolder(32, 64, true);
	}
}

BlurScreen::~BlurScreen() {
	delete target;
}

void BlurScreen::render(FlashRender& render) {
	render.flush();
	FPoint position(0.f,0.f);
	localToGlobal(position.x, position.y);
	float alpha = getAlpha();
	IFlashDisplayObject* p = getParent();
	while (p) {
		alpha *= p->getAlpha();
		p = p->getParent();
	};
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(position.x, position.y, 0.f));
	Render::device.MatrixScale(1.f, -1.f, 1.f);
    
	Render::BeginAlphaMul(alpha);
	
	target->Bind();
	FRect rect(-MyApplication::GAME_WIDTH * 0.5f, MyApplication::GAME_WIDTH * 0.5f, -MyApplication::GAME_HEIGHT * 0.5f, MyApplication::GAME_HEIGHT * 0.5f);
	FRect uv(0.0f, 1.0f, 0.0f, 1.0f);
    Render::BeginColor(Color(230, 241, 254));
	Render::DrawRect(rect.Rounded(), uv);
    Render::EndColor();
	effects.Draw();
	Render::EndAlphaMul();
    
	Render::device.PopMatrix();
	render.invalidateConstant(FlashTextureCh0);
}

bool BlurScreen::hitTest(float x, float y, IHitTestDelegate* hitTestDelegate) {
	return true;
}

bool BlurScreen::getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem) {
	float x,y;
	getPosition(x, y);
	float w2 = MyApplication::GAME_WIDTH * 0.5f;
	float h2 = MyApplication::GAME_HEIGHT * 0.5f;
	if ( targetCoordinateSystem == NULL ){
		left = x - w2;
		top = y - h2;
		right = x + w2;
		bottom = y + h2;
	}else{
		float tx, ty;
		tx = x;
		ty = y;
		localToTarget(tx, ty, targetCoordinateSystem);
		left = right = tx;
		top = bottom = ty;

		tx = x + MyApplication::GAME_WIDTH;
		ty = y;
		localToTarget(tx, ty, targetCoordinateSystem);
		if (tx < left) {       left = tx; }
		else if (tx > right) { right = tx; }
		if (ty < top) { top = ty; }
		else if (ty > bottom) { bottom = ty; }
		
		tx = x;
		ty = y + MyApplication::GAME_HEIGHT;
		localToTarget(tx, ty, targetCoordinateSystem);
		if (tx < left) {       left = tx; }
		else if (tx > right) { right = tx; }
		if (ty < top) { top = ty; }
		else if (ty > bottom) { bottom = ty; }

		tx = x + MyApplication::GAME_WIDTH;
		ty = y + MyApplication::GAME_HEIGHT;
		localToTarget(tx, ty, targetCoordinateSystem);
		if (tx < left) {       left = tx; }
		else if (tx > right) { right = tx; }
		if (ty < top) { top = ty; }
		else if (ty > bottom) { bottom = ty; }
	}
	return true;
}

Render::Texture* GetScreenshot()
{
	Render::device.BeginScene();
	Render::device.ResetViewport();
	Render::device.Begin2DMode();
	Core::mainScreen.Draw();
	Render::device.End2DMode();

	const int width = Render::device.PhysicalWidth();
	const int height = Render::device.PhysicalHeight();
	
	Render::Image image;
	image.width = width;
	image.height = height;
	image.components = 4;
	image.pixelType = PIXEL_TYPE_8888;
	image.data.reset(new unsigned char[width * height * 4]);

	Render::device.GLFinish();
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data.get());
	
	Render::Texture *tex = new Render::Texture(image);
	tex->SetLoader(new TextureLoader());
	tex->BeginUse(ResourceLoadMode::Sync);
	
	return tex;
}

void BlurScreen::shoot()
{
	Render::Texture *screen = GetScreenshot();

	if (screen)
	{
		IRect rect = target->GetBitmapRect();
		FRect uv = FRect(0.f, 1.f, 0.f, 1.f);
		// сначала отрисуем в таргет текстуру
		target->BeginRendering(Color::BLACK);
		screen->Bind();
		Render::DrawRect(rect, uv);
		target->EndRendering();
		screen->EndUse(ResourceLoadMode::Sync);
		delete screen;
        
		// потом будем рисовать таргет в таргеты
		Render::ShaderProgram *shader = Core::resourceManager.Get<Render::ShaderProgram>("BlurShader");
        Render::ShaderProgram *shader2 = Core::resourceManager.Get<Render::ShaderProgram>("BlurShaderPass2");
		float size[4] = {
			1.f / static_cast<float>(rect.width) / Render::device.ContentScaleFactor(),
			1.f / static_cast<float>(rect.height) / Render::device.ContentScaleFactor(),
			0.f, 0.f
		};
        
		RenderTargetHolder help(rect.width, rect.height, false);
		RenderTargetHolder* from = target;
		RenderTargetHolder* to = &help;
		static int n = 4;
		for (int i = 0; i < n; ++i) {
                
			to->BeginRendering();
			shader->Bind();
			shader->setPSParam("textureSizeMul", size, 4);
			from->Bind();
			Render::DrawRect(rect, uv);
			shader->Unbind();
			to->EndRendering();
			
			std::swap(from, to);
			std::swap(shader, shader2);
		}

		if (from != target) {
			// если пинг-понг вызвался нечетное количество раз, нужно отрендерить help в target
			target->BeginRendering();
			help.Bind();
			Render::DrawRect(rect, uv);
			target->EndRendering();
		}
	}
}

void BlurScreen::addEffect(const std::string& effectName, const FPoint& effectPosition) {
	ParticleEffect* effect = effects.AddEffect(effectName);
	effect->Reset();
	effect->SetPos(effectPosition.x, -effectPosition.y); // во флеше "y" повернута вниз
}

void BlurScreen::update(float dt) {
	effects.Update(dt);
}

void BlurScreen::release() {
	target->Purge();
	effects.KillAllEffects();
}

EmptyBlurScreen::EmptyBlurScreen()
	: FlashDisplayObject()
{
}

EmptyBlurScreen::~EmptyBlurScreen() {
}

void EmptyBlurScreen::render(FlashRender& render) {
}

bool EmptyBlurScreen::hitTest(float x, float y, IHitTestDelegate* hitTestDelegate) {
	return true;
}

bool EmptyBlurScreen::getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem) {
	return true;
}

void EmptyBlurScreen::release() {
}
