#pragma once

#include "UpdateLevelItem.h"
#include "AvatarContainer.h"
#include "FriendInfo.h"
#include "CardFlashAnimationPlayer.h"

namespace Card 
{

	// ворота перехода в новую локацию (эпизод)
	class Gateway : public UpdateLevelItem {
	public:
		Gateway();
		~Gateway();
		virtual void init(Container* parent, rapidxml::xml_node<>* info);
		virtual void Draw(const FPoint& shift);
		virtual void MouseDown(const FPoint& mouse_position, bool& capture);
		virtual void Update(float dt);
		virtual void OnClick();
		virtual void AcceptMessage(const Message &message);
		virtual FPoint getCenterPosition() const;
		virtual void InitMarker();
		virtual void startVisualization();
		void removeFrozenAnimation();
	protected:
		virtual bool hitTest(const FPoint & position) const;
		virtual void endVisualization();
	private:
		struct Slot {
			FPoint position;
			int type; // -1 - левый слот, 0 - средний слот, 1 - правый слот
			Image back;
			Image lock;
			Image tick;
		};
		// настройка для множества шлюзов
		struct Setting {
			Image gate_open;
			Image gate_close;
			Image key;
			Image key_shadow;
			Image key_glow;
			Slot slots[3];
			Image closed;
			Image open_active;
			Image open_pressed;
			IRect bounds;
		};
		Setting* setting;
		FPoint gatePosition;
		EffectsContainer* effectContainer;

		CardFlashAnimationPlayer* gateOpenAnimation;
		bool drawOpenAnimation;
		bool drawFrozenFrame;
		FPoint openAnimationOffset;

		struct AppearanceItem {
			AppearanceItem();

			bool activeVisualization; // запущен эффект появления
			float timer; // таймер
			float appearanceTime; // время появления
			float appearancePause; // пауза перед появлением
			FPoint position; // текущая позиция
			float alpha;

			virtual void Update(float dt) {};
			virtual void Draw() {}
		};

		//
		struct HelpSlot : public AppearanceItem {
			HelpSlot();
			Slot* slot; // настройка из setting
			bool unlock; // уже помогли, разблокировали или нашли ключ

			static float APPEARANCE_Y;
			static float APPEARANCE_X;
			
			virtual void Update(float dt);
			virtual void Draw();
		};

		// иконки помощи
		HelpSlot helpSlots[3];

		struct Key {
			Key();
			Image* key;
			Image* key_shadow;
			Image* key_glow;

			float timer;

			FPoint position;
			FPoint shadowPosition;

			void Update(float dt);
			void DrawShadow();
			void Draw();
		};

		// ключик
		Key key;
		// обновить состояние открытости слотов друзей/ключиков
		void updateTickets();

		// загрузка настроек 
		static bool settings_loaded;
		static void load_settings();
		static std::map<std::string, Setting> settings;
	public:
		static FPoint GATE_CENTER;
	};
} // namespace Card