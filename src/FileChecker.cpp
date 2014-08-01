#include "stdafx.h"
#include "types.h"
#include "FileChecker.h"
#include "GameInfo.h"

#import "DownloadPackage.h"

#ifdef ENGINE_TARGET_IPHONE
#import <GameKit/GameKit.h>
#endif

void FileChecker::check() {
	
	tHashMap hashMap;
	// загрузка проверяемых файлов+хешей
	Xml::RapidXmlDocument doc("FilesListDefault.xml");
	rapidxml::xml_node<>* xml_file_node = doc.first_node()->first_node();
	while(xml_file_node){
		hashMap[Xml::GetStringAttribute(xml_file_node, "path")] = Xml::GetStringAttribute(xml_file_node, "hash");
		xml_file_node = xml_file_node->next_sibling();
	}
	// загрузка возможно обновленных файлов+хешей
	if (File::Exists("FilesList.xml")) {
		Xml::RapidXmlDocument doc("FilesList.xml");
		rapidxml::xml_node<>* xml_file_node = doc.first_node()->first_node();
		while(xml_file_node){
			std::string path = Xml::GetStringAttribute(xml_file_node, "path");
			if (hashMap.find(path) != hashMap.end()) {
				// обновим
				hashMap[path] = Xml::GetStringAttribute(xml_file_node, "hash");
			}
			xml_file_node = xml_file_node->next_sibling();
		}
	}
	
	// TODO проверка хешей
#ifdef ENGINE_TARGET_IPHONE
	std::vector<uint8_t> data;
	data.reserve(8 * 1024);
	for (tHashMapIterator i = hashMap.begin(), e = hashMap.end(); i != e; ++i) {
		data.clear();
		File::LoadFile(i->first, data);
		NSData* ns_data = [NSData dataWithBytes:&data[0] length:data.size()];
		std::string hash = [[DownloadPackage md5:ns_data] UTF8String];
		if (hash != i->second) {
			gameInfo.setPerhapsCheater();
			break;
		}
	}
#endif
}
