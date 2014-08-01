#pragma once

#include "GameFieldAddress.h"
#include "GameFieldController.h"
#include "RenderTargetHolder.h"

namespace Tutorial
{

class AreaHighlighter : public GameFieldController
{
private:
	RenderTargetHolder _lightMap;
	Render::ShaderProgram *_shader;
	Render::Texture *_cellLight;

	std::vector<IRect> _prev;
	std::vector<IRect> _current;
	float _opacity;

	void UpdateTarget();
	void DrawArea(const std::vector<IRect>& cells, float scale);
public:
	AreaHighlighter(const std::vector<IRect>& cells, float opacity);

	void SetShape(const std::vector<IRect>& cells);

	void Hide();

	void DrawAbsolute() override;
	void Update(float dt) override;
	bool isFinish() override;
};

class ShowDirectionController : public GameFieldController
{
private:
	Render::Texture *tex;

	float _alpha;

	FPoint _start;
	FPoint _dpos;
	FPoint vt, vn;
	int _count;
	float _waveTimer;

	float WaveFunc(float t) const;
public:
	// from, to - координаты экрана
	ShowDirectionController(FPoint from, FPoint to);

	void Start();
	void End();

	void DrawAbsolute() override;
	void Update(float dt) override;
	bool isFinish() override;
};

class ShowMoveFinger : public GameFieldController
{
private:
	Render::Texture *_tex;
	std::vector<FPoint> _chain;
	std::vector<float> _chainT;

	FPoint getChainPos(float t) const;
public:
	ShowMoveFinger(std::vector<FPoint> chain, float startTime);

	void Finish();

	void DrawAbsolute() override;
	void Update(float dt) override;
	bool isFinish() override;
};

class ShowText : public GameFieldController
{
	FPoint _pos;
	Render::Text *_text;
	bool _hiding;
public:
	ShowText(FPoint pos, const std::string& textID);

	void Finish();

	void DrawAbsolute() override;
	void Update(float dt) override;
	bool isFinish() override;
};

class PointArrowController : public GameFieldController
{
	std::vector<FRect> _rects;
	Render::Texture *_arrowTex;
	float _pulseTimer;
public:
	PointArrowController(const std::vector<IPoint> &squares, bool screenPoints = false);

	void Finish();
	void DrawAbsolute() override;
	void Update(float dt) override;
	bool isFinish() override;
};

} // namespace Tutorial