#pragma once

#include "UpdateLevelItem.h"
#include "AvatarContainer.h"

namespace Card 
{
	class LevelStar : public Image
	{
	public:
		int index;
		LevelStar();
		void init(bool set);
		void runEffect();
		void Update(float dt, EffectsContainer *effCont);
		void Draw();
	private:
		float time;
		bool show;
		bool run;
	};

	// маркер c номером уровня
	class LevelMarker :public UpdateLevelItem
	{
	public:
		~LevelMarker();
		virtual void init(Container* parent, rapidxml::xml_node<>* info);
		virtual void Draw(const FPoint& shift);
		virtual void MouseDown(const FPoint& mouse_position, bool& capture);
		virtual void OnClick();
		virtual void Update(float dt);
		virtual void InitMarker();
		virtual FPoint getCenterPosition() const;
		virtual void startVisualization();
		int GetLevel() const { return level; }
		void runStarEffects();
	protected:
		virtual void endVisualization();
		
	private:
		// уровень
		int level;

		// настройка для множества маркеров
		struct Setting {
			ListImage close;
			ListImage open;
			ListImage openActive;
			ListImage openPressed;
			Image stars[3];
			IRect bounds;
		};
		LevelStar stars[3];
		Setting* setting;
		EffectsContainer _effCont;

		// загрузка настроек маркеров
		static bool settings_loaded;
		static void load_settings();
		static std::map<std::string, Setting> settings;

	public:
		static Render::Texture *default_light_field;
		static FPoint LEVEL_MARKER_CENTER;
		static float LOW_LIGHT_LEVEL;
	};
} // namespace Card