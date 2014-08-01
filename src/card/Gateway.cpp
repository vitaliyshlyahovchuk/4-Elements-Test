#include "stdafx.h"
#include "Gateway.h"
#include "GameInfo.h"
#include "LevelMarker.h"
#include "MyApplication.h"
#include "BigBackground.h"

namespace Card
{
	bool Gateway::settings_loaded = false;
	std::map<std::string, Gateway::Setting> Gateway::settings;

	FPoint Gateway::GATE_CENTER;

	void Gateway::load_settings() {
		settings_loaded = true;
		Xml::RapidXmlDocument fileSettings("maps/gatewaySettings.xml");

		rapidxml::xml_node<>* root = fileSettings.first_node();

		GATE_CENTER.x = Xml::GetFloatAttribute(root, "center_x");
		GATE_CENTER.y = Xml::GetFloatAttribute(root, "center_y");

		std::map<std::string, FPoint> image_positions;
		for (rapidxml::xml_node<>* e = root->first_node("positions")->first_node(); e; e = e->next_sibling()) {
			image_positions[Xml::GetStringAttribute(e, "id")] = FPoint(e);
		}
		
		for (rapidxml::xml_node<>* e = root->first_node("subtype"); e; e = e->next_sibling()) {
			std::string id = Xml::GetStringAttribute(e, "id");
			settings[id] = Setting();
			Setting& image_setting = settings[id];

			for (rapidxml::xml_node<>* im = e->first_node(); im; im = im->next_sibling()) {
				std::string xml_type = Xml::GetStringAttribute(im, "type");
				Image* image = nullptr;
				if (xml_type == "closed") {
					image = &image_setting.closed;
				} else if (xml_type == "open_active") {
					image = &image_setting.open_active;
				} else if (xml_type == "open_pressed") {
					image = &image_setting.open_pressed;
				} else if (xml_type == "key") {
					image = &image_setting.key;
				} else if (xml_type == "key_shadow") {
					image = &image_setting.key_shadow;
				} else if (xml_type == "key_glow") {
					image = &image_setting.key_glow;
				} else if (xml_type == "gate_open") {
					image = &image_setting.gate_open;
				} else if (xml_type == "gate_close") {
					image = &image_setting.gate_close;
				} else {
					Assert(false);
				}
				std::string image_id = Xml::GetStringAttribute(im, "id");
				image->texture = Core::resourceManager.Get<Render::Texture>(image_id);
				image->position = image_positions[image_id];
				image_setting.bounds = MapItem::unionRect(image_setting.bounds, image->texture->getBitmapRect().MovedTo(image->position.Rounded()));
			}

			for (int i = 0; i < 3; ++i) {
				Slot* slot = &image_setting.slots[i];
				slot->position = FPoint(65.f * (i-1), 0.f);
				slot->type =i - 1;
				slot->back.texture = Core::resourceManager.Get<Render::Texture>("LevelMarkerBack");
				slot->back.position = image_positions["LevelMarkerBack"];
				slot->lock.texture = Core::resourceManager.Get<Render::Texture>("LevelMarkerLock");
				slot->lock.position = image_positions["LevelMarkerLock"];
				slot->tick.texture = Core::resourceManager.Get<Render::Texture>("LevelMarkerTick");
				slot->tick.position = image_positions["LevelMarkerTick"];
			}

		}

	}

	Gateway::AppearanceItem::AppearanceItem()
		:timer(0.f)
		,appearanceTime(1.f)
		,appearancePause(0.f)
		,position(0.f,0.f)
		,alpha(0.f)

	{}

	float Gateway::HelpSlot::APPEARANCE_Y = 10.f;
	float Gateway::HelpSlot::APPEARANCE_X = 4.f;

	Gateway::HelpSlot::HelpSlot()
		:AppearanceItem()
		,slot(NULL)
		,unlock(false)
	{}

	void Gateway::HelpSlot::Update(float dt) {
		if (activeVisualization) {
			if (appearancePause > 0.f) {
				appearancePause -= dt;
				position.y = -APPEARANCE_Y;
				position.x = APPEARANCE_X * static_cast<float>(slot->type);
				alpha = 0.f;
				return;
			}
			timer += dt;
			if (timer > appearanceTime) {
				timer = appearanceTime;
				activeVisualization = false;
			}
			float t = timer / appearanceTime;
			position.x = APPEARANCE_X * (1.f - t) * static_cast<float>(slot->type);
			position.y = -APPEARANCE_Y * (1.f - math::EaseOutElastic(t, 3.f, 0.5f));
			alpha = math::clamp(0.f, 1.f, math::EaseOutExpo(t + 0.1f));
		}
	}

	void Gateway::HelpSlot::Draw() {
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(position + slot->position);
		Render::BeginAlphaMul(alpha);
		slot->back.Draw();
		if (unlock) {
			slot->tick.Draw();
		} else {
			slot->lock.Draw();
		}
		Render::EndAlphaMul();
		Render::device.PopMatrix();
	}

	Gateway::Key::Key()
		:timer(0.f)
	{}

	void Gateway::Key::Update(float dt) {
		timer += dt;
		position.x = 1.5f * math::sin(timer);
		position.y = 2.5f * math::cos(timer);

		shadowPosition.x = position.x;
		shadowPosition.y = position.y * 0.33f;
	}

	void Gateway::Key::Draw() {
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(position);
		key->Draw();
		Render::device.PopMatrix();
	}

	void Gateway::Key::DrawShadow() {
		Render::device.PushMatrix();
		key_glow->Draw();
		Render::device.MatrixTranslate(shadowPosition);
		key_shadow->Draw();
		Render::device.PopMatrix();
	}

	Gateway::Gateway()
		: UpdateLevelItem()
		, effectContainer(new EffectsContainer())
		, drawOpenAnimation(false)
		, drawFrozenFrame(false)
	{
	}

	Gateway::~Gateway() {
		delete effectContainer;
	}

	void Gateway::init(Container* parent, rapidxml::xml_node<>* info) {
		if (!settings_loaded) {
			load_settings();
		}
		UpdateLevelItem::init(parent, info);
		Assert2(settings.find(Xml::GetStringAttribute(info, "subtype")) != settings.end(), "Нет настройки для маркера ворот: " + GetName());
		setting = &settings[Xml::GetStringAttribute(info, "subtype")];
		gatePosition.x = Xml::GetFloatAttributeOrDef(info, "gate_x", 0.f);
		gatePosition.y = Xml::GetFloatAttributeOrDef(info, "gate_y", 0.f);
		pressed = false;
		rect = setting->bounds.MovedBy(position.Rounded());

		openAnimationOffset.x = Xml::GetFloatAttributeOrDef(info, "animation_x", 0.f);
		openAnimationOffset.y = Xml::GetFloatAttributeOrDef(info, "animation_y", 0.f);
		drawOpenAnimation = false;
		drawFrozenFrame = false;

		// аватарки друзей-помощников
		for (int i = 0; i < 3; ++i) {
			helpSlots[i].slot = &setting->slots[i];
			helpSlots[i].alpha = 1.f;
		}

		key.key = &setting->key;
		key.key_shadow = &setting->key_shadow;
		key.key_glow = &setting->key_glow;
	}

	void Gateway::InitMarker() {
		UpdateLevelItem::InitMarker();
		effectContainer->Finish();
		if(isOpen) {
			if (!isComplete) {
				//приоткрывается немного
				for (int i = 0; i < 3; ++i) {
					helpSlots[i].activeVisualization = false;
				}
				updateTickets();
 			} else {
				//открывается полностью
			}
		}
	}

	void Gateway::AcceptMessage(const Message &message) {
		if(message.is("OnSetFriendAvatars")) {
			//todo Сделать нормальное сообщение - обновить слоты билетиков
			updateTickets();
		}
	}

	FPoint Gateway::getCenterPosition() const {
		return GATE_CENTER;
	}

	void Gateway::updateTickets() {
		for (int i = 0; i < 3; ++i) {
			std::string uid = gameInfo.getLocalString("ticket" + utils::lexical_cast(i+1), "");
			if (uid != "") {
				helpSlots[i].unlock = true;
			} else {
				helpSlots[i].unlock = false;
			}
		}
	}
	

	void Gateway::Draw(const FPoint& shift) {
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(shift + position);
		if(isOpen) {
			if (!drawOpenAnimation && !isComplete) {
				if (pressed) {
					setting->open_pressed.Draw();
				} else {
					setting->open_active.Draw();
				}
				setting->gate_close.Draw(gatePosition);
			}

			if (!drawOpenAnimation && isComplete && !drawFrozenFrame) {
				setting->gate_open.Draw(gatePosition);
			}

			if (drawOpenAnimation || drawFrozenFrame) gateOpenAnimation->Draw();

			if (!isComplete) {
				for (int i = 0; i < 3; ++i) {
					helpSlots[i].Draw();
				}
				if (drawFrozenFrame) drawFrozenFrame = false;
			}
		} else {
			setting->gate_close.Draw(gatePosition);
			setting->closed.Draw();
		}
		if (!drawOpenAnimation && !isComplete) {
			key.DrawShadow();
			effectContainer->Draw();
			key.Draw();
		}
		Render::device.PopMatrix();
		if (!drawOpenAnimation && !isComplete) {
			UpdateLevelItem::Draw(shift);
		}
	}


	void Gateway::startVisualization() {
		UpdateLevelItem::startVisualization();
		visualizationTime = 2.f;
		if (!isComplete) {
			for (int i = 0; i < 3; ++i) {
				helpSlots[i].activeVisualization = true;
				helpSlots[i].timer = 0.f;
				helpSlots[i].appearancePause = visualizationTime + (0.2f * i);
				helpSlots[i].appearanceTime = 2.f;
				helpSlots[i].Update(0.f);
			}
			updateTickets();
			// у первых ворот слоты показывать не будем
			if (gameInfo.getLocalInt("current_marker") < 16 ) {
				for (int i = 0; i < 3; ++i) helpSlots[i].activeVisualization = false;
			}
			// пробуем запустить поиск ключа
			Core::LuaDoString("needTicket:tryLaunchFindKey()");
		} else {
			MM::manager.PlaySample("MapGatewayOpen");
			gateOpenAnimation = new CardFlashAnimationPlayer("card_elements", "gate", gatePosition + openAnimationOffset);
			drawOpenAnimation = true;
			drawFrozenFrame = false;
		}
	}

	void Gateway::endVisualization() {
		UpdateLevelItem::endVisualization();
		if (isComplete && isOpen && gameInfo.getLocalBool("GateWasOpened", false)) {
			gameInfo.setLocalBool("GateWasOpened", false);
		}
	}

	void Gateway::Update(float dt) {
		UpdateLevelItem::Update(dt);
		if (isOpen) {
			if (!isComplete) {
				for (int i = 0; i < 3; ++i) {
					helpSlots[i].Update(dt);
				}
			}
			if (drawOpenAnimation) {
				gateOpenAnimation->Update(dt);
				if (gateOpenAnimation->isFinish()) {
					drawOpenAnimation = false;
					drawFrozenFrame = true;
				}
			}
		}
		key.Update(dt);
		effectContainer->Update(dt);
	}

	void Gateway::MouseDown(const FPoint& mouse_position, bool& capture) {
		int extreme_level = gameInfo.getLocalInt("extreme_level", 0);
		if( extreme_level > 16) {
			UpdateLevelItem::MouseDown(mouse_position, capture);
		}
	}

	void Gateway::OnClick() {
		Core::messageManager.putMessage(Message("Gateway"));
	}

	bool Gateway::hitTest(const FPoint & position) const {
		return isOpen && !isComplete && rect.Contains(position.Rounded());
	}

	void Gateway::removeFrozenAnimation() {
		if (!drawOpenAnimation && drawFrozenFrame) drawFrozenFrame = false;
	}

} // namespace Card