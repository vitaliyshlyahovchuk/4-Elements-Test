#include "stdafx.h"
#include "CardWidget.h"
#include "MapItem.h"
#include "Map.h"
#include "GameInfo.h"
#include "LevelInfoManager.h"
#include "CardFactory.h"
#include "Shine.h"
#include "BigBackground.h"
#include "FriendInfo.h"
#include "LevelMarker.h"
#include "DynamicScroller.h"
#include "MyApplication.h"
#include "ParalaxCloud.h"
#include "CardResourceManager.h"

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
#include "FBInterface.h"
#endif



namespace Card
{

#ifdef CARD_EDITOR
		rapidxml::xml_document<char> itemsSettings;
#endif
	
	CardWidget::CardWidget(const std::string &name, rapidxml::xml_node<>* xml_elem)
		:GUI::Widget(name, xml_elem)
		, _tapScroll(false)
		, _mouseDown(false)
		, _editor(false)
		, open_level_visualization(false)
		, scroll(nullptr)
		, background(nullptr)
		, _virtualScreen(MyApplication::GAME_WIDTH, MyApplication::GAME_HEIGHT, true)
		, _virtualScreen2(MyApplication::GAME_WIDTH, MyApplication::GAME_HEIGHT, true)
	{
		Card::Factory::registerItem();
		map = new Map();
		background = new BigBackground();

		BigBackground::OFFSET = (MyApplication::GAME_WIDTH - 640.f) * 0.5f;
		_cardShader = Core::resourceManager.Get<Render::ShaderProgram>("CardShader");
		Assert(_cardShader);
	}

	CardWidget::~CardWidget()
	{
		delete background;
		delete map;
		ReleaseCardResources();
	}

	void CardWidget::updateItems(bool obviously = false)
	{
		float low = draw_rect_shift - scroll->GetPosition();
		if( Card::cardResourceManager.IsLoadedAll() ) {
			scroll->Continue();
		} else if( !Card::cardResourceManager.IsLoadedArea(low, low + draw_rect.height) && !obviously) {
			scroll->Pause();
			return;
		}
			
		_scrollPos = scroll->GetPosition();
			
		draw_rect.y = draw_rect_shift - static_cast<int>(_scrollPos);
		update_rect.y = update_rect_shift - static_cast<int>(_scrollPos);

		draw_objects.clear();
		update_objects.clear();
		std::vector<MapItem*>& objects = map->getObjects();

		for(MapItem *item : objects)
		{
			if (item->needDraw(draw_rect)) {
				draw_objects.push_back(item);
			}
			if (item->needUpdate(update_rect)) {
				update_objects.push_back(item);
			}
		}

		std::sort(draw_objects.begin(), draw_objects.end(), MapItem::CompareByBound);
		background->setDrawRect(draw_rect);
		Card::cardResourceManager.Update(draw_rect.y + 0.f, draw_rect.y + draw_rect.height + 0.f, scroll->GetVelocity());
	}

	void CardWidget::Update(float dt)
	{
		for(MapItem *item : update_objects) {
			item->Update(dt);
		}
		scroll->Update(dt);

		updateItems();

		ParalaxCloud::SCROLL_POS = _scrollPos;
		_virtualScreen.BeginRendering(Color(0, 0, 0, 0));
		RenderFull();
		_virtualScreen.EndRendering();

		map->UpdateShine(dt);
		map->DrawShine(&_virtualScreen2, _scrollPos);
	}

	void CardWidget::Draw()
	{
		Render::device.SetBlend(false);
		_cardShader->Bind();
		{		
			//float power = (Core::mainInput.GetMousePos().x + 0.f)/MyApplication::GAME_WIDTH;
			float brightness = 0.87f; //(Core::mainInput.GetMousePos().y + 0.f)/MyApplication::GAME_HEIGHT;
			float param[4] = {0.f, 1.f, brightness, 0.f};
			_cardShader->setPSParam("params", param, 4);
		
			_virtualScreen.Bind(0);
			_virtualScreen2.Bind(1);
			FRect rect(_virtualScreen.GetBitmapRect());
			FRect uv(0,1,0,1);
			_virtualScreen.TranslateUV(rect, uv);
			Render::DrawRect(rect, uv);
		}
		_cardShader->Unbind();
		Render::device.SetBlend(true);

		map->DrawShineEffect(ParalaxCloud::SCROLL_POS);

		FPoint offset_screen(BigBackground::OFFSET, _scrollPos);
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(offset_screen);

		for(MapItem *item : draw_objects) {
			if( item->isDrawUp() ) {
				item->Draw(FPoint(0,0));
			}
		}
		Render::device.PopMatrix();
	}

	void CardWidget::RenderFull()
	{
		FPoint offset_screen(BigBackground::OFFSET, _scrollPos);

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(offset_screen);
		Render::device.SetBlend(false);
		background->Draw();
		Render::device.SetBlend(true);

		for (size_t i = 0; i < draw_objects.size(); ++i) {
			MapItem *item = draw_objects[i];
			if ( !item->isDrawUp() ) {
				item->Draw(FPoint(0,0));
			}
		}

		Render::device.PopMatrix();

#ifdef _DEBUG
		IPoint mousePos = Core::mainInput.GetMousePos();
		std::string output = Int::ToString(mousePos.x) + ", " + Int::ToString(mousePos.y - static_cast<int>(offset_screen.y));
		Render::FreeType::BindFont("debug");
		Render::PrintString(10, 130, output);
#endif
	}

	bool CardWidget::MouseDown(const IPoint &mouse_pos)
	{
		mouseDownPosition = mouse_pos;
		IPoint mouse_position(int(mouse_pos.x - BigBackground::OFFSET), int(mouse_pos.y - _scrollPos));
		bool capture = false;
		for( MapItemList::reverse_iterator itr = draw_objects.rbegin(); itr != draw_objects.rend(); ++itr )
		{
			(*itr)->MouseDown(mouse_position, capture);
		}
		_mouseDown = true;
		_tapScroll = !capture && !_editor;
		if (_tapScroll) {
			scroll->MouseDown(mouse_pos.y);
		}
		return capture;
	}

	void CardWidget::MouseMove(const IPoint &mouse_pos)	{
		IPoint mouse_position(int(mouse_pos.x - BigBackground::OFFSET), int(mouse_pos.y - _scrollPos));
		bool capture = false;
		if(!_tapScroll)
		{
			for(MapItem *item : draw_objects) {
				item->MouseMove(mouse_position, capture);
			}
		}
		if(!capture && _mouseDown && !_editor)
		{
			if(!_tapScroll)
			{
				scroll->MouseDown(mouse_pos.y);
				_tapScroll = true;
			}
			scroll->MouseMove(mouse_pos.y);
		}
		updateItems();
	}

	void CardWidget::MouseUp(const IPoint &mouse_pos)
	{
		scroll->MouseUp(mouse_pos.y);
		IPoint mouse_position(int(mouse_pos.x - BigBackground::OFFSET), int(mouse_pos.y - _scrollPos));
		if (gameInfo.IsDevMode()) // дебажное переключение уровней
		{
			if((FPoint(mouseDownPosition).GetDistanceTo(mouse_pos) < 30))
			{
				FPoint target_pos = FPoint(110.f, MyApplication::GAME_HEIGHT - 150.f);
				if(FPoint(mouse_pos).GetDistanceTo(target_pos) < 80)
				{
					map->debugDecrementHistory();
					map->initMarkers();
					map->updateUserAvatar();
					return;
				}
				target_pos = FPoint(MyApplication::GAME_WIDTH - 110.f, MyApplication::GAME_HEIGHT - 150.f);
				if(FPoint(mouse_pos).GetDistanceTo(target_pos) < 80)
				{	
					if (map->inGateway()) {
						Core::LuaDoString("gateComplete:show()");
					} else {
						// откроем буст
						int lvl = gameInfo.getLocalInt("current_level", 0) + 2;
						if (gameInfo.needBoostTutorial(lvl, 1) || gameInfo.needBoostTutorial(lvl, 2)) {
							gameInfo.shownBoostTutorials.push_back(gameInfo.currentBoostTutorial.name);
							gameInfo.currentBoostTutorial = GameInfo::BoostTutorial();
						}
						// перейдём на следующий уровень
						levelsInfo.setCurrentLevel(gameInfo.getLocalInt("extreme_level"));
						levelsInfo.setLevelScore(levelsInfo.getScoreForStar(3));
						AcceptMessage(Message("StartVisualization"));
					}
					return;
				}
			}
		}
		bool capture = math::abs(mouseDownPosition.x - mouse_pos.x) > 20 || _tapScroll;
		for(MapItem *item : draw_objects) {
			item->MouseUp(mouse_position, capture);
		}
		_tapScroll = false;
		_mouseDown = false;
	}

	void CardWidget::Load(bool debug = false)
	{
		draw_objects.clear();
		update_objects.clear();
		gameInfo.setLocalBool("NeedShowStartLevelPanel", false);

		Card::cardResourceManager.Release();

		rapidxml::file<char> file("maps/maps.xml");
#ifndef CARD_EDITOR // в CARD_EDITOR itemsSettings - статичная переменная
		rapidxml::xml_document<char> itemsSettings;
#endif
		itemsSettings.parse<rapidxml::parse_default>(file.data());

		rapidxml::xml_node<>* xml_root = itemsSettings.first_node();
		map->loadDescription(xml_root->first_node("Objects"), xml_root->first_node("shine"), debug);
		background->init(xml_root);
		map->initMarkers();
		map->updateUserAvatar();
#ifndef ENGINE_TARGET_WIN32
		if(FB::IsLoggedIn()) {
			SetFriendAvatars();
		}
#endif

		const int w = MyApplication::GAME_WIDTH;
		const int h = MyApplication::GAME_HEIGHT;
		
		draw_rect_shift   = 0;
		draw_rect         = IRect(-BigBackground::OFFSET, 0, w, h - draw_rect_shift * 2);
		update_rect_shift = -200;
		update_rect       = IRect(-BigBackground::OFFSET, 0, w, h - draw_rect_shift * 2);

		scroll = new DynamicScroller(h + 0.f, 100.f, 0.f);
		scroll->SetContentsSize(Xml::GetIntAttribute(xml_root, "height"));
		scroll->SetDeceleration(1000.f);
		scroll->SetBounceDeceleration(1000.f);
		scroll->SetMaxVelocity(3000.f);
		scroll->SetMagnetNet(1.f);
		scroll->SetMaxPosition(-100.f);

		{
			// смещение карта в текущий маркер
			UpdateLevelItem* from = map->getUpdateLevelItem(gameInfo.getLocalInt("current_marker"));
			IRect rect = from->getRect();
			float offset_next = -rect.y - rect.Height() / 2.f;
			offset_next += MyApplication::GAME_HEIGHT / 2.f;
			if(offset_next > 0) {
				offset_next = 0;
			}
			scroll->SetPosition(offset_next);
		}

		updateItems(true);
	}

	void CardWidget::AcceptMessage(const Message &message)
	{
		if (message.is("InitCard")) {
			Load();
		}
		else if(message.is("ReloadGame"))
		{
			if (map->isLoaded()) {
				map->initMarkers();
				map->updateUserAvatar();
				updateItems(true);
			}
		}
		else if (message.is("PausedScrolling"))
		{
			scroll->AcceptMessage(message);
		}
		else if(message.is("ReleaseCardResourses"))
		{
			ReleaseCardResources();
			Card::cardResourceManager.Release();
		}
		else if (message.is("SetFriendAvatars")) {
			if (map->isLoaded()) {
				SetFriendAvatars();
				updateItems(true);
			}
		} else if (message.is("FacebookLogout")) {
			//Оставляем только одну аватарку - аватарку игрока
			map->clearAvatarFriends();
			updateItems(true);
		} else if (message.is("incrementHistory")) {
			map->incrementHistory();
			open_level_visualization = true;
		} else if(message.is("StartVisualization")) {
			if (open_level_visualization) {
				open_level_visualization = false;
				int current_marker = gameInfo.getLocalInt("current_marker");
				if(gameInfo.getLocalBool("NeedShowStartLevelPanel", false))
				{
					//Проверим не нужно ли проскроллировать камеру
					UpdateLevelItem* from = map->getUpdateLevelItem(current_marker);
					IRect rect = from->getRect();
					float offset_next = -rect.y - rect.Height()/2.f;
					offset_next += MyApplication::GAME_HEIGHT/2.f;
					if(offset_next > -100.f)
					{
						offset_next = -100.f;
					}
					scroll->SetPosition(offset_next);
					updateItems();
				}
				if (gameInfo.getLocalBool("isEndHistory")) {
					// анимация открытия последнего уровня
					map->getUpdateLevelItem(current_marker)->startVisualization();
				} else {
					map->getUpdateLevelItem(current_marker - 1)->startVisualization();
				}
			} else {
				// возможно есть анимация получения звезды
				map->runStarEffects();
			}
			// проверим если нужно убрать флэш анимации
			map->removeFrozenAnimations();

			Card::cardResourceManager.Update(draw_rect.y + 0.f, draw_rect.y + draw_rect.height + 0.f, scroll->GetVelocity());
		}
		else if (message.is("StartTutorialScroll")) {
			Assert(open_level_visualization);

			int fromMarker = message.getIntegerParam();
			// ищем позицию откуда начнем показывать карту
			UpdateLevelItem* item = map->getUpdateLevelItem(fromMarker);
			IRect rect = item->getRect();
			float offset_next = -rect.y - rect.Height()/2.f;
			offset_next += MyApplication::GAME_HEIGHT/2.f;
			if(offset_next > -100.f){
				offset_next = -100.f;
			}
			scroll->SetPosition(offset_next);
			updateItems();

			// ищем начальную позицию
			int current_marker = gameInfo.getLocalInt("current_marker");
			item = map->getUpdateLevelItem(current_marker);
			rect = item->getRect();
			offset_next = -rect.y - rect.Height()/2.f;
			offset_next += MyApplication::GAME_HEIGHT/2.f;
			if(offset_next > -100.f){
				offset_next = -100.f;
			}
			Message msg("ScrollTo");
			msg.getVariables().setFloat("position", offset_next);
			msg.getVariables().setFloat("timeScale", 0.2f);
			scroll->AcceptMessage(msg);

			// сразу покажем звёзды пройденного уровня
			item = map->getUpdateLevelItem(current_marker - 1);
			item->InitMarker();
			dynamic_cast<LevelMarker*>(item)->runStarEffects();
		}
		else if (message.is("WaitForResource")) {
			Card::cardResourceManager.WaitForLoading();
		}
		else if (message.is("EndVisualization")) {
			if (message.getData() == "markerComplete") {
				map->runUserAvatar();
				// вместе со стартом полёта аватара стартуем появление текущего маркера
				map->getUpdateLevelItem(gameInfo.getLocalInt("current_marker"))->startVisualization();
			} else if (message.getData() == "userAvatar") {
				// аватар прилетел к текущему маркеру
			} else if (message.getData() == "markerOpen") {
				Core::LuaCallVoidFunction("OnCardEventFinish");
				map->runStarEffects(); // возможно, человек играл еще и в предыдущие уровни
				// второй эпизод открывается автоматически (marker == 15)
				if (gameInfo.getLocalInt("current_marker") == 15) {
					Core::LuaDoString("gateComplete:show()");
				}
			} else if (message.getData() == "ScrollTo") {
				if(open_level_visualization) {
					AcceptMessage(Message("StartVisualization"));
				}
			}
		}
		else if (message.is("OpenGateway")) {
			Core::LuaDoString("OnOpenGateway()");
			map->incrementHistory();
			open_level_visualization = true;
			AcceptMessage(Message("StartVisualization"));
		}
		else if (message.is("OpenEpisod")) {
			map->openEpisod();
		}
		else if(message.is("ScrollTo")) {
			scroll->AcceptMessage(message);
		}
		else if (message.is("InitMarkers")) {
			map->initMarkers();
			map->updateUserAvatar();
		}
		else if(message.is("KeyPress") && gameInfo.IsDevMode()) {
			int key = utils::lexical_cast<int>(message.getData());
			if(key == '+') {
				map->debugTryOpenEpisod();
				map->incrementHistory();
				map->initMarkers();
				map->updateUserAvatar();
			}else if(key == 0x0D) { //VK_RETURN
				if (map->inGateway()) {
					Core::LuaDoString("gateComplete:show()");
				} else {
					levelsInfo.setCurrentLevel(gameInfo.getLocalInt("extreme_level"));
					levelsInfo.setLevelScore(levelsInfo.getScoreForStar(3));
					AcceptMessage(Message("StartVisualization"));
				}
			} else if (key == '-') {
				map->debugDecrementHistory();
				map->initMarkers();
				map->updateUserAvatar();
			} else if (key == 'r') {
				Load(true);
			} else if (key == 's') {
				gameInfo.Save(false);
#ifdef CARD_EDITOR
				std::ofstream file("maps/maps.xml");
				file << itemsSettings;
				file.close();
#endif
			}
#ifdef ENGINE_TARGET_WIN32
			else if (key == -VK_HOME) {
				scroll->SetPosition(0.f);
			}
			else if (key == -VK_END) {
				scroll->SetPosition(scroll->GetMinPosition());
			}
			else if(key == 't') {
				Core::LuaCallVoidFunction("ReloadLua2");
			}
			else if(key == 'f') {
				FriendInfo* f = new FriendInfo();
				CardAvatarFriends* avatars = map->getAvatarFriends(gameInfo.getLocalInt("current_marker", 0));
				avatars->AddFriend(f);
			}
#endif
			else if(key == 'e') {
				_editor = !_editor;
			}
		} else {
			map->AcceptMessage(message);
		}
	}

	Message CardWidget::QueryState(const Message& message) const {
		if (message.is("inGateway")) {
			return Message(map->inGateway() ? 1 : 0);
		}
		Assert(false);
		return Message();
	}

	void CardWidget::SetFriendAvatars() {
		for (std::vector<FriendInfo*>::iterator i = gameInfo.friends.begin(), e = gameInfo.friends.end(); i != e; ++i) {
			FriendInfo* f = *i;
			CardAvatarFriends* avatars = map->getAvatarFriends(f->current_marker);
			avatars->AddFriend(f);
		}
	}

	void CardWidget::MouseWheel(int delta) {
		if(Core::mainInput.IsControlKeyDown()) {
			return;
		}
		scroll->SetPosition(scroll->GetPosition() + delta*50);
	}

	void CardWidget::ReleaseCardResources() {
		_virtualScreen.Purge();
		_virtualScreen2.Purge();
	}
};
