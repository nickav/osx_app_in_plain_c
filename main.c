
// this code should work if compiled as C99+ or Objective-C (with or without ARC)

// we don't need much here
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <OpenGL/gl.h>

// maybe this is available somewhere in objc runtime?
#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

// this is how they are defined originally
#ifdef __OBJC__
	#import <Cocoa/Cocoa.h>
#else
#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
  typedef CGPoint NSPoint;
  typedef CGRect NSRect;

  extern id NSApp;
  extern id const NSDefaultRunLoopMode;
#endif

// ABI is a bit different between platforms
#ifdef __arm64__
#define abi_objc_msgSend_stret objc_msgSend
#else
#define abi_objc_msgSend_stret objc_msgSend_stret
#endif
#ifdef __i386__
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#else
#define abi_objc_msgSend_fpret objc_msgSend
#endif

#define objc_msgSend_id             ((id (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void           ((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id        ((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_bool      ((void (*)(id, SEL, BOOL))objc_msgSend)
#define objc_msgSend_id_char_const	((id (*)(id, SEL, char const *))objc_msgSend)

#define cast(Type) (Type)
#define u32 unsigned int

bool terminated = false;

NSUInteger applicationShouldTerminate(id self, SEL _sel, id sender) {
	printf("requested to terminate\n");
	terminated = true;
	return 0;
}

void windowWillClose(id self, SEL _sel, id notification) {
	printf("window will close\n");
  terminated = true;
}

int main() {
  // Init Platform
  id poolAlloc = objc_msgSend_id((id)objc_getClass("NSAutoreleasePool"), sel_registerName("alloc"));
  id pool = objc_msgSend_id(poolAlloc, sel_registerName("init"));

  objc_msgSend_id((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
  ((void (*)(id, SEL, NSInteger))objc_msgSend)(NSApp, sel_registerName("setActivationPolicy:"), 0);


	Class AppDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "AppDelegate", 0);
	bool resultAddProtoc = class_addProtocol(AppDelegateClass, objc_getProtocol("NSApplicationDelegate"));
	assert(resultAddProtoc);
	bool resultAddMethod = class_addMethod(AppDelegateClass, sel_registerName("applicationShouldTerminate:"), (IMP)applicationShouldTerminate, NSUIntegerEncoding "@:@");
	assert(resultAddMethod);
  id dgAlloc = objc_msgSend_id((id)AppDelegateClass, sel_registerName("alloc"));
  id dg = objc_msgSend_id(dgAlloc, sel_registerName("init"));

  objc_msgSend_void_id(NSApp, sel_registerName("setDelegate:"), dg);
  objc_msgSend_void(NSApp, sel_registerName("finishLaunching"));

  id menubarAlloc = objc_msgSend_id((id)objc_getClass("NSMenu"), sel_registerName("alloc"));
  id menubar = objc_msgSend_id(menubarAlloc, sel_registerName("init"));

  id appMenuItemAlloc = objc_msgSend_id((id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
  id appMenuItem = objc_msgSend_id(appMenuItemAlloc, sel_registerName("init"));

  objc_msgSend_void_id(menubar, sel_registerName("addItem:"), appMenuItem);
  ((id (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("setMainMenu:"), menubar);

  id appMenuAlloc = objc_msgSend_id((id)objc_getClass("NSMenu"), sel_registerName("alloc"));
  id appMenu = objc_msgSend_id(appMenuAlloc, sel_registerName("init"));

  {
    id processInfo = objc_msgSend_id(cast(id)objc_getClass("NSProcessInfo"), sel_registerName("processInfo"));
    id appName = objc_msgSend_id(processInfo, sel_registerName("processName"));

    id quitTitlePrefixString = objc_msgSend_id_char_const(cast(id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "Quit ");
    id quitTitle = ((id (*)(id, SEL, id))objc_msgSend)(quitTitlePrefixString, sel_registerName("stringByAppendingString:"), appName);

    id quitMenuItemKey = objc_msgSend_id_char_const(cast(id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "q");
    id quitMenuItemAlloc = objc_msgSend_id(cast(id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
    id quitMenuItem = ((id (*)(id, SEL, id, SEL, id))objc_msgSend)(quitMenuItemAlloc, sel_registerName("initWithTitle:action:keyEquivalent:"), quitTitle, sel_registerName("terminate:"), quitMenuItemKey);

    objc_msgSend_void_id(appMenu, sel_registerName("addItem:"), quitMenuItem);
    objc_msgSend_void_id(appMenuItem, sel_registerName("setSubmenu:"), appMenu);
  }

  // Init window
  id opengl_context;
  id content_view;
  {
    NSRect rect = {{0, 0}, {(CGFloat)1280, (CGFloat)720}};
    id windowAlloc = objc_msgSend_id(cast(id)objc_getClass("NSWindow"), sel_registerName("alloc"));
    id window = window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(windowAlloc, sel_registerName("initWithContentRect:styleMask:backing:defer:"), rect, 15, 2, NO);

    // when we are not using ARC, than window will be added to autorelease pool
    // so if we close it by hand (pressing red button), we don't want it to be released for us
    // so it will be released by autorelease pool later
    //[window setReleasedWhenClosed:NO];
    objc_msgSend_void_bool(window, sel_registerName("setReleasedWhenClosed:"), NO);

    Class WindowDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "WindowDelegate", 0);
    resultAddProtoc = class_addProtocol(WindowDelegateClass, objc_getProtocol("NSWindowDelegate"));
    assert(resultAddProtoc);

    SEL windowWillCloseSel = sel_registerName("windowWillClose:");
    resultAddMethod = class_addMethod(WindowDelegateClass, windowWillCloseSel, (IMP)windowWillClose,  "v@:@");
    assert(resultAddMethod);
    id wdgAlloc = ((id (*)(Class, SEL))objc_msgSend)(WindowDelegateClass, sel_registerName("alloc"));
    id wdg = ((id (*)(id, SEL))objc_msgSend)(wdgAlloc, sel_registerName("init"));

    content_view  = objc_msgSend_id(window, sel_registerName("contentView"));

    // disable this if you don't want retina support
    objc_msgSend_void_bool(content_view , sel_registerName("setWantsBestResolutionOpenGLSurface:"), YES);

    {
			NSPoint point = {20, 20};
			((void (*)(id, SEL, NSPoint))objc_msgSend)(window, sel_registerName("cascadeTopLeftFromPoint:"), point);
		}

    const char *window_title = "sup from C";
    id titleString = objc_msgSend_id_char_const(cast(id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), window_title);
    objc_msgSend_void_id(window, sel_registerName("setTitle:"), titleString);

    u32 opengl_major = 3;
    u32 opengl_minor = 3;
    u32 opengl_hex_version = (opengl_major << 12) | (opengl_minor << 8);
    u32 gl_attribs[] = {
      8, 24,                  // NSOpenGLPFAColorSize, 24,
      11, 8,                  // NSOpenGLPFAAlphaSize, 8,
      5,                      // NSOpenGLPFADoubleBuffer,
      73,                     // NSOpenGLPFAAccelerated,
      //72,                   // NSOpenGLPFANoRecovery,
      //55, 1,                // NSOpenGLPFASampleBuffers, 1,
      //56, 4,                // NSOpenGLPFASamples, 4,
      99, opengl_hex_version, // NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      0
    };

    id pixel_format_alloc = objc_msgSend_id(cast(id)objc_getClass("NSOpenGLPixelFormat"), sel_registerName("alloc"));
    id pixel_format = ((id (*)(id, SEL, const uint32_t*))objc_msgSend)(pixel_format_alloc, sel_registerName("initWithAttributes:"), gl_attribs);

    id opengl_context_alloc = objc_msgSend_id(cast(id)objc_getClass("NSOpenGLContext"), sel_registerName("alloc"));
    opengl_context = ((id (*)(id, SEL, id, id))objc_msgSend)(opengl_context_alloc, sel_registerName("initWithFormat:shareContext:"), pixel_format, nil);

    objc_msgSend_void_id(opengl_context, sel_registerName("setView:"), content_view );
    objc_msgSend_void_id(window, sel_registerName("makeKeyAndOrderFront:"), window);
    objc_msgSend_void_bool(window, sel_registerName("setAcceptsMouseMovedEvents:"), YES);

    objc_msgSend_void_id(opengl_context, sel_registerName("setView:"), content_view );

    {
			id blackColor = objc_msgSend_id(cast(id)objc_getClass("NSColor"), sel_registerName("blackColor"));
			objc_msgSend_void_id(window, sel_registerName("setBackgroundColor:"), blackColor);
			objc_msgSend_void_bool(NSApp, sel_registerName("activateIgnoringOtherApps:"), YES);
		}
  }

	// explicit runloop
	printf("entering runloop\n");

	Class NSDateClass = objc_getClass("NSDate");
	SEL distantPastSel = sel_registerName("distantPast");
	SEL nextEventMatchingMaskSel = sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:");
	SEL frameSel = sel_registerName("frame");
	SEL typeSel = sel_registerName("type");
	SEL buttonNumberSel = sel_registerName("buttonNumber");
	SEL keyCodeSel = sel_registerName("keyCode");
	SEL keyWindowSel = sel_registerName("keyWindow");
	SEL mouseLocationOutsideOfEventStreamSel = sel_registerName("mouseLocationOutsideOfEventStream");
	SEL convertRectToBackingSel = sel_registerName("convertRectToBacking:");
	SEL scrollingDeltaXSel = sel_registerName("scrollingDeltaX");
	SEL scrollingDeltaYSel = sel_registerName("scrollingDeltaY");
	SEL hasPreciseScrollingDeltasSel = sel_registerName("hasPreciseScrollingDeltas");
	SEL modifierFlagsSel = sel_registerName("modifierFlags");
	SEL charactersSel = sel_registerName("characters");
	SEL UTF8StringSel = sel_registerName("UTF8String");
	SEL sendEventSel = sel_registerName("sendEvent:");
	SEL updateWindowsSel = sel_registerName("updateWindows");
	SEL updateSel = sel_registerName("update");
	SEL makeCurrentContextSel = sel_registerName("makeCurrentContext");
	SEL flushBufferSel = sel_registerName("flushBuffer");

	while(!terminated)
	{
		//NSEvent * event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
		id distantPast = ((id (*)(Class, SEL))objc_msgSend)(NSDateClass, distantPastSel);
		id event = ((id (*)(id, SEL, NSUInteger, id, id, BOOL))objc_msgSend)(NSApp, nextEventMatchingMaskSel, NSUIntegerMax, distantPast, NSDefaultRunLoopMode, YES);

		if(event)
		{
			//NSEventType eventType = [event type];
			NSUInteger eventType = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, typeSel);

			switch(eventType)
			{
				//case NSMouseMoved:
				//case NSLeftMouseDragged:
				//case NSRightMouseDragged:
				//case NSOtherMouseDragged:
				case 5:
				case 6:
				case 7:
				case 27:
				{
					//NSWindow * currentWindow = [NSApp keyWindow];
					id currentWindow = ((id (*)(id, SEL))objc_msgSend)(NSApp, keyWindowSel);

					//NSRect adjustFrame = [[currentWindow contentView] frame];
					id currentWindowContentView = ((id (*)(id, SEL))objc_msgSend)(currentWindow, sel_registerName("contentView"));
					NSRect adjustFrame = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(currentWindowContentView, frameSel);

					//NSPoint p = [currentWindow mouseLocationOutsideOfEventStream];
					// NSPoint is small enough to fit a register, so no need for objc_msgSend_stret
					NSPoint p = ((NSPoint (*)(id, SEL))objc_msgSend)(currentWindow, mouseLocationOutsideOfEventStreamSel);

					// map input to content view rect
					if(p.x < 0) p.x = 0;
					else if(p.x > adjustFrame.size.width) p.x = adjustFrame.size.width;
					if(p.y < 0) p.y = 0;
					else if(p.y > adjustFrame.size.height) p.y = adjustFrame.size.height;

					// map input to pixels
					NSRect r = {p.x, p.y, 0, 0};
					//r = [currentWindowContentView convertRectToBacking:r];
					r = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(currentWindowContentView, convertRectToBackingSel, r);
					p = r.origin;

					printf("mouse moved to %f %f\n", p.x, p.y);
					break;
				}
				//case NSLeftMouseDown:
				case 1:
					printf("mouse left key down\n");
					break;
				//case NSLeftMouseUp:
				case 2:
					printf("mouse left key up\n");
					break;
				//case NSRightMouseDown:
				case 3:
					printf("mouse right key down\n");
					break;
				//case NSRightMouseUp:
				case 4:
					printf("mouse right key up\n");
					break;
				//case NSOtherMouseDown:
				case 25:
				{
					// number == 2 is a middle button
					//NSInteger number = [event buttonNumber];
					NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, buttonNumberSel);
					printf("mouse other key down : %i\n", (int)number);
					break;
				}
				//case NSOtherMouseUp:
				case 26:
				{
					//NSInteger number = [event buttonNumber];
					NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, buttonNumberSel);
					printf("mouse other key up : %i\n", (int)number);
					break;
				}
				//case NSScrollWheel:
				case 22:
				{
					//CGFloat deltaX = [event scrollingDeltaX];
					CGFloat deltaX = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, scrollingDeltaXSel);

					//CGFloat deltaY = [event scrollingDeltaY];
					CGFloat deltaY = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, scrollingDeltaYSel);

					//BOOL precisionScrolling = [event hasPreciseScrollingDeltas];
					BOOL precisionScrolling = ((BOOL (*)(id, SEL))objc_msgSend)(event, hasPreciseScrollingDeltasSel);

					if(precisionScrolling)
					{
						deltaX *= 0.1f; // similar to glfw
						deltaY *= 0.1f;
					}

					if(fabs(deltaX) > 0.0f || fabs(deltaY) > 0.0f)
						printf("mouse scroll wheel delta %f %f\n", deltaX, deltaY);
					break;
				}
				//case NSFlagsChanged:
				case 12:
				{
					//NSEventModifierFlags modifiers = [event modifierFlags];
					NSUInteger modifiers = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, modifierFlagsSel);

					// based on NSEventModifierFlags
					struct
					{
						union
						{
							struct
							{
								uint8_t alpha_shift:1;
								uint8_t shift:1;
								uint8_t control:1;
								uint8_t alternate:1;
								uint8_t command:1;
								uint8_t numeric_pad:1;
								uint8_t help:1;
								uint8_t function:1;
							};
							uint8_t mask;
						};
					} keys;

					//keys.mask = (modifiers & NSDeviceIndependentModifierFlagsMask) >> 16;
					keys.mask = (modifiers & 0xffff0000UL) >> 16;

					printf("mod keys : mask %03u state %u%u%u%u%u%u%u%u\n", keys.mask, keys.alpha_shift, keys.shift, keys.control, keys.alternate, keys.command, keys.numeric_pad, keys.help, keys.function);
					break;
				}
				//case NSKeyDown:
				case 10:
				{
					//NSString * inputText = [event characters];
					id inputText = ((id (*)(id, SEL))objc_msgSend)(event, charactersSel);

					//const char * inputTextUTF8 = [inputText UTF8String];
					const char * inputTextUTF8 = ((const char* (*)(id, SEL))objc_msgSend)(inputText, UTF8StringSel);

					//you can get list of virtual key codes from Carbon HIToolbox/Events.h
					//uint16_t keyCode = [event keyCode];
					uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, keyCodeSel);

					printf("key down %u, text '%s'\n", keyCode, inputTextUTF8);
					break;
				}
				//case NSKeyUp:
				case 11:
				{
					//uint16_t keyCode = [event keyCode];
					uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, keyCodeSel);

					printf("key up %u\n", keyCode);
					break;
				}
				default:
					break;
			}

			//[NSApp sendEvent:event];
			((void (*)(id, SEL, id))objc_msgSend)(NSApp, sendEventSel, event);

			// if user closes the window we might need to terminate asap
			if(terminated)
				break;

			//[NSApp updateWindows];
			((void (*)(id, SEL))objc_msgSend)(NSApp, updateWindowsSel);
		}

		// do runloop stuff
		//[openGLContext update]; // probably we only need to do it when we resize the window
		((void (*)(id, SEL))objc_msgSend)(opengl_context, updateSel);

		//[openGLContext makeCurrentContext];
		((void (*)(id, SEL))objc_msgSend)(opengl_context, makeCurrentContextSel);

		//NSRect rect = [contentView frame];
		NSRect rect = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(content_view , frameSel);

		//rect = [contentView convertRectToBacking:rect];
		rect = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(content_view , convertRectToBackingSel, rect);

		glViewport(0, 0, rect.size.width, rect.size.height);

		glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1.0f, 0.85f, 0.35f);
		glBegin(GL_TRIANGLES);
		{
			glVertex3f(  0.0f,  0.6f, 0.0f);
			glVertex3f( -0.2f, -0.3f, 0.0f);
			glVertex3f(  0.2f, -0.3f ,0.0f);
		}
		glEnd();

		//[openGLContext flushBuffer];
		((void (*)(id, SEL))objc_msgSend)(opengl_context, flushBufferSel);
	}

	printf("gracefully terminated\n");

	//[pool drain];
	((void (*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
	return 0;
}
