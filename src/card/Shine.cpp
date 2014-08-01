#include "stdafx.h"
#include <limits>
#include "Shine.h"
#include "RenderTargetHolder.h"
#include "MyApplication.h"

using namespace Card;

const float Shine::TRANSPARENT_BORDER_SIZE = 40.f;
float Shine::SHINE_X_OFFSET = 0.f;

Shine::Shine()
	:last_render_shift(std::numeric_limits<float>::min())
	,time(6.f)
	,timer(std::numeric_limits<float>::max())
	,curren_border(0)
	,needDrawEffect(false)
	,effectFinishing(false)
{
	piece_amount = 768.f / 64.f;
	path.reserve(piece_amount + 1);
	for (int i = 0; i <= piece_amount; ++i) {
		path.push_back(FPoint());
	}
	SHINE_X_OFFSET = (MyApplication::GAME_WIDTH - 768.f) * 0.5f;
}
		
void Shine::init(rapidxml::xml_node<>* info) {
	borders.clear();
	for (rapidxml::xml_node<>* borderXml = info->first_node(); borderXml; borderXml = borderXml->next_sibling()) {
		std::vector<FPoint> keys;
		for (rapidxml::xml_node<>* keyXml = borderXml->first_node(); keyXml; keyXml = keyXml->next_sibling()) {
			keys.push_back(FPoint(keyXml));
		}
		borders.push_back(keys);
	}
	
}

void Shine::InitEpisods(int episod_index) {
	if (timer < time) {
		//анимация еще идет, ускорим ее
		Update(time - timer + 0.1f);
	}
	curren_border = episod_index;
	initBorder(path, curren_border);
	CalculateBounds();
	last_render_shift = std::numeric_limits<float>::min();
}

void Shine::initBorder(std::vector<FPoint> &path, int index_border) {
	SplinePath<FPoint> spline;
	const std::vector<FPoint>& keys = borders[index_border];
	for (size_t i = 0; i < keys.size(); ++i) {
		spline.addKey(keys[i]);
	}
	spline.CalculateGradient();
	float dt = 1.f / piece_amount;
	for (int i = 0; i < piece_amount; ++i) {
		path[i] = spline.getGlobalFrame(dt * i);
	}
	path[piece_amount] = spline.getGlobalFrame(1.f);
}

void Shine::CalculateBounds() {

	min_border = std::numeric_limits<float>::max();
	max_border = std::numeric_limits<float>::min();
	for (int i = 0; i <= piece_amount; ++i) {
		if (path[i].y > max_border) {
			max_border = path[i].y;
		}
		if (path[i].y < min_border) {
			min_border = path[i].y;
		}
	}
	max_border += TRANSPARENT_BORDER_SIZE;
}

void Shine::Draw(RenderTargetHolder* target, float shift) {
	if (std::fabs(shift - last_render_shift) > 0.1f || target->NeedRedraw()) {
		last_render_shift = shift;

		bool need_draw_border = max_border > -shift && min_border < MyApplication::GAME_HEIGHT - shift; 
		bool need_draw_shine = (max_border - TRANSPARENT_BORDER_SIZE) > -shift;

		target->BeginRendering(Color(0, 0, 0, 0));
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(SHINE_X_OFFSET, shift, 0.f));
		Render::device.SetTexturing(false);
		Render::BeginColor(Color::BLACK);
	
		// рисуем полупрозрачные области
		if (need_draw_border) {
			for (int i = 0; i < piece_amount; ++i) {
				math::Vector3 v1 = path[i];
				math::Vector3 v2 = path[i + 1];
				math::Vector3 v3 = v1 + math::Vector3(0.f, TRANSPARENT_BORDER_SIZE , 0.f);
				math::Vector3 v4 = v2 + math::Vector3(0.f, TRANSPARENT_BORDER_SIZE , 0.f);
				Render::device.TrueDraw(v1, v2, v3, v4, Color::BLACK, Color::BLACK, Color::BLACK_TRANSPARENT, Color::BLACK_TRANSPARENT);
			}
		}
		// рисуем засвеченные области
		if (need_draw_shine) {
			for (int i = 0; i < piece_amount; ++i) {
				math::Vector3 v1 = path[i];
				math::Vector3 v2 = path[i + 1];
				v1.y = v2.y = -shift;
				math::Vector3 v3 = path[i];
				math::Vector3 v4 = path[i + 1];
#ifdef _DEBUG
				//v3.y -= 2; v4.y -= 2; // чтобы видно было границу
#endif
				if (v1.y < v3.y && v2.y < v4.y) {
					Render::device.TrueDraw(v1, v2, v3, v4, Color::BLACK, Color::BLACK, Color::BLACK, Color::BLACK);
				}
			}
		}

		Render::EndColor();
		Render::device.SetTexturing(true);
		Render::device.PopMatrix();
		target->EndRendering();
	}
};

void Shine::DrawEffect(float shift)
{
	if (needDrawEffect) {
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(0, shift + min_border + 36.f, 0.f));
		_effCont.Draw();
		Render::device.PopMatrix();
	}
}

void Shine::OpenEpisod() {
	anim_path0 = path;
	std::copy(path.begin(), path.end(), anim_path0.begin());
	++curren_border;
	anim_path1 = path;
	initBorder(anim_path1, curren_border);
	timer = 0.f;

	needDrawEffect = true;
	effectFinishing = false;
	_eff = _effCont.AddEffect("NewChapterLine");
	_eff->SetPos(MyApplication::GAME_WIDTH/2, 0);
	_eff->Reset();
}

void Shine::Update(float dt) {
	if (timer < time) {
		timer += dt;
		float n = timer / time;
		for (int i = 0; i <= piece_amount; ++i) {
			path[i] = math::lerp(anim_path0[i], anim_path1[i], n);
		}
		last_render_shift = std::numeric_limits<float>::min();
		CalculateBounds();
	} else {
		// тень не движется, проверим не нужно ли завершить эффект
		if (needDrawEffect) {
			if (!effectFinishing) {
				_effCont.Finish();
				effectFinishing = true;
			}
			if (!_effCont.IsEffectAlive(_eff)) {
				needDrawEffect = false;
			}
		}
	}
	if (needDrawEffect) _effCont.Update(dt);
}