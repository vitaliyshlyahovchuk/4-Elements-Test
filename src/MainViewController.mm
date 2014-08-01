//
//  MainViewController.m
//  80days_ipad
//
//  Created by Slava on 12/29/10.
//  Copyright 2010 Playrix Entertainment. All rights reserved.
//

//No need to modify this code -- VLAD KHOREV

#import "MainViewController.h"
#import "Engine.h"

@implementation MainViewController

- (BOOL) shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
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

- (void)didReceiveMemoryWarning {
    //[Engine releaseByWarning];
    [super didReceiveMemoryWarning];
}

// To hide status bar in iOS 7.
- (BOOL)prefersStatusBarHidden {
	return YES;
}
@end
