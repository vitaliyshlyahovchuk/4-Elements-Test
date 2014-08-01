#include "stdafx.h"
#include "FriendInfo.h"
#include "FlashUserAvatar.h"

UserAvatar::UserAvatar():texture(NULL) {
	SetTexture(NULL);
}

void UserAvatar::SetTexture(Render::Texture* texture) {
	if (texture == NULL) {
		texture = Core::resourceManager.Get<Render::Texture>("DefaultUserPic");
	}
	if (this->texture != texture) {
		this->texture = texture;
		IRect rect = texture->getBitmapRect();
		texturePosition.x = -0.5f * rect.width;
		texturePosition.y = -0.5f * rect.height;
		size.x = static_cast<float>(rect.width);
		size.y = static_cast<float>(rect.height);
	}
}

void UserAvatar::Draw(const FPoint& position) {
	texture->Draw(position + texturePosition);
}

float UserAvatar::GetWidth() const {
	return size.x;
}

float UserAvatar::GetHeight() const {
	return size.y;
}

FriendInfo::FriendInfo()
	: current_marker(1)
    , extreme_level(0)
	, avatar(new UserAvatar())
	, name("&@Test@")
{
}

FriendInfo::~FriendInfo() {
	delete avatar;
	avatar = NULL;
}

IFlashDisplayObject* FriendInfo::getFlashAvatar() {
	return new FlashUserAvatar(avatar);
}

Render::Texture* UserAvatar::GetTexture() {
	return texture;
}
