#include "stdafx.h"
#include "RenderTargetHolder.h"

RenderTargetHolder::RenderTargetHolder(int width, int height, bool alpha, bool depth, bool stencil)
	: _width(width)
	, _height(height)
	, _renderTarget(nullptr)
	, _alpha(alpha)
	, _depth(depth)
	, _stencil(stencil)
	, _filter(Render::Texture::BILINEAR)
	, _adress(Render::Texture::CLAMP)
{
}

void RenderTargetHolder::SetParams(int width, int height, bool alpha, bool depth, bool stencil)
{
	_width = width;
	_height = height;
	_alpha = alpha;
	_depth = depth;
	_stencil = stencil;
	
	Purge();
	CheckTarget();
}

RenderTargetHolder::~RenderTargetHolder()
{
	if (_renderTarget != nullptr) {
		Render::device.DeleteRenderTarget(_renderTarget);
	}
}

void RenderTargetHolder::Purge()
{
	if (_renderTarget != nullptr) {
		Render::device.DeleteRenderTarget(_renderTarget);
		_renderTarget = nullptr;
	}
}

void RenderTargetHolder::CheckTarget()
{
	if (_renderTarget == nullptr) 
		_renderTarget = Render::device.CreateRenderTarget(_width, _height, _alpha, true, _depth, Render::MULTISAMPLE_NONE, _stencil);
	else 
		Render::Target::EnsureValidity(&_renderTarget);
}

void RenderTargetHolder::BeginRendering(Color bgcolor)
{
	CheckTarget();
	Render::device.BeginRenderTo(_renderTarget, bgcolor);
}

void RenderTargetHolder::BeginRendering()
{
	CheckTarget();
	Render::device.BeginRenderTo(_renderTarget);
}

void RenderTargetHolder::EndRendering()
{
	Render::device.EndRenderTo();
}

bool RenderTargetHolder::IsValid() const
{
	return _renderTarget && !_renderTarget->Empty();
}

bool RenderTargetHolder::NeedRedraw() const
{
	return _renderTarget && _renderTarget->IsValid() && _renderTarget->Empty();
}

void RenderTargetHolder::Draw(IPoint pos)
{
	CheckTarget();
	_renderTarget->Draw(pos);
}

void RenderTargetHolder::Bind(int channel, int stageOp)
{
	CheckTarget();
	_renderTarget->setFilterType(_filter);
	_renderTarget->setAddressType(_adress);
	_renderTarget->Bind(channel, stageOp);
}

void RenderTargetHolder::setFilteringType(Render::Texture::FilteringType filter)
{
	_filter = filter;
}

void RenderTargetHolder::setAddressType(Render::Texture::AddressType adress)
{
	_adress = adress;
}

IRect RenderTargetHolder::GetBitmapRect()
{
	CheckTarget();
	return _renderTarget->getBitmapRect();
}

bool RenderTargetHolder::needTranslate() const
{
	return _renderTarget->needTranslate();
}

void RenderTargetHolder::TranslateUV(FPoint& uv) const
{
	_renderTarget->TranslateUV(uv);
}

void RenderTargetHolder::TranslateUV(FRect& uv) const
{
	FPoint uv1(uv.xStart, uv.yStart), uv2(uv.xEnd, uv.yEnd);
	_renderTarget->TranslateUV(uv1);
	_renderTarget->TranslateUV(uv2);
	uv.xStart = uv1.x;
	uv.xEnd = uv2.x;
	uv.yStart = uv1.y;
	uv.yEnd = uv2.y;
}

void RenderTargetHolder::TranslateUV(FRect &rect, FRect &uv) const
{
	_renderTarget->TranslateUV(rect, uv);
}