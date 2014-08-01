#include "stdafx.h"
#include "AvatarContainer.h"
#include "UpdateLevelItem.h"
#include "FriendInfo.h"
#include "GameInfo.h"

namespace Card
{	
	//---------------------------------------------------------------------------------------------
	class FriendAvatar {
	public:
		FPoint position;
		UserAvatar* avatar;

		FriendAvatar()
			:frame(Core::resourceManager.Get<Render::Texture>("LevelMarkerFriendFrame"))
			,avatar(NULL)
			,avatar_offset(27.5f, 29.f)
		{
		}

		void Draw(const FPoint& shift) {
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(shift + position + avatar_offset);
			Render::device.MatrixScale(0.88f);
			avatar->Draw(FPoint(0.f, 0.f));
			Render::device.PopMatrix();
			frame->Draw(shift + position);
		}

		IRect getRect() const {
			return frame->getBitmapRect().Inflated(20).MovedBy(position.Rounded());
		}

	private:
		FPoint avatar_offset;
		Render::Texture* frame;
	};
	//---------------------------------------------------------------------------------------------
	
	FPoint CardAvatarFriends::marker_offset(44.f, -30.f);

	CardAvatarFriends::CardAvatarFriends()
		:MapItem()
		,openAvatars(false)
		,openAvatarTimer(0.f)
		,openAvatarTime(0.f)
		,stateIn(true)
	{
		iconsAmount = 0;
		icons[0] = icons[1] = icons[2] = NULL;

		// сплайны смещения при открытии/закрытии
		openOffsetPath[0].Clear();
		openOffsetPath[0].addKey(FPoint(0, 0));
		openOffsetPath[0].addKey(FPoint(8, 46));
		openOffsetPath[0].addKey(FPoint(6, 44));
		openOffsetPath[0].CalculateGradient(false);
		openOffsetPath[1].Clear();
		openOffsetPath[1].addKey(FPoint(0, 0));
		openOffsetPath[1].addKey(FPoint(2, 0));
		openOffsetPath[1].addKey(FPoint(0, 0));
		openOffsetPath[1].CalculateGradient(false);
		openOffsetPath[2].Clear();
		openOffsetPath[2].addKey(FPoint(0, 0));
		openOffsetPath[2].addKey(FPoint(-7, -46));
		openOffsetPath[2].addKey(FPoint(-6, -44));
		openOffsetPath[2].CalculateGradient(false);

		openOffset[0] = openOffset[1] = openOffset[2] = FPoint(0.f, 0.f);
	}

	bool CardAvatarFriends::needDraw(const IRect &draw_rect) const {
		if (iconsAmount > 0) {
			return MapItem::needDraw(draw_rect);
		}
		return false;
	}

	void CardAvatarFriends::Draw(const FPoint& shift) {
		for (int i = iconsAmount; i > 0; --i) {
			icons[i-1]->Draw(shift + position + openOffset[i-1]);
		}
	}

	bool CardAvatarFriends::isDrawUp() const {
		return true;
	}

	void CardAvatarFriends::MouseDown(const FPoint& mouse_position, bool &capture) {
		if (!capture && rect.Contains(mouse_position.Rounded())) {
			pressed = true;
			mouse_pressed = mouse_position;
		}
	}

	void CardAvatarFriends::MouseMove(const FPoint& mouse_position, bool &capture) {
		if (pressed) {
			float dx = mouse_position.x - mouse_pressed.x;
			float dy = mouse_position.y - mouse_pressed.y;
			if (dx * dx + dy * dy > 6.f * 6.f) { // 6 пикселей чувствительность клика
				pressed = false;
			} else {
				capture = true;
			}
		}
	}

	void CardAvatarFriends::MouseUp(const FPoint& mouse_position, bool& capture) {
		if (pressed && !openAvatars && iconsAmount > 1 && rect.Contains(mouse_position.Rounded())) {
			openAvatars = true;
			openAvatarTimer = 0.f;
			openAvatarTime = 0.8f;
		} else if (!stateIn && !openAvatars && iconsAmount > 1) {
			openAvatars = true;
			openAvatarTimer = 0.f;
			openAvatarTime = 0.5f;
		}
	}

	void CardAvatarFriends::Update(float dt) {
		if (openAvatars) {
			openAvatarTimer += dt;
			if (openAvatarTimer >= openAvatarTime) {
				openAvatars = false;
				stateIn = !stateIn;
			} else {
				for (int i = iconsAmount; i > 0; --i) {
					float t = math::ease(openAvatarTimer / openAvatarTime, 0.3f, 0.4f);
					if (!stateIn) t = 1 - t;
					openOffset[i-1] = openOffsetPath[i-1].getGlobalFrame(t);
				}
			}
		}
	}

	void CardAvatarFriends::AddFriend(FriendInfo* info) {
		for (int i = 0; i < 3; ++i) {
			if (icons[i] == NULL) {
				icons[i] = new FriendAvatar();
			}
			if (icons[i]->avatar == NULL) {
				icons[i]->avatar = info->avatar;
				++iconsAmount;
				correctIconsPosition();
				return;
			}
		}
	}

	void CardAvatarFriends::correctIconsPosition() {
		for (int i = 0; i < iconsAmount; ++i) {
			icons[i]->position = FPoint(6.f, -5.f) * static_cast<float>(i);
			if (i > 0) {
				rect = icons[i]->getRect();
			} else {
				rect = MapItem::unionRect(rect, icons[i]->getRect());
			}
		}
		rect.MoveTo(position.Rounded());
	}

	void CardAvatarFriends::ClearFriends() {
		for (int i = 0; i < 3; ++i) {
			if (icons[i]) {
				icons[i]->avatar = NULL;
			}
		}
		iconsAmount = 0;
	}
	
	void CardAvatarFriends::attachToMarker(UpdateLevelItem* marker) {
		setPosition(marker->getPosition() + marker->getCenterPosition() + marker_offset);
	}

	//---------------------------------------------------------------------------------------------

	CardUserAvatar::CardUserAvatar(UserAvatar *avatar)
		: MapItem()
		, frame(Core::resourceManager.Get<Render::Texture>("LevelMarkerUserFrame"))
		, avatar(avatar)
		, avatar_offset(47.f, 50.f)
		, titleBossText(Core::resourceManager.Get<Render::Text>("AvatarTitleText"))
		, text_offset(47.f, 110.f)
		, marker_offset(-145.f, -54.f)
		, moveTime(0.f)
		, moveTimer(0.f)
		, move(false)
	{
		rect = frame->getBitmapRect().MovedBy(avatar_offset.Rounded()).Inflated(20);
	}

	void CardUserAvatar::Draw(const FPoint& shift) {
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(shift + position + avatar_offset);
		Render::device.MatrixScale(1.16f);
		avatar->Draw(FPoint(0.f, 0.f));
		Render::device.PopMatrix();
		frame->Draw(shift + position);
		titleBossText->Draw(shift + position + text_offset);
	}


	void CardUserAvatar::attachToMarker(UpdateLevelItem* marker) {
		currentMarker = marker;
		setPosition(marker->getPosition() + marker->getCenterPosition() + marker_offset);
	}

	void CardUserAvatar::goToMarker(UpdateLevelItem* marker) {
		if (currentMarker == marker) {
			return;
		}

		MM::manager.PlaySample("MapMarkerFly");

		currentMarker = marker;

		path.Clear();
		path.addKey(position);
		path.addKey(marker->getPosition() + marker->getCenterPosition() + marker_offset);
		path.CalculateGradient(false);

		moveTime = 1.0f;		// Аватар должен прилететь примерно в одно время с окончанием эффекта LevelMarkerOpen
		moveTimer = 0.f;
		move = true;
	}

	void CardUserAvatar::Update(float dt) {
		if(move) {
			moveTimer += dt;
			if (moveTimer > moveTime) {
				moveTimer = moveTime;
				move = false;
				Core::guiManager.getLayer("CardLayer")->getWidget("CardWidget")->AcceptMessage(Message("EndVisualization", "userAvatar"));
			}
			float t = math::ease(moveTimer / moveTime, 0.3f, 0.3f);
			setPosition(path.getGlobalFrame(t));
		}
	}

	bool CardUserAvatar::isDrawUp() const {
		return true;
	}
}//namespace Card