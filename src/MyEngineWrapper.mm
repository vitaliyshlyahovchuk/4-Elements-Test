//
//  Created by Slava on 17.11.10.
//  Copyright 2010 Playrix Entertainment. All rights reserved.
//

#import "MyEngineWrapper.h"
#include "PlayrixEngine.h"

#import <CommonCrypto/CommonDigest.h> // for MD5

#include <AdSupport/ASIdentifierManager.h>

// For hw-uuid
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>

@implementation EngineWrapper

+ (NSString*) getText:(const char*)idResource
{
	Render::Text* result = Core::resourceManager.Find<Render::Text>(idResource);
	if (result) {
		return [NSString stringWithUTF8String:result->ToString().c_str()];
	}
	return [NSString stringWithFormat:@"[%@]", [NSString stringWithUTF8String: idResource]];
}

+ (NSString*) macaddress
{
	int mib[6];
	
	mib[0] = CTL_NET;
	mib[1] = AF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_LINK;
	mib[4] = NET_RT_IFLIST;
	
	if ((mib[5] = if_nametoindex("en0")) == 0) {
		printf("Error: if_nametoindex error\n");
		return NULL;
	}
	
	size_t len;
	if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
		printf("Error: sysctl, take 1\n");
		return NULL;
	}
	
	char * buf = (char *)malloc(len);
	if (buf == NULL) {
		printf("Could not allocate memory. error!\n");
		return NULL;
	}
	
	if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
		printf("Error: sysctl, take 2");
		free(buf);
		return NULL;
	}
	
	struct if_msghdr * ifm = (struct if_msghdr *)buf;
	struct sockaddr_dl * sdl = (struct sockaddr_dl *)(ifm + 1);
	unsigned char * ptr = (unsigned char *)LLADDR(sdl);
	NSString *outstring = [NSString stringWithFormat:@"%02X:%02X:%02X:%02X:%02X:%02X",
						   *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5)];
	free(buf);
	
	return outstring;
}

+ (NSString*) MD5FromString:(NSString*)str
{
	if (!str || [str length] == 0) {
		return nil;
	}
	
	unsigned char outputBuffer[CC_MD5_DIGEST_LENGTH];
	CC_MD5([str UTF8String], [str length], outputBuffer);
	
	NSMutableString *outputString = [[NSMutableString alloc] initWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
	for (NSInteger count = 0; count < CC_MD5_DIGEST_LENGTH; count++){
		[outputString appendFormat:@"%02x", outputBuffer[count]];
	}
	
	return [outputString autorelease];
}

+ (NSString*) UniqueDeviceIdentifier
{
	static NSString * cached = nil;
	if (cached) { return cached; }
	
	NSString * uniqueIdentifier = nil;
	
	// В iOS 7 вместо mac-адреса возвращается ерунда. В этом случае будем использовать новый advertisingIdentifier, который появился только в iOS 6.
	// В iOS < 7 будем всё-таки использовать mac-адрес, пока возможно.
	if ([[[UIDevice currentDevice] systemVersion] compare:@"7.0" options:NSNumericSearch] == NSOrderedAscending // ios < 7
		|| !NSClassFromString(@"ASIdentifierManager")
		) {
		// Используем хэш от мак-адреса.
		NSString * macaddress = [EngineWrapper macaddress];
		NSString * salt = @"Township";
		
		NSString * stringToHash = [NSString stringWithFormat:@"%@%@",macaddress, salt];
		uniqueIdentifier = [EngineWrapper MD5FromString:stringToHash];
	} else {
		// Используем advertisingIdentifier.
		NSUUID * advId = [[ASIdentifierManager sharedManager] advertisingIdentifier];
		if (advId != nil) {
			uniqueIdentifier = [advId UUIDString];
		} else {
			Assert(false);
			return @"";
		}
	}
	
	cached = uniqueIdentifier;
	[cached retain];
	
	return cached;
}

@end

//----------------------------------------------------------------------

std::string DeviceDescription()
{
	size_t size;
	sysctlbyname("hw.machine", NULL, &size, NULL, 0);
	char *machine = (char*)malloc(size);
	sysctlbyname("hw.machine", machine, &size, NULL, 0);
	NSString *platform = [NSString stringWithUTF8String:machine];
	free(machine);
	
	if ([platform isEqualToString:@"iPhone1,1"])    return "iPhone 2G";
	if ([platform isEqualToString:@"iPhone1,2"])    return "iPhone 3G";
	if ([platform isEqualToString:@"iPhone2,1"])    return "iPhone 3GS";
	if ([platform isEqualToString:@"iPhone3,1"])    return "iPhone 4";
	if ([platform isEqualToString:@"iPhone3,2"])    return "iPhone 4";
	if ([platform isEqualToString:@"iPhone3,3"])    return "iPhone 4 (CDMA)";
	if ([platform isEqualToString:@"iPhone4,1"])    return "iPhone 4S";
	if ([platform isEqualToString:@"iPhone5,1"])    return "iPhone 5";
	if ([platform isEqualToString:@"iPhone5,2"])    return "iPhone 5 (GSM+CDMA)";
	
	if ([platform isEqualToString:@"iPod1,1"])      return "iPod Touch (1 Gen)";
	if ([platform isEqualToString:@"iPod2,1"])      return "iPod Touch (2 Gen)";
	if ([platform isEqualToString:@"iPod3,1"])      return "iPod Touch (3 Gen)";
	if ([platform isEqualToString:@"iPod4,1"])      return "iPod Touch (4 Gen)";
	if ([platform isEqualToString:@"iPod5,1"])      return "iPod Touch (5 Gen)";
	
	if ([platform isEqualToString:@"iPad1,1"])      return "iPad";
	if ([platform isEqualToString:@"iPad1,2"])      return "iPad 3G";
	if ([platform isEqualToString:@"iPad2,1"])      return "iPad 2 (WiFi)";
	if ([platform isEqualToString:@"iPad2,2"])      return "iPad 2";
	if ([platform isEqualToString:@"iPad2,3"])      return "iPad 2 (CDMA)";
	if ([platform isEqualToString:@"iPad2,4"])      return "iPad 2";
	if ([platform isEqualToString:@"iPad2,5"])      return "iPad Mini (WiFi)";
	if ([platform isEqualToString:@"iPad2,6"])      return "iPad Mini";
	if ([platform isEqualToString:@"iPad2,7"])      return "iPad Mini (GSM+CDMA)";
	if ([platform isEqualToString:@"iPad3,1"])      return "iPad 3 (WiFi)";
	if ([platform isEqualToString:@"iPad3,2"])      return "iPad 3 (GSM+CDMA)";
	if ([platform isEqualToString:@"iPad3,3"])      return "iPad 3";
	if ([platform isEqualToString:@"iPad3,4"])      return "iPad 4 (WiFi)";
	if ([platform isEqualToString:@"iPad3,5"])      return "iPad 4";
	if ([platform isEqualToString:@"iPad3,6"])      return "iPad 4 (GSM+CDMA)";
	
	if ([platform isEqualToString:@"i386"])         return "Simulator";
	if ([platform isEqualToString:@"x86_64"])       return "Simulator";
	
	return [platform UTF8String];
}

std::string GetUniqueDeviceIdentifier()
{
	return [[EngineWrapper UniqueDeviceIdentifier] UTF8String];
}

std::string GetDeviceIdName()
{
	return [[NSString stringWithFormat:@"%@ %@ %@",
			 [[UIDevice currentDevice] name],
			 [[UIDevice currentDevice] systemName],
			 [[UIDevice currentDevice] systemVersion]] UTF8String];
}


//----------------------------------------------------------------------

long long int GetTimestamp()
{
	return (long long int)[[NSDate date] timeIntervalSince1970];
}

void SaveTimestamp(const std::string& name, long long int timestamp)
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	@synchronized(defaults) {
		[defaults setObject:[NSNumber numberWithUnsignedLongLong:timestamp] forKey:[NSString stringWithUTF8String:name.c_str()]];
		[defaults synchronize];
	}
}

long long int LoadTimestamp(const std::string& name)
{
	long long int res = 0;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	@synchronized(defaults) {
		id res_ns = [defaults objectForKey:[NSString stringWithUTF8String:name.c_str()]];
		if (res_ns != nil && [res_ns isKindOfClass:[NSNumber class]]) {
			res = (long long int)[res_ns unsignedLongLongValue];
		}
	}
	return res;
}

bool CheckTimeInterval(const std::string& name, long long int interval)
{
	long long int lastShow_t = LoadTimestamp(name);
	long long int now = GetTimestamp();
	if (lastShow_t < now && now - lastShow_t < interval) {
		NSLogD(@"CheckTimeInterval: %s. Wait for %lld seconds more.", name.c_str(), lastShow_t + interval - now);
		return false;
	}
	
	return true;
}

bool PassTimeInterval(const std::string& name, long long int interval)
{
	bool res = CheckTimeInterval(name, interval);
	if (res) {
		SaveTimestamp(name, GetTimestamp());
	}
	return res;
}

std::string UrlEsc(const std::string& s)
{
	NSString * esc_title = [NSString stringWithUTF8String:s.c_str()];
	esc_title = [esc_title stringByReplacingOccurrencesOfString:@"|" withString:@" "];
	esc_title = (NSString*)CFURLCreateStringByAddingPercentEscapes(NULL, (CFStringRef)esc_title, NULL, CFSTR("!*'();:@&=+$,/?%#[]"), kCFStringEncodingUTF8);
	return [esc_title UTF8String];
}

static bool jailbroken = false;

void CheckJailbreak()
{
#if TARGET_IPHONE_SIMULATOR
	return; // На эмуляторе эти проверки не имеют смысла.
	#endif
	
	//Presence of any of the following file paths indicates a jail-broken device:
	NSArray * check_paths = [NSArray arrayWithObjects:
							 @"/bin/bash",
							 @"/Applications/Cydia.app",
							 @"/Applications/blackra1n.app",
							 @"/Applications/FakeCarrier.app",
							 @"/Applications/Icy.app",
							 @"/Applications/IntelliScreen.app",
							 @"/Applications/MxTube.app",
							 @"/Applications/RockApp.app",
							 @"/Applications/SBSettings.app",
							 @"/Applications/WinterBoard.app",
							 @"/Library/MobileSubstrate/DynamicLibraries/LiveClock.plist",
							 @"/Library/MobileSubstrate/DynamicLibraries/Veency.plist",
							 @"/private/var/lib/apt",
							 @"/private/var/lib/cydia",
							 @"/private/var/mobile/Library/SBSettings/Themes",
							 @"/private/var/stash",
							 @"/private/var/tmp/cydia.log",
							 @"/System/Library/LaunchDaemons/com.ikey.bbot.plist",
							 @"/System/Library/LaunchDaemons/com.saurik.Cydia.Startup.plist",
							 @"/usr/bin/sshd",
							 @"/usr/libexec/sftp-server",
							 @"/usr/sbin/sshd",
							 nil];
	for (int i = 0; i < [check_paths count]; ++i) {
		std::string path = std::string([[check_paths objectAtIndex:i] UTF8String]);
		if (File::ExistsInFs(path)){
			jailbroken = true;
			return;
		}
	}
}

bool isJailbroken()
{
	return jailbroken;
}