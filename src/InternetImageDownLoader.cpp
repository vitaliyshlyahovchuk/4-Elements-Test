#include "stdafx.h"
#include "InternetImageDownLoader.h"
#include "Core/MessageManager.h"
#include "FriendInfo.h"
#include "MyEngineWrapper.h"
#include "ImageLoader.h"
#include "MyApplication.h"
#include "Core/TextureLoader.h"

#ifdef ENGINE_TARGET_IPHONE
	#import <UIKit/UIKit.h>
#else
	#include "jni_support.h"
	using namespace Android;
#endif

static std::string GetCachedFile(const std::string &saveId)
{
#ifdef ENGINE_TARGET_IPHONE
	NSString *saveID = [NSString stringWithUTF8String:saveId.c_str()];
	NSString *savedFile = nil;
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *files = [defaults dictionaryForKey:@"friend_foto"];
	if (files) {
		savedFile = [files objectForKey:saveID];
	}
	std::string path;
	if (savedFile && [savedFile UTF8String]) {
		path = [savedFile UTF8String];
	}
	return path;
#else
	// TODO android
	return "";
#endif
}

static void AddToCache(const std::string &saveId, const std::string &path)
{
#ifdef ENGINE_TARGET_IPHONE
	NSString *nsPath = [NSString stringWithUTF8String:path.c_str()];
	NSString *saveID = [NSString stringWithUTF8String:saveId.c_str()];
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *files = [defaults dictionaryForKey:@"friend_foto"];
	NSMutableDictionary *nd = nil;
	if (files) {
		nd = [NSMutableDictionary dictionaryWithDictionary:files];
	}
	else {
		nd = [NSMutableDictionary dictionaryWithCapacity:1];
	}
	[nd setObject:nsPath forKey:saveID];
	[defaults setObject:nd forKey:@"friend_foto"];
	[defaults synchronize];
#else
	// TODO android
#endif
}

static void LoadAvatarFromFile(UserAvatar *avatar, const std::string &path)
{
	std::string file = path, file_a;
	Render::Texture *pic = NULL;
	if (Render::ImageLoader::SelectImageType(file, file_a) != Render::IT_NONE) {
		pic = new Render::Texture(path);
		if (pic && pic->Empty()) {
			pic->SetLoader(new TextureLoader());
			pic->BeginUse(ResourceLoadMode::Sync);
			avatar->SetTexture(pic);
		}
	}
}

class UploadTexture
{
public:

	UploadTexture(UserAvatar* avatar, const std::vector<uint8_t> &data_)
		: avatar(avatar)
		, data(data_)
	{}

	void operator()() {
		Render::Texture *pic = new Render::Texture();
		if (pic && pic->Empty()) {
			pic->SetLoader(new TextureLoader());
			pic->BeginUse(ResourceLoadMode::Sync);
			avatar->SetTexture(pic);
		}
	}

private:

	UserAvatar *avatar;

	std::vector<uint8_t> data;

};

InternetImageDownLoader::InternetImageDownLoader()
{
	loader.SetTimeout(30);
	loader.UseCache(false);
}

void InternetImageDownLoader::LoadFromInternet(UserAvatar *avatar, const std::string &url, const std::string &saveId)
{
	std::string cachedFile = GetCachedFile(saveId);
	if (cachedFile.empty()) {
#ifdef ENGINE_TARGET_IPHONE
		auto nativePreprocess = [avatar, saveId](NSData *data) -> NSData* {
			UIImage *image = [UIImage imageWithData:data];
			if (!image) {
				NSLogD(@"downLoadImages: Can't read image.");
				return nil;
			}
			NSData *pngdata = UIImagePNGRepresentation(image);
			std::string path = File::GetSpecialFolderPath(SpecialFolder::LocalCaches) + "/" + saveId + ".png";
			BOOL result = [pngdata writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES];
			if (result) {
				Core::appInstance->RunInMainThread([avatar, saveId, path]() {
					AddToCache(saveId, path);
					LoadAvatarFromFile(avatar, path);
				});
			}
			else {
				Halt("downLoadImages: Can't write image to disk");
			}
			// данные дальше нам не нужны
			return nil;
		};
#else
		auto nativePreprocess = [avatar, saveId](JNIEnv *env, jobject data) -> jobject {
			JniClass clazz(env, "com/playrix/fourelementsfreemium/CommonUtils");
			std::string path = "Caches/" + saveId + ".png";
			JniString jstr(env, path);
			jmethodID method = env->GetStaticMethodID(clazz, "convertImgAndSave", "(Ljava/io/InputStream;Ljava/lang/String;)Z");
			bool result = env->CallStaticBooleanMethod(clazz, method, data, jstr.get());
			if (result) {
				std::vector<std::pair<std::string, IO::FileInfo::Type> > searchPaths;
				Core::fileSystem.GenIndexMap(searchPaths);
				Core::appInstance->RunInMainThread([avatar, saveId, path]() {
					AddToCache(saveId, path);
					LoadAvatarFromFile(avatar, path);
				});
			}
			else {
				Halt("downLoadImages: Can't write image to disk");
			}
			// данные дальше нам не нужны
			return NULL;
		};
#endif
		loader.DownloadFileWithNativePreprocess(url, [url](int status, const std::vector<uint8_t> &/*data*/) {
			if (status != 200) {
				Log::Info("Image download error: http-status=" + utils::lexical_cast(status) + ", url=" + url);
			}
		}, nativePreprocess);
	}
	else {
		LoadAvatarFromFile(avatar, cachedFile);
	}
}

void InternetImageDownLoader::LoadFromCache(UserAvatar *avatar, const std::string &saveId)
{
	std::string cachedFile = GetCachedFile(saveId);
	if (cachedFile.empty()) {
		return;
	}
	LoadAvatarFromFile(avatar, cachedFile);
}
