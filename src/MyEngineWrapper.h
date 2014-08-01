//
//  EngineWrapper.h
//  80days
//
//  Created by Slava on 17.11.10.
//  Copyright 2010 Playrix Entertainment. All rights reserved.
//

#ifdef __OBJC__

@interface EngineWrapper : NSObject {

}

// Возвращает текст (Core::resourceManager.GetText) в виде NSString.
+ (NSString*)getText:(const char*)idResource;

// Возвращает md5-hash в виде строки, состоящей из символов 0-9a-f.
+ (NSString*) MD5FromString:(NSString*)str;

// Возвращает уникальный идентификатор дейвайса (хэш от мак-адреса с солью).
+ (NSString*) UniqueDeviceIdentifier;

@end

// Для отключения NSLog в продакшене:
#ifndef PRODUCTION
#define NSLogD NSLog
#else
#define NSLogD(...)
#endif

#endif // __OBJC__

#ifdef __cplusplus

std::string DeviceDescription();

/// уникальный идентификатор устройства
std::string GetUniqueDeviceIdentifier();

/// короткое имя устройства + название системы и версия
std::string GetDeviceIdName();

// Функции для работы с timestamp'ами и интервалами.

/// Возвращает unix timestamp (количество СЕКУНД с 1 января 1970).
long long int GetTimestamp();

/// Сохраняет значение в UserDefaults.
void SaveTimestamp(const std::string& name, long long int timestamp);

/// Возвращает значение из UserDefaults, либо 0.
long long int LoadTimestamp(const std::string& name);

/// Возвращает true, если с сохранённого (как 'name') момента времени прошло >= interval секунд.
bool CheckTimeInterval(const std::string& name, long long int interval);

/// То же, что CheckTimeInterval, плюс, если возвращает true, то записывет новое (текущее) значение времени.
bool PassTimeInterval(const std::string& name, long long int interval);

std::string UrlEsc(const std::string& s);

void CheckJailbreak();
bool isJailbroken();

#endif