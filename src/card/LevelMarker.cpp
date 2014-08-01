#include "stdafx.h"
#include "LevelMarker.h"
#include "GameInfo.h"
#include "LevelInfoManager.h"
#include "FriendInfo.h"
#include "Marketing/RatingControl.h"
#include "MyApplication.h"
#include "BigBackground.h"

namespace Card
{
	bool LevelMarker::settings_loaded = false;
	std::map<std::string, LevelMarker::Setting> LevelMarker::settings;

	FPoint LevelMarker::LEVEL_MARKER_CENTER;

	LevelStar::LevelStar()
		: Image()
		, show(false)
		, run(false)
		, time(0.f)
		, index(0)
	{
	}

	void LevelStar::init(bool set) {
		show = set;
		run = false;
	}

	void LevelStar::runEffect() {
		//Теперь полученные звёзды должны отображаться сразу			
		run = false;
		show = true;
		return;

		if (!show) {
			run = true;
			time = -index*0.5f - 0.001f;
		}
	}

	void LevelStar::Update(float dt, EffectsContainer *effCont) {
		if (run) {
			if (time < 0) {
				time += dt;
				if (time >= 0) {
					time = 0.f;
				}
				return;
			} else {
				time += dt;
				if (time > 0.5f) { // time > число (число - время проигрывания эффекта CardStarAppear..N)
					run = false;
					show = true;
				}
			}
		}
	}

	void LevelStar::Draw() {
		if(show) {
			Image::Draw();
		}
	}


	void LevelMarker::load_settings() {
		settings_loaded = true;
		Xml::RapidXmlDocument fileSettings("maps/markerSettings.xml");
		rapidxml::xml_node<>* root = fileSettings.first_node();
		LEVEL_MARKER_CENTER.x = Xml::GetFloatAttribute(root, "center_x");
		LEVEL_MARKER_CENTER.y = Xml::GetFloatAttribute(root, "center_y");

		std::map<std::string, FPoint> image_positions;
		for (rapidxml::xml_node<>* e = root->first_node("positions")->first_node(); e; e = e->next_sibling()) {
			image_positions[Xml::GetStringAttribute(e, "id")] = FPoint(e);
		}
		
		for (rapidxml::xml_node<>* e = root->first_node("subtype"); e; e = e->next_sibling()) {
			std::string id = Xml::GetStringAttribute(e, "id");
			settings[id] = Setting();
			Setting& setting = settings[id];
			int count_star = 0;

			for (rapidxml::xml_node<>* im = e->first_node(); im; im = im->next_sibling()) {
				std::string type = Xml::GetStringAttribute(im, "type");
				Image* image = nullptr;
				if (type == "open") {
					setting.open.push_back(Image());
					image = &setting.open.back();
				} else if (type == "open_active") {
					setting.openActive.push_back(Image());
					image = &setting.openActive.back();
				} else if (type == "open_pressed") {
					setting.openPressed.push_back(Image());
					image = &setting.openPressed.back();
				} else if (type == "close") {
					setting.close.push_back(Image());
					image = &setting.close.back();
				} else if (type == "star") {
					setting.stars[count_star] = Image();
					image = &setting.stars[count_star++];
				} else {
					Assert(false);
				}
				std::string image_id = Xml::GetStringAttribute(im, "id");
				image->texture = Core::resourceManager.Get<Render::Texture>(image_id);
				image->position = image_positions[image_id];
				setting.bounds = MapItem::unionRect(setting.bounds, image->texture->getBitmapRect().MovedTo(image->position.Rounded()));
			}
		}
	}

	void LevelMarker::init(Container* parent, rapidxml::xml_node<>* info) {
		UpdateLevelItem::init(parent, info);
		level = Xml::GetIntAttribute(info, "level");
		if (!settings_loaded) { load_settings(); };

		Assert2(settings.find(Xml::GetStringAttribute(info, "subtype")) != settings.end(), "Нет настройки для маркера уровня: " + GetName());
		setting = &settings[Xml::GetStringAttribute(info, "subtype")];
		rect = setting->bounds.MovedBy(position.Rounded());

		for (int i = 0; i < 3; ++i) {
			stars[i].index = i;
			stars[i].texture = setting->stars[i].texture;
			stars[i].position = setting->stars[i].position;
		}
	}

	LevelMarker::~LevelMarker() {
		_effCont.KillAllEffects();
	}


	void LevelMarker::Draw(const FPoint& shift) {
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(shift + position);
		if(isOpen) {

			ListImage& openList = pressed ? setting->openPressed : setting->openActive;

			if(activeVisualization && !isComplete) {
				float t = visualizationTimer / visualizationTime * 3;
				const float from_t = 0.0f;
				float alpha = math::clamp(0.f, 1.f, (t - from_t)/(1 - from_t));
				Render::BeginAlphaMul(1.f - alpha);
				for (ListImageIter i = setting->close.begin(), e = setting->close.end(); i != e; ++i) {
					i->Draw();
				}
				Render::EndAlphaMul();
				Render::BeginAlphaMul(alpha);
				for (ListImageIter i = openList.begin(), e = openList.end(); i != e; ++i) {
					i->Draw();
				}
				for (ListImageIter i = setting->open.begin(), e = setting->open.end(); i != e; ++i) {
					i->Draw();
				}
				for (int i = 0; i < 3; ++i) {
					stars[i].Draw();
				}
				Render::EndAlphaMul();
				_effCont.Draw();
			} else {
				for (ListImageIter i = openList.begin(), e = openList.end(); i != e; ++i) {
					i->Draw();
				}
				for (ListImageIter i = setting->open.begin(), e = setting->open.end(); i != e; ++i) {
					i->Draw();
				}
				for (int i = 0; i < 3; ++i) {
					stars[i].Draw();
				}
				_effCont.Draw();
			}
		} else {
			for (ListImageIter i = setting->close.begin(), e = setting->close.end(); i != e; ++i) {
				i->Draw();
			}
		}
		Render::FreeType::BindFont("LevelMarker");
		Render::PrintString(LEVEL_MARKER_CENTER, utils::lexical_cast(level + 1), 1.f, CenterAlign, CenterAlign);
		/*Render::device.SetTexturing(false);
		Render::DrawFrame(IRect(LEVEL_MARKER_CENTER.Rounded(), 2, 2));
		Render::device.SetTexturing(true);*/
		Render::device.PopMatrix();
		UpdateLevelItem::Draw(shift);
	}

	FPoint LevelMarker::getCenterPosition() const {
		return LEVEL_MARKER_CENTER;
	}

	void LevelMarker::InitMarker() {
		UpdateLevelItem::InitMarker();
		//Стартовая установка параметров
		int received_stars = levelsInfo.getLevelStars(level);
		for(int i =0; i < 3; i++) {
			stars[i].init(i < received_stars);
		}
	}

	void LevelMarker::MouseDown(const FPoint& mouse_position, bool& capture) {
		// пока не прошли пять уровней, не даем кликать на маркеры предыдущих уровней во время анимации перехода
		int extreme_level = gameInfo.getLocalInt("extreme_level", 0);
		if( level == extreme_level || extreme_level >= 5 || !gameInfo.getLocalBool("NeedShowStartLevelPanel") ) {
			UpdateLevelItem::MouseDown(mouse_position, capture);
		}
	}

	void LevelMarker::OnClick() {
		// установим текущий уровень
		levelsInfo.setCurrentLevel(level);
		Core::messageManager.putMessage(Message("LevelMarker"));
		gameInfo.setLocalBool("NeedShowStartLevelPanel", false);
		MM::manager.PlaySample("ButtonClick", false);
	}

	void LevelMarker::startVisualization() {
		UpdateLevelItem::startVisualization();
		if (!isComplete) {
			visualizationTime = 1.4f;
			ParticleEffect *eff = _effCont.AddEffect("LevelMarkerOpen");
			eff->SetPos(LEVEL_MARKER_CENTER.x, LEVEL_MARKER_CENTER.y);
			eff->Reset();

			// на 10, 20, 40, 70 уровне просим поставить нам рейтинг
#ifndef ENGINE_TARGET_WIN32
			if (level == 10 || level == 20 || level == 40 || level == 70) {
				std::string topLayerName = Core::mainScreen.GetTopLayer()->name;
				if (topLayerName == "CardLayer") {
					Marketing::RatingControl::Run();
					gameInfo.setLocalBool("NeedShowStartLevelPanel", false);
				}
			}
#endif
		} else {
			visualizationTime = 0.5f;
			int received_stars = levelsInfo.getLevelStars(level);
			for(int i =0; i < 3; i++) {
				if (i < received_stars) {
					stars[i].runEffect();
				}
			}
		}
	}

	void LevelMarker::Update(float dt) {
		UpdateLevelItem::Update(dt);
		_effCont.Update(dt);
		for (int i = 0; i < 3; ++i) {
			stars[i].Update(dt, &_effCont);
		}
	}

	void LevelMarker::endVisualization() {
		UpdateLevelItem::endVisualization();
	}

	void LevelMarker::runStarEffects() {
		if (isOpen && isComplete) {
			int received_stars = levelsInfo.getLevelStars(level);
			for(int i =0; i < 3; i++) {
				if (i < received_stars) {
					stars[i].runEffect();
				}
			}
		}
	}

}//namespace Card*/