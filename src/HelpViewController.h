//
//  Copyright 2013 Playrix Entertainment. All rights reserved.
//

@class AppDelegate;

@interface HelpViewController : UIViewController<UIWebViewDelegate> {
@private
	AppDelegate * _appDelegate;
	UIWebView *_webView;	
}
- (void) open;
- (void) createView;
- (id) initWithAppDelegate:(AppDelegate*)appDelegate;
@end
