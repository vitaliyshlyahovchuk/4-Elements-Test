#ifndef _INTERNERIMAGEDOWNLOADER_H
#define _INTERNERIMAGEDOWNLOADER_H

#include "http_downloader.h"

class UserAvatar;

class InternetImageDownLoader
	: public RefCounter
{
public:

	InternetImageDownLoader();
	void LoadFromInternet(UserAvatar *avatar, const std::string &url, const std::string &saveId);
	void LoadFromCache(UserAvatar *avatar, const std::string &saveId);

private:

	HttpDownloader loader;
};

#endif // _INTERNERIMAGEDOWNLOADER_H
