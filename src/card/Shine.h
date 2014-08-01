#pragma once

class RenderTargetHolder;

namespace Card {

class Shine
{
public:
	Shine();
	void init(rapidxml::xml_node<>* info);
	void Draw(RenderTargetHolder* target, float shift);
	void DrawEffect(float shift);
	void Update(float dt);
	void InitEpisods(int episod_index);
	void OpenEpisod();
private:
	static const float TRANSPARENT_BORDER_SIZE;
	static float SHINE_X_OFFSET;
	// границы участков эпизодов
	std::vector<std::vector<FPoint>> borders;
	// из скольки кусочков рисуется граница
	int piece_amount;
	// какую границу сейчас рисуем
	int curren_border;
	// текущая граница
	std::vector<FPoint> path;

	// границы для анимации
	std::vector<FPoint> anim_path0;
	std::vector<FPoint> anim_path1;
	float timer;
	float time;

	// с каким смещением в последний раз рисовали
	float last_render_shift;

	float min_border;
	float max_border;

	// контейнер для эффекта на границе, при открытии эпизода
	EffectsContainer _effCont;
	ParticleEffect *_eff;
	bool needDrawEffect;
	bool effectFinishing;
private:
	void initBorder(std::vector<FPoint> &path, int index_border);
	void CalculateBounds();
};

} // namespase