/*
 Copyright (C) 2010 Apple Inc. All Rights Reserved.
*/

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "MyApplication.h"

@class AppDelegate;

/*
This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
The view content is basically an EAGL surface you render your OpenGL scene into.
Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
*/

@interface EAGLView : UIView {
    
@private

	BOOL animating;
	
	BOOL displayLinkSupported;
	
	NSTimer *animationTimer;
	//Timer for animation without display link
	
	NSInteger animationFrameInterval;
	// Use of the CADisplayLink class is the preferred method for controlling your animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	// The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
	// isn't available.
	
	id displayLink;
	
	IBOutlet AppDelegate* appDelegate;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

-(void) resolutionWidth:(int)rw resolutionHeight:(int)rh matrixWidth:(int)mw matrixHeight:(int)mh;

- (void) startAnimation;
- (void) stopAnimation;
- (void) drawView:(id)sender;
- (void) tryDisableDisplayLinks;
- (void) tryEnableDisplayLinks;

@end
