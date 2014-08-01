#include "stdafx.h"
#include "BoostersTransitionWidget.h"
#include "MyApplication.h"
#include "GameInfo.h"
#include "GameField.h"

BoostersTransitionWidget::BoostersTransitionWidget(const std::string& name_, rapidxml::xml_node<>* xmlElement)
	: Widget(name_)
	, _boosterBackTex(NULL)
	, _boosters_count(0)
	, _timer(0.f)
	, _state(NONE)
	, _boostsReadyToNextState(0)
	, _boostersLoaded(false)
{

}

void BoostersTransitionWidget::LoadBoosters()
{
	_boosterBackTex = Core::resourceManager.Get<Render::Texture>("BoosterBack");

	Xml::RapidXmlDocument doc("FlyingBoosters.xml");

	rapidxml::xml_node<> *booster = doc.first_node()->first_node("Booster");
	while (booster) {
		FlyingBooster fb;
		fb.name = Xml::GetStringAttributeOrDef(booster, "name", "");
		fb.runEffectName = Xml::GetStringAttributeOrDef(booster, "effectName", "");
		fb.usageTime = Xml::GetFloatAttributeOrDef(booster, "usageTime", 0.5f);
		fb.waitY = Xml::GetIntAttributeOrDef(booster, "waitY", 220);

		rapidxml::xml_node<> *icon = booster->first_node("icon");
		fb.textureid = Xml::GetStringAttributeOrDef(icon, "textureid", "");
		fb.iconOffset.x = Xml::GetIntAttributeOrDef(icon, "x", 0);
		fb.iconOffset.y = Xml::GetIntAttributeOrDef(icon, "y", 0);

		rapidxml::xml_node<> *pathin = booster->first_node("pathIn");
		fb.timeIn = Xml::GetFloatAttributeOrDef(pathin, "time", 0.5f);

		rapidxml::xml_node<> *pathout = booster->first_node("pathOut");
		fb.timeOut = Xml::GetFloatAttributeOrDef(pathout, "time", 0.5f);
		int pathoutType = Xml::GetIntAttributeOrDef(pathout, "type", 1);
		fb.pathoutType = pathoutType;

		fb.boosterIconTex = Core::resourceManager.Get<Render::Texture>(fb.textureid);
		fb.state = NONE;

		fb.waitTime = 0;		// будет определена в Init
		fb.pos = FPoint(0, 0);	// будет определена в Init

		_allflyingBoosters.push_back(fb);

		booster = booster->next_sibling("Booster");
	}

	_boostersLoaded = true;
}

void BoostersTransitionWidget::Init(const Message& message)
{
	_flyingBoosters.clear();
	_boostsReadyToNextState = 0;
	_timer = 0;
	_boosters_count = 0;
	_state = NONE;

	int count = message.getVariables().getInt("count");
	IPoint half_screen = IPoint(MyApplication::GAME_WIDTH/2, MyApplication::GAME_HEIGHT/2);
	IPoint half_backTex = IPoint(_boosterBackTex->getBitmapRect().Width() / 2, _boosterBackTex->getBitmapRect().Height() / 2);
	std::string device = gameInfo.getGlobalString("device_id", "");

	float delay = 0.f;
	while (true) {
		if (message.getVariables().findName("boostType" + utils::lexical_cast(_boosters_count+1))) {
			_boosters_count++;
			std::string boost_name = message.getVariables().getString("boostType" + utils::lexical_cast(_boosters_count));
			bool found = false;
			for (int i = 0; i < (int)_allflyingBoosters.size(); i++) {
				if (_allflyingBoosters[i].name == boost_name) {
					found = true;
					FlyingBooster fb = _allflyingBoosters[i];

					fb.pathIn.Clear();
					if (count == 1) {
						fb.pos = FPoint(half_screen.x - half_backTex.x, -20.f - half_backTex.y*2);
					} else if (count == 2) {
						if (_boosters_count == 1) {
							fb.pos = FPoint(half_screen.x - half_backTex.x - 100.f, -20.f - half_backTex.y*2);
						} else {
							fb.pos = FPoint(half_screen.x - half_backTex.x + 100.f, -20.f - half_backTex.y*2);
						}
					} else if (count == 3) {
						if (_boosters_count == 1) {
							fb.pos = FPoint(half_screen.x - half_backTex.x - 180.f, -20.f - half_backTex.y*2);
						} else if (_boosters_count == 2) {
							fb.pos = FPoint(half_screen.x - half_backTex.x, -20.f - half_backTex.y*2);
						} else {
							fb.pos = FPoint(half_screen.x - half_backTex.x + 180.f, -20.f - half_backTex.y*2);
						}
					} else {
						Assert(false); // может быть только 3 бустера
					}
					fb.pathIn.addKey(fb.pos);
					if (device == "ipad") {
						fb.waitY += 0; 
					} else if (device == "iphone5") {
						fb.waitY += 50;
					} else {
						fb.waitY += 0;
					}
					fb.pathIn.addKey(fb.pos + IPoint(0, fb.waitY));
					fb.pathIn.CalculateGradient();

					fb.pathOut.Clear();
					fb.pathOut.addKey(fb.pos + IPoint(0, fb.waitY));
					if (fb.pathoutType == 0) {
						//остаёмся на месте
						fb.pathOut.addKey(fb.pos + IPoint(0, fb.waitY));
					}else if (fb.pathoutType == 1) {
						// к количеству ходов
						if (device == "ipad") {
							fb.pathOut.addKey(FPoint(250, 930));
						} else if (device == "iphone5") {
							fb.pathOut.addKey(FPoint(190, 1020));
						} else {
							fb.pathOut.addKey(FPoint(190, 900));
						}
					} else if (fb.pathoutType == 2) {
						// к месту откуда вылетают суперфишки
						fb.pathOut.addKey(FPoint(400, 700));
					} else if (fb.pathoutType == 3) {
						// к центру экрана
						fb.pathOut.addKey(FPoint(half_screen.x - half_backTex.x, half_screen.y - half_backTex.y));
					} else {
						Assert(false); // непонятно куда
					}
					fb.pathOut.CalculateGradient();

					fb.waitTime = delay;
					delay = delay + fb.usageTime + fb.timeOut;
					_flyingBoosters.push_back(fb);
				}
			}
			Assert(found);	// такого буста нет
		} else {
			break;
		}
	}

	// сразу запустим
	_state = MOVEIN;
	for (int i = 0; i < _boosters_count; i++) _flyingBoosters[i].state = MOVEIN;
}

void BoostersTransitionWidget::Update(float dt)
{
	if (_state == MOVEIN) {
		_timer += dt;
		for (int i = 0; i < _boosters_count; i++) {
			if (_flyingBoosters[i].state == MOVEIN) {
				if (_flyingBoosters[i].timeIn >= _timer) {
					float t = math::ease(_timer / _flyingBoosters[i].timeIn, 0.3f, 0.3f);
					_flyingBoosters[i].pos = _flyingBoosters[i].pathIn.getGlobalFrame(t);
				} else {
					_flyingBoosters[i].state = WAIT;
					_boostsReadyToNextState++;
				}
			}
		}
		if (_boostsReadyToNextState == (int)_flyingBoosters.size()) {
			_boostsReadyToNextState = 0;
			_timer = 0;
			_state = WAIT;
		}
	} else if (_state == RUNING) {
		_timer += dt;
		for (int i = 0; i < _boosters_count; i++) {
			if (_flyingBoosters[i].state == WAIT && _flyingBoosters[i].waitTime <= _timer) {
				_flyingBoosters[i].state = MOVEOUT;
			}
			if (_flyingBoosters[i].state == MOVEOUT) {
				if (_flyingBoosters[i].timeOut >= (_timer - _flyingBoosters[i].waitTime)) {
					float t = math::ease((_timer - _flyingBoosters[i].waitTime) / _flyingBoosters[i].timeOut, 0.5f, 0.7f);
					_flyingBoosters[i].pos = _flyingBoosters[i].pathOut.getGlobalFrame(t);
				} else {
					_flyingBoosters[i].state = RUNING;

					ParticleEffectPtr eff = _effCont.AddEffect(_flyingBoosters[i].runEffectName);
					eff->posX = _flyingBoosters[i].pos.x + (_boosterBackTex->getBitmapRect().Width() / 2);
					eff->posY = _flyingBoosters[i].pos.y + (_boosterBackTex->getBitmapRect().Height() / 2);
					eff->Reset();

					Message msg = Message("RunBoost");
					msg.getVariables().setString("name", _flyingBoosters[i].name);
					Core::guiManager.getLayer("GameLayer")->getWidget("GameField")->AcceptMessage(msg);
				}
			}
			if (_flyingBoosters[i].state == RUNING && (_flyingBoosters[i].usageTime + _flyingBoosters[i].waitTime) <= _timer ) {
				_flyingBoosters[i].state = FINISHED;
				_boostsReadyToNextState++;

				// Проверим, нужно ли показать подсказку после буста
				if (!gameInfo.getLocalBool("TutorialAfter" + _flyingBoosters[i].name, false)) {
					Core::LuaCallVoidFunction("showTutorialAfterBoost", "TutorialAfter" + _flyingBoosters[i].name);
					gameInfo.setLocalBool("TutorialAfter" + _flyingBoosters[i].name, true);
				}
			}
		}
		if (_boostsReadyToNextState == (int)_flyingBoosters.size()) {
			_state = FINISHED;
		}
	}
	if (_state == FINISHED) {
		if (_effCont.IsFinished()) {
			Core::guiManager.getLayer("GameLayer")->getWidget("GameField")->AcceptMessage(Message("BoostsBeforeStartFinished"));
		}
	}
	_effCont.Update(dt);
}

void BoostersTransitionWidget::Draw()
{
	if (_state != NONE && _state != FINISHED) {
		for (int i = 0; i < _boosters_count; i++) {
			if (_flyingBoosters[i].state != FINISHED && _flyingBoosters[i].state != RUNING) {
				_boosterBackTex->Draw(_flyingBoosters[i].pos);
				_flyingBoosters[i].boosterIconTex->Draw(_flyingBoosters[i].pos + _flyingBoosters[i].iconOffset);
			}
		}
	}
	_effCont.Draw();
}

bool BoostersTransitionWidget::MouseDown(const IPoint& mouse_pos)
{
	GameField::Get()->MouseDown(mouse_pos);
	return true;
}

void BoostersTransitionWidget::MouseUp(const IPoint &mouse_pos)
{
	GameField::Get()->MouseUp(mouse_pos);
}

void BoostersTransitionWidget::AcceptMessage(const Message& message)
{
	if (message.is("Init")) {
		if (!_boostersLoaded) LoadBoosters();
		Init(message);
	} else if (message.is("StartRuning")) {
		_state = RUNING;
	} else if (message.is("Layer", "LayerDeinit")) {
		if (_state == FINISHED) _state = NONE;
	}
}