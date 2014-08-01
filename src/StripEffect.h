#pragma once

class StripEffect
{
	SplinePath<float> _x;
	SplinePath<float> _y;
	SplinePath<float> _height;

	VertexBufferIndexed _buffer;

	float _stripTime;
	float _textureOffset;
	float _textureScale;
	float _stripLength;
	int _numSectors;

public:
	Color _color;
	StripEffect(const Color &color = Color::WHITE);

	void addPathKey(float xValue, float yValue, float heightValue);
	void CalculateBuffer(int numSteps);
	void setKeyColor(int key, const Color& color);
	void setColor(const Color &color);
	void Draw();
	void DrawSimple();

	void setTextureSpeed(float textureOffset)
	{
		_textureOffset = textureOffset;
	}

	float getTextureSpeed()
	{
		return _textureOffset;
	}

	int getKeysCount()
	{
		return _buffer._buffer.size()/2;
	}

	void setTextureScale(float textureScale)
	{
		_textureScale = textureScale;
	}

	void setStripLength(float stripLength)
	{
		_stripLength = stripLength;
	}

	void setStripTime(float stripTime);
	void setStripTimeTile(float stripTime, const Color &color);


	math::Vector3 getStripPosition();
};