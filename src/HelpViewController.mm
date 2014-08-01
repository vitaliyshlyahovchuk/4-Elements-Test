//
//  Copyright 2013 Playrix Entertainment. All rights reserved.
//

#import "AppDelegate.h"
#import "HelpViewController.h"
#include <Core/Locale.h>
#import <sys/utsname.h>
#import <SystemConfiguration/SystemConfiguration.h>

@implementation HelpViewController

-(id)initWithAppDelegate:(AppDelegate*)appDelegate {
	if (self = [super initWithNibName:nil bundle:nil]) {
		_appDelegate = appDelegate;		
	}
	return self;
}

-(void)createView {
	Assert(_appDelegate != nil);
	UIView* rootView;
	CGRect webFrame;
	UIImageView* headerView;
	float scale = DeviceIpad() ? 1.0 : 0.5;
	float width = MyApplication::GAME_WIDTH * scale;
	float height = MyApplication::GAME_HEIGHT * scale;
	UIImage* header = [UIImage imageWithContentsOfFile:@"../base/help/header.png"];
	headerView = [[UIImageView alloc] initWithImage:header];
	float headerScale = width / header.size.width;
	headerView.frame = CGRectMake(0, 0, width, header.size.height * headerScale);
	rootView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, height)];
	webFrame = CGRectMake(0, header.size.height * scale - 65 * headerScale, width, height - header.size.height * scale + 65 * headerScale);
	_webView = [[UIWebView alloc] initWithFrame:webFrame];
	_webView.backgroundColor = [UIColor colorWithWhite:1.0 alpha:1.0];
	
	// Этот хак скрывает тени, отрисовываемые сверху или снизу при "перескролле" _webView
	// см. http://stackoverflow.com/questions/1074320/remove-uiwebview-shadow
	for (UIView *wview in [[[_webView subviews] objectAtIndex:0] subviews]) {
		if ([wview isKindOfClass:[UIImageView class]]) {
			wview.hidden = YES;
		}
	}
	
	UIImage* backImage = [UIImage imageWithContentsOfFile:@"../base/help/back.png"];
	int backX = (MyApplication::GAME_WIDTH - backImage.size.width - 10) * scale;
	int backY = 10 * scale;
	UIButton *back = [UIButton buttonWithType:UIButtonTypeCustom];
	[back setImage:backImage forState:UIControlStateNormal];
	back.frame = CGRectMake(backX , backY, backImage.size.width * scale, backImage.size.height * scale);
	[back addTarget:_appDelegate action:@selector(hideHelp) forControlEvents:UIControlEventTouchUpInside];
	
	[rootView addSubview:_webView];
	[rootView addSubview:headerView];
	[rootView addSubview:back];
	[self setView:rootView];
	
	[headerView release];
	[_webView release];
	[rootView release];
}

-(void)open
{
	std::string lang = Core::locale.GetLanguage();
	NSString* pathForResource = [NSString stringWithFormat:@"help-%s", lang.c_str()];
	NSString* indexPath = [[NSBundle mainBundle] pathForResource:pathForResource ofType:@"html" inDirectory:@"./base/help/"];
	if (indexPath == nil) {
		Assert(false);
		indexPath = [[NSBundle mainBundle] pathForResource:@"help-en" ofType:@"html" inDirectory:@"./base/help/"];
	}
	NSURL *url = [NSURL fileURLWithPath:indexPath];
	NSURLRequest *request = [NSURLRequest requestWithURL:url cachePolicy:NSURLRequestUseProtocolCachePolicy timeoutInterval:30];
	[_webView loadRequest:request];
}

-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	return UIInterfaceOrientationIsPortrait(interfaceOrientation);
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
-(NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
}

- (BOOL) shouldAutorotate
{
    return YES;
}
#endif

// To hide status bar in iOS 7.
- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (void)dealloc {
	[_webView setDelegate:nil];
	[_webView release];
	[super dealloc];
}

@end
