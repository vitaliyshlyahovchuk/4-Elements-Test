/*
 Copyright (C) 2010 Apple Inc. All Rights Reserved.
*/

//Last modified by VLAD KHOREV

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"
#import "PlayrixEngine.h"
#import "MyApplication.h"
#import "Engine.h"

@interface EAGLView ()

//Add here all private methods you need

@end


@implementation EAGLView

@synthesize animating;
@dynamic animationFrameInterval;

// You must implement this method
+ (Class)layerClass 
{
    return [CAEAGLLayer class];
}

//Use this function to change resolution. See playrix knowledge base for more info
-(void) resolutionWidth:(int)rw resolutionHeight:(int)rh matrixWidth:(int)mw matrixHeight:(int)mh
{
	CGRect tempRect = self.bounds;
	
	self.bounds = CGRectMake(0, 0, rw, rh);
	
	CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
	
	[Engine recreateApplicationWithLayer:eaglLayer setMatrixWidth:mw setMatrixHeight:mh];
	
	self.bounds = tempRect;
}

- (BOOL) checkDisplayLinkSupported {
    // A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
    // class is used as fallback when it isn't available.
    NSString *reqSysVer = @"3.1";
    NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending) {
        return TRUE;
    } else {
        return FALSE;
    }
}
    
//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder 
{
    
    if ((self = [super initWithCoder:coder])) 
	{
       
		// Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
        
		UIPinchGestureRecognizer *pinchGesture = [[UIPinchGestureRecognizer alloc]
                                                  initWithTarget:self action:@selector(handlePinchGesture:)];
        [self addGestureRecognizer:pinchGesture];
        [pinchGesture release];
		
        animating = FALSE;
		displayLinkSupported = [self checkDisplayLinkSupported];
		animationFrameInterval = 1;
		displayLink = nil;
		animationTimer = nil;
	}
    return self;
}

- (void) tryDisableDisplayLinks {
    [self stopAnimation];
    displayLinkSupported = FALSE;
    [self startAnimation];
}

- (void) tryEnableDisplayLinks {
    [self stopAnimation];
    displayLinkSupported = [self checkDisplayLinkSupported];
    [self startAnimation];
}

- (void)drawView:(id)sender
{
	[appDelegate update];
}

- (void)layoutSubviews {
    [self drawView:nil];
}


- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!animating)
	{
		if (displayLinkSupported)
		{
			displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[displayLink setFrameInterval:animationFrameInterval];
			[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			animationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 60.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[displayLink invalidate];
			displayLink = nil;
		}
		else
		{
			[animationTimer invalidate];
			animationTimer = nil;
		}
		
		animating = FALSE;
	}
}

- (void)dealloc 
{
    [super dealloc];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*) __unused event
{
	UITouch *touch = [[touches allObjects] objectAtIndex:0];
	CGPoint point = [touch locationInView:self];
	if (isRetina()) {
		point.x *= 2; point.y *= 2;
	}
	Core::appInstance->setMousePos(point.x, point.y);
	Core::mainInput.MouseLeftButtonDown();
	static double oldtouchtime = 0;
	double touchtime = CFAbsoluteTimeGetCurrent();
	if(touchtime - oldtouchtime < 0.5f) {
		Core::mainInput.MouseDoubleClick();
		oldtouchtime = touchtime - 1000;
	} else {
		oldtouchtime = touchtime;
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*) __unused event 
{
	UITouch *touch = [[touches allObjects] objectAtIndex:0];
	CGPoint point = [touch locationInView:self];
	if (isRetina()) {
		point.x *= 2; point.y *= 2;
	}
	Core::appInstance->updateMousePos(point.x, point.y);
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*) __unused event 
{
	UITouch *touch = [[touches allObjects] objectAtIndex:0];
	CGPoint point = [touch locationInView:self];
	if (isRetina()) {
		point.x *= 2; point.y *= 2;
	}
	Core::appInstance->setMousePos(point.x, point.y);
	Core::mainInput.MouseLeftButtonUp();
}

- (void)touchesCanceled:(NSSet*)touches withEvent:(UIEvent*) __unused event 
{
	UITouch *touch = [[touches allObjects] objectAtIndex:0];
	CGPoint point = [touch locationInView:self];
	if (isRetina()) {
		point.x *= 2; point.y *= 2;
	}
	Core::appInstance->setMousePos(point.x, point.y);
	Core::mainInput.MouseLeftButtonUp();
}


//float prevScale = 1.0f;
float prevScale = -1.0f;
- (void)handlePinchGesture:(UIPinchGestureRecognizer *)gestureRecongnizer
{
	float SCR_DIAG = math::sqrt(MyApplication::GAME_WIDTH*MyApplication::GAME_WIDTH + MyApplication::GAME_HEIGHT*MyApplication::GAME_HEIGHT);
    if ([gestureRecongnizer numberOfTouches] == 2)
	{
		CGPoint firstPoint = [(UIPinchGestureRecognizer *)gestureRecongnizer locationOfTouch: (NSUInteger)0 inView: self];
		CGPoint secondPoint = [(UIPinchGestureRecognizer *)gestureRecongnizer locationOfTouch: (NSUInteger)1 inView: self];
		CGPoint anchorPoint = CGPointMake((firstPoint.x + secondPoint.x) / 2.0f, (firstPoint.y + secondPoint.y) / 2.0f);

		//CGFloat factor = [(UIPinchGestureRecognizer *)gestureRecongnizer scale];

        
		CGPoint point = anchorPoint;//[gestureRecongnizer locationInView:self];
		if (isRetina()) {
			point.x *= 2; point.y *= 2;
        }
        
        
        //------------------------------------------------------------
        if (isRetina()) {
			firstPoint.x*=2; firstPoint.y*=2;
            secondPoint.x*=2; secondPoint.y*=2;
		}
        float cathetusX = abs(firstPoint.x-secondPoint.x);
        float cathetusY = abs(firstPoint.y-secondPoint.y);
        float factor = math::sqrt(cathetusX*cathetusX+cathetusY*cathetusY)/SCR_DIAG;
        //------------------------------------------------------------
		
		if([gestureRecongnizer state] == UIGestureRecognizerStateBegan)
		{
            float delta = (prevScale<0)? 0.f : factor - prevScale;
            Core::appInstance->pinchBegan(delta, point.x, point.y);
		}
		
		if ([gestureRecongnizer state] == UIGestureRecognizerStateBegan ||
			[gestureRecongnizer state] == UIGestureRecognizerStateChanged)
		{
            float delta = (prevScale<0)? 0.f : factor - prevScale;
            Core::appInstance->pinchChanged(delta, point.x, point.y);
            
		}
		
		prevScale = factor;
	}
	else{
		Core::mainInput.PinchEnded();
		 Core::mainInput.MouseLeftButtonUp();
		//prevScale = 1.0f;
        prevScale = -1.0f;
	}
	
	if ([gestureRecongnizer state] == UIGestureRecognizerStateEnded)
	{
		Core::mainInput.PinchEnded();
		 Core::mainInput.MouseLeftButtonUp();
		//prevScale = 1.0f;
        prevScale = -1.0f;
	}
}


@end
