#import <UIKit/UIKit.h>
#import "MainViewController.h"
#import "MyApplication.h"
#import <HockeySDK/HockeySDK.h>

@class EAGLView;

@interface AppDelegate : NSObject <UIApplicationDelegate, BITHockeyManagerDelegate> {
    UIWindow *window;
    EAGLView *glView;
    MainViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView *glView;
@property (nonatomic, retain) IBOutlet MainViewController *viewController;
//@property (nonatomic, retain) IBOutlet UINavigationController *navigationController;

- (void) update;
- (void) tryDisableDisplayLinks;
- (void) tryEnableDisplayLinks;
+ (AppDelegate*) GetInstance;

@end



