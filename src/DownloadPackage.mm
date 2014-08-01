#import <CommonCrypto/CommonDigest.h>
#import "DownloadPackage.h"
#include "Utils/Xml.h"
#include "Utils/Str.h"
#include "boost/filesystem/operations.hpp"
#include "Core/GlobalConstants.h"
#include "AppCommon.h"

std::string PackageUpdater::WORK_PATH;
std::string PackageUpdater::TEMP_PATH;

static const int SHORT_WAIT = 10 * 60;
static const int LONG_WAIT = 3 * 60 * 60;
static const int NOT_WAIT = 0;

const std::string GetURLPath()
{
	NSBundle* bundle = [NSBundle mainBundle];
	std::string update_url = [[bundle objectForInfoDictionaryKey:@"UpdateUrlPath"] UTF8String];
	std::string bundle_version = GetAppVersion();
	return update_url + "v" + bundle_version + "/";
}

std::string getDir(const std::string &path)
{
	std::size_t endPos = path.find_last_of("/\\");
	return (endPos == std::string::npos) ? "" : path.substr(0, endPos);
}

namespace File
{
bool CopyWithReplace(const std::string& src, const std::string& dst)
{
	FILE *fs = fopen(src.c_str(), "rb");
	if( !fs ){
		return false;
	}
	
	FILE *fd = fopen(dst.c_str(), "wb");
	if ( !fd ){
		fclose(fs);
		return false;
	}
	
	size_t bytes = 0;
	char buf[512];
	do {
		bytes = fread(buf, 1, 512, fs);
		if(bytes)
		{
			fwrite(buf, 1, bytes, fd);
		}
	} while(bytes);
	
	fclose(fs);
	fclose(fd);
	
	return true;
}

} // end of namespace

//------------------------------------------------------------------------------


@implementation DownloadPackage

+(NSString *) md5:(NSData *) input
{
	const unsigned char *cStr = (unsigned char *)[input bytes];
	int size = input.length;
	unsigned char digest[16];
	CC_MD5( cStr, size, digest ); // This is the md5 call
	
	NSMutableString *output = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
	
	for(int i = 0; i < CC_MD5_DIGEST_LENGTH; i++){
		[output appendFormat:@"%02x", digest[i]];
	}
	
	return  output;
}

+(NSString *) urlEncode: (NSString *) unencodeString
{
	NSString * encodedString = (NSString*) CFURLCreateStringByAddingPercentEscapes(NULL, (CFStringRef)unencodeString, NULL, (CFStringRef)@" ", kCFStringEncodingUTF8 );
	
	return encodedString;
}

-(void) setStartCallback:(UpdaterStartCallback) cb
{
	cbStart = cb;
}

-(void) setFinishCallback:(UpdaterFinishCallback) cb
{
	cbFinish = cb;
}

-(void) downloadThreadOnce
{
	PackageUpdater updater;

	NSAutoreleasePool* pool = [NSAutoreleasePool new];

	cbStart();
	
	bool result = false;
	updater.ClearTemp();
	updater.LoadCurrentFileList();
	if( updater.LoadNewFileList() )
	{
		updater.CreateFileListsToUpdate();
		if( updater.NeedUpdate() )
		{
			if( updater.DownloadFiles() )
			{
				result = updater.SetNewFileList();
			}
		}
	}
	else
	{
		updater.CheckCache();
	}
	
	cbFinish(result);
	
	[pool drain];
}

@end

//------------------------------------------------------------------------------
PackageUpdater::PackageUpdater()
{
}

void PackageUpdater::LoadCurrentFileList()
{
	mFileHashes.clear();
	needUpdate = false;
	
	std::string localListPath = WORK_PATH + "FilesList.xml";
	
	bool needCheckCache = false;
	if(!File::ExistsInFs(localListPath)){
		std::string dir = getDir(localListPath);
		if(!File::ExistsInFs(dir)){
			File::mkdir(dir);
		}
		
		File::CopyWithReplace("FilesList.xml", localListPath.c_str());
	}
	
	try {
		rapidxml::file<> file(localListPath.c_str());
		rapidxml::xml_document<> doc;
		doc.parse<0>(file.data());
		
		rapidxml::xml_node<>* xml_files = doc.first_node("files");
		currentBuild = Xml::GetIntAttributeOrDef(xml_files, "build", GetBuildNumber());
		
		rapidxml::xml_node<>* xml_file = xml_files->first_node("file");
		
		while(xml_file){
			std::string path = utils::String::SwapSlashes(Xml::GetStringAttribute(xml_file, "path"));
			std::string hash = Xml::GetStringAttribute(xml_file, "hash");
			mFileHashes[path] = hash;
			xml_file = xml_file->next_sibling("file");
		}
	}
	catch (rapidxml::parse_error const& e) {
		Log::log.WriteError(e.what());
		Assert(false);
	} catch (...) {
		Assert(false);
	}
}

bool PackageUpdater::LoadNewFileList()
{
	std::string hash;
	return DownloadFile("FilesList.xml", hash) && ValidateFilesList("FilesList.xml");
}

void PackageUpdater::CreateFileListsToUpdate()
{
	try {
		rapidxml::file<> file((TEMP_PATH + "FilesList.xml").c_str());
		rapidxml::xml_document<> doc;
		doc.parse<0>(file.data());
		
		size_t filesNumber = 0; // кол-во файлов в скаченном списке
		
		//какие файлы нужно обновить
		rapidxml::xml_node<>* xml_files = doc.first_node("files");

		rapidxml::xml_node<>* xml_file = xml_files->first_node("file");
		while(xml_file){
			std::string path = utils::String::SwapSlashes(Xml::GetStringAttribute(xml_file, "path"));
			std::string hash = Xml::GetStringAttribute(xml_file, "hash");
			
			bool not_exist = !File::ExistsInFs(WORK_PATH + path);
			bool no_hash = mFileHashes.find(path) == mFileHashes.end();
			bool wrong_hash = mFileHashes[path] != hash;
			
			if( not_exist || no_hash || wrong_hash){
				// файла нет, либо устарел, добавляем в список закачек
				mFileHashes[path] = hash;
				mFilesToDownload.push_back(path);
				NSLog(@"File to update: %@", [NSString stringWithCString:path.c_str() encoding:NSUTF8StringEncoding]);
			} else {
				// уже есть актуальная версия файла, копируем из текущей папки в папку, куда будем скачивать новые файлы
				CopyToTemp(path);
			}
			++filesNumber;
			
			xml_file = xml_file->next_sibling("file");
		}
		
		needUpdate = !mFilesToDownload.empty() || (filesNumber != mFileHashes.size());
		if( !needUpdate ) {
			NSLog(@"Nothing to update");
		}
	}
	catch (rapidxml::parse_error const& e) {
		Log::log.WriteError(e.what());
		Assert(false);
	} catch (...) {
		Assert(false);
	}
}

bool PackageUpdater::NeedUpdate()
{
	return needUpdate;
}

bool PackageUpdater::DownloadFiles()
{
   try {
	   while( !mFilesToDownload.empty() )
	   {
		   std::string file = (*mFilesToDownload.begin());
		   std::string hash;

		   if( !DownloadFile(file, hash) || mFileHashes[file] != hash) {
			   NSLog(@"File wrong hash: %@", [NSString stringWithCString:file.c_str() encoding:NSUTF8StringEncoding]);
			   return false;
		   }
		   mFilesToDownload.erase(mFilesToDownload.begin());
	   }
	   needUpdate = false;
   } catch (...) {	   
	   Assert(false);
	   return false;
   }
	
	return true;
}

bool PackageUpdater::DownloadFile(const std::string file, std::string &hash)
{
	std::string tmppath = TEMP_PATH + file;
	
	hash = "";
	
	std::string dir = getDir(tmppath);
	if(!File::ExistsInFs(dir)){
		File::mkdir(dir);
	}
	NSString * ns_file = [DownloadPackage urlEncode:[NSString stringWithCString:file.c_str() encoding:NSUTF8StringEncoding] ];
	NSLog(@"File: %@", ns_file);
	
 	NSURL* xmlUrl = [NSURL URLWithString:[NSString stringWithFormat:@"%@%@", [NSString stringWithCString:GetURLPath().c_str() encoding:NSUTF8StringEncoding], ns_file]];

	NSLog(@"Url: %@", [xmlUrl absoluteString]);
	NSData* data = [NSData dataWithContentsOfURL:xmlUrl];
	if (data != nil) {
		BOOL result = [data writeToFile:[NSString stringWithCString:tmppath.c_str() encoding:NSUTF8StringEncoding] atomically:YES];
		if (result)
		{
			NSString *ns_hash = [DownloadPackage md5: data ] ;
			hash = std::string([ns_hash UTF8String]);
			NSLog(@"Download success");
			return true;
		}
	}
	NSLog(@"Download failed");
	return false;
}

void PackageUpdater::CopyToTemp(const std::string file)
{
	File::mkdir(TEMP_PATH + getDir(file));
	File::CopyWithReplace(WORK_PATH + file, TEMP_PATH + file);
}

bool PackageUpdater::SetNewFileList()
{
	boost::system::error_code error;
	boost::filesystem::remove_all(WORK_PATH, error);
	if( error ) {
		Log::Error("Failed to set updated file list. Can't remove current folder, error " + utils::lexical_cast(error));
		return false;
	}
	boost::filesystem::rename(TEMP_PATH, WORK_PATH);
	return true;
}

bool PackageUpdater::ValidateFilesList(const std::string file)
{
	std::string tmppath = TEMP_PATH + file;
	
	if(!File::ExistsInFs(tmppath)){ return false; }
	
	try {
		rapidxml::file<> xmlfile(tmppath.c_str());
		rapidxml::xml_document<> doc;
		doc.parse<0>(xmlfile.data());
		
		if(file == "FilesList.xml"){
			rapidxml::xml_node<>* xml_files = doc.first_node("files");
			rapidxml::xml_node<>* xml_file = xml_files->first_node("file");
			while(xml_file){
				if(Xml::GetStringAttributeOrDef(xml_file, "hash", "") == "") return false;
				if(Xml::GetStringAttributeOrDef(xml_file, "path", "") == "") return false;
				xml_file = xml_file->next_sibling("file");
			}
		}

	}
	catch (rapidxml::parse_error const& e) {
		Log::log.WriteError(e.what());
		return false;
	}
	catch (...) {
		return false;
	}
	
	return true;
}

void PackageUpdater::CheckCache()
{
	if( currentBuild < GetBuildNumber() )
	{
		boost::system::error_code error;
		boost::filesystem::remove_all(WORK_PATH, error);
		if( error )
			Log::Error("Can't remove outdated cache folder, error " + utils::lexical_cast(error));
	}
}

void PackageUpdater::ClearTemp()
{
	boost::system::error_code error;
	boost::filesystem::remove_all(TEMP_PATH, error);
	if( error )
		Log::Error("Can't remove temp folder, error " + utils::lexical_cast(error));
}

