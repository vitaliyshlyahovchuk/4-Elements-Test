#include "stdafx.h"
#include "StripEffect.h"
#include "SomeOperators.h"
//#include "Game.h"


StripEffect::StripEffect(const Color &color)
	: _stripTime(0.0f)
	, _numSectors(0)
	, _textureScale(6.0f)
	, _textureOffset(-16.0f)
	, _stripLength(0.25f)
	, _color(color)
{
}

void StripEffect::addPathKey(float xValue, float yValue, float heightValue)
{
	_x.addKey(xValue);
	_y.addKey(yValue);
	_height.addKey(heightValue);
}

void StripEffect::CalculateBuffer(int numSteps)
{
	_x.CalculateGradient();
	_y.CalculateGradient();
	_height.CalculateGradient();

	float step = 1.0f / numSteps;
	_buffer._buffer.reserve((numSteps+ 1) * 2);
	for (int i = 0; i <= numSteps; i++)
	{
		float sectorTime = step * i;

		if (sectorTime >= 0.99f)
			sectorTime = 0.99f;

		float gradX = _x.getGlobalGradient(sectorTime);
		float gradY = _y.getGlobalGradient(sectorTime);

		float stripX = _x.getGlobalFrame(sectorTime);
		float stripY = _y.getGlobalFrame(sectorTime);

		math::Vector3 grad = math::Vector3(gradX, gradY, 0.0f);
		math::Vector3 crossGrad = grad^math::Vector3(0.0f, 0.0f, 1.0f);

		crossGrad.SetLength(_height.getGlobalFrame(sectorTime));

		_buffer._buffer.push_back(Render::QuadVert(stripX+crossGrad.x, stripY+crossGrad.y, 0.0f, Color::WHITE, sectorTime, 0.0f));
        
		_buffer._buffer.push_back(Render::QuadVert(stripX-crossGrad.x, stripY-crossGrad.y, 0.0f, Color::WHITE, sectorTime, 1.0f));    
	}
    
    int numTriangles = numSteps * 2;
    _buffer._ibuffer.reserve(numTriangles * 3);
    for (int i = 0; i < numTriangles ; ++i) {
        _buffer._ibuffer.push_back(i);
        _buffer._ibuffer.push_back(i+1);
        _buffer._ibuffer.push_back(i+2);
    } 
    _buffer.numVertices = _buffer._buffer.size();
    _buffer.numIndices = _buffer._ibuffer.size();
	_numSectors = numSteps;

	setStripTime(0.f);
}

void StripEffect::setKeyColor(int key, const Color& color)
{
	_buffer._buffer[key*2].color = MAKECOLOR4(color.alpha, color.red, color.green, color.blue);
	_buffer._buffer[key*2+1].color = MAKECOLOR4(color.alpha, color.red, color.green, color.blue);
}

void StripEffect::setColor(const Color &color)
{
	for (size_t i = 0 ; i < _buffer._buffer.size() ; ++i)
	{
		_buffer._buffer[i].color = MAKECOLOR4(color.alpha, color.red, color.green, color.blue);
	}
}

void StripEffect::setStripTime(float stripTime)
{
	_stripTime = stripTime;
	if(_numSectors <= 0)
	{
		return;
	}
	float step = 1.0f/(_numSectors+1);
	for (int i = 0; i < (_numSectors+1); i++)
	{
		float stripFactor = _stripTime*(1.0f+2*_stripLength)-_stripLength;
		float key = i*step;
		float alpha;
		if (key < stripFactor)
		{
			alpha = 1.f-(stripFactor-key)*(1.0f/_stripLength);
			if (alpha < 0.0f)
			{
				alpha = 0.0f;
			}
		}
		else
		{
			alpha = 1.f-(key-stripFactor)*(1.0f/_stripLength);
			if (alpha < 0.0f)
			{
				alpha = 0.0f;
			}
		}
		setKeyColor(i, Color(_color.red, _color.green, _color.blue, math::lerp<unsigned char>(0, _color.alpha, alpha)));
	}
}

void StripEffect::setStripTimeTile(float stripTime, const Color &color)
{
	_stripTime = stripTime;

	for (int i = 0; i < (_numSectors+1); i++)
	{
		setKeyColor(i, color);
	}
}

math::Vector3 StripEffect::getStripPosition()
{
	float stripFactor = _stripTime*(1.0f+2*_stripLength);
	if (stripFactor < 0.0f)
	{
		return math::Vector3(_x.getGlobalFrame(0.0f), _y.getGlobalFrame(0.0f), 0.0f);
	}
	if (stripFactor > 1.0f)
	{
		return math::Vector3(_x.getGlobalFrame(1.0f), _y.getGlobalFrame(1.0f), 1.0f);
	}

	return math::Vector3(_x.getGlobalFrame(stripFactor), _y.getGlobalFrame(stripFactor), 0.0f);
}

void StripEffect::Draw()
{
    static VertexBufferIndexed tmp;
	//Render::device.SetCurrentMatrix(Render::TEXTURE);
	//Render::device.MatrixTranslate(math::Vector3(-_stripTime*_textureOffset, 0.0f, 0.0f));
	//Render::device.MatrixScale(_textureScale, 1.0f, 1.0f);
	//Render::device.SetCurrentMatrix(Render::MODELVIEW);
	tmp.isStatic = true;
    tmp._buffer = _buffer._buffer;
    tmp._ibuffer = _buffer._ibuffer;
    tmp.numIndices = _buffer.numIndices;
    tmp.numVertices = _buffer.numVertices;
	tmp._quadBuffer = _buffer._quadBuffer;
	//tmp.vertexAttribs = _buffer.vertexAttribs;
    
	for (size_t i = 0; i < _buffer.numVertices; ++i) {
        Render::QuadVert &qv = tmp._buffer[i];
        qv.u +=  -_stripTime*_textureOffset;
    }
	tmp.UploadVertex();
	tmp.UploadIndex();
    Render::device.DrawIndexed(&tmp);

	tmp.Reinit(0, 0);
	tmp._ibuffer.clear();
	tmp._buffer.clear();

	//Render::device.SetCurrentMatrix(Render::TEXTURE);
	//Render::device.ResetMatrix();
	//Render::device.SetCurrentMatrix(Render::MODELVIEW);

}

void StripEffect::DrawSimple()
{
	Render::device.DrawIndexed(&_buffer);
}
