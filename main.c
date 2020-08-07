
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
#define objc_msgSend_id_char_const  ((id (*)(id, SEL, char const *))objc_msgSend)

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
  id poolAlloc = objc_msgSend_id(cast(id)objc_getClass("NSAutoreleasePool"), sel_registerName("alloc"));
  id pool = objc_msgSend_id(poolAlloc, sel_registerName("init"));

  objc_msgSend_id((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
  ((void (*)(id, SEL, NSInteger))objc_msgSend)(NSApp, sel_registerName("setActivationPolicy:"), 0);


  Class AppDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "AppDelegate", 0);
  bool resultAddProtoc = class_addProtocol(AppDelegateClass, objc_getProtocol("NSApplicationDelegate"));
  assert(resultAddProtoc);
  bool resultAddMethod = class_addMethod(AppDelegateClass, sel_registerName("applicationShouldTerminate:"), (IMP)applicationShouldTerminate, NSUIntegerEncoding "@:@");
  assert(resultAddMethod);
  id dgAlloc = objc_msgSend_id(cast(id)AppDelegateClass, sel_registerName("alloc"));
  id dg = objc_msgSend_id(dgAlloc, sel_registerName("init"));

  objc_msgSend_void_id(NSApp, sel_registerName("setDelegate:"), dg);
  objc_msgSend_void(NSApp, sel_registerName("finishLaunching"));

  id menubarAlloc = objc_msgSend_id(cast(id)objc_getClass("NSMenu"), sel_registerName("alloc"));
  id menubar = objc_msgSend_id(menubarAlloc, sel_registerName("init"));

  id appMenuItemAlloc = objc_msgSend_id(cast(id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
  id appMenuItem = objc_msgSend_id(appMenuItemAlloc, sel_registerName("init"));

  objc_msgSend_void_id(menubar, sel_registerName("addItem:"), appMenuItem);
  ((id (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("setMainMenu:"), menubar);

  id appMenuAlloc = objc_msgSend_id(cast(id)objc_getClass("NSMenu"), sel_registerName("alloc"));
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
  id window;
  {
    NSRect rect = {{0, 0}, {(CGFloat)1280, (CGFloat)720}};
    id windowAlloc = objc_msgSend_id(cast(id)objc_getClass("NSWindow"), sel_registerName("alloc"));
    window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(windowAlloc, sel_registerName("initWithContentRect:styleMask:backing:defer:"), rect, 15, 2, NO);

    // when we are not using ARC, than window will be added to autorelease pool
    // so if we close it by hand (pressing red button), we don't want it to be released for us
    // so it will be released by autorelease pool later
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
    u32 opengl_minor = 2;
    u32 opengl_hex_version = (opengl_major << 12) | (opengl_minor << 8);
    u32 gl_attribs[] = {
      8, 24,                  // NSOpenGLPFAColorSize, 24,
      11, 8,                  // NSOpenGLPFAAlphaSize, 8,
      5,                      // NSOpenGLPFADoubleBuffer,
      73,                     // NSOpenGLPFAAccelerated,
      72,                     // NSOpenGLPFANoRecovery,
      55, 1,                  // NSOpenGLPFASampleBuffers, 1,
      56, 4,                  // NSOpenGLPFASamples, 4,
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

  objc_msgSend_id(opengl_context, sel_registerName("makeCurrentContext"));

  // explicit runloop
  printf("entering runloop\n");

  SEL flushBufferSel = sel_registerName("flushBuffer");

  while (!terminated) {
    id distant_past = objc_msgSend_id(cast(id)objc_getClass("NSDate"), sel_registerName("distantPast"));
    id event = ((id (*)(id, SEL, NSUInteger, id, id, BOOL))objc_msgSend)(NSApp, sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"), NSUIntegerMax, distant_past, NSDefaultRunLoopMode, YES);

    if (event) {
      NSUInteger event_type = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("type"));

      switch (event_type) {
        //case NSMouseMoved:
        //case NSLeftMouseDragged:
        //case NSRightMouseDragged:
        //case NSOtherMouseDragged:
        case 5:
        case 6:
        case 7:
        case 27: {
          id window = objc_msgSend_id(NSApp, sel_registerName("keyWindow"));

          id content_view = objc_msgSend_id(window, sel_registerName("contentView"));
          NSRect original_frame = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(content_view, sel_registerName("frame"));

          NSPoint p = ((NSPoint (*)(id, SEL))objc_msgSend)(window, sel_registerName("mouseLocationOutsideOfEventStream"));

          // map input to content view rect
          if (p.x < 0) p.x = 0;
          else if (p.x > original_frame.size.width) p.x = original_frame.size.width;
          if (p.y < 0) p.y = 0;
          else if (p.y > original_frame.size.height) p.y = original_frame.size.height;

          // map input to pixels
          NSRect r = {p.x, p.y, 0, 0};
          r = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(content_view, sel_registerName("convertRectToBacking:"), r);
          p = r.origin;

          printf("mouse moved to %f %f\n", p.x, p.y);
        } break;

        //case NSLeftMouseDown:
        case 1: {
          printf("mouse left key down\n");
        } break;

        //case NSLeftMouseUp:
        case 2: {
          printf("mouse left key up\n");
        } break;

        //case NSRightMouseDown:
        case 3: {
          printf("mouse right key down\n");
        } break;

        //case NSRightMouseUp:
        case 4: {
          printf("mouse right key up\n");
        } break;

        //case NSOtherMouseDown:
        case 25: {
          // number == 2 is a middle button
          //NSInteger number = [event buttonNumber];
          NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("buttonNumber"));
          printf("mouse other key down : %i\n", (int)number);
        } break;

        //case NSOtherMouseUp:
        case 26: {
          //NSInteger number = [event buttonNumber];
          NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("buttonNumber"));
          printf("mouse other key up : %i\n", (int)number);
        } break;

        //case NSScrollWheel:
        case 22: {
          CGFloat dx = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, sel_registerName("scrollingDeltaX"));
          CGFloat dy = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, sel_registerName("scrollingDeltaY"));
          BOOL precision_scrolling = ((BOOL (*)(id, SEL))objc_msgSend)(event, sel_registerName("hasPreciseScrollingDeltas"));

          if (precision_scrolling) {
           // similar to glfw
            dx *= 0.1f;
            dy *= 0.1f;
          }

          if (fabs(dx) > 0.0f || fabs(dy) > 0.0f) {
            printf("mouse scroll wheel delta %f %f\n", dx, dy);
          }
        } break;

        //case NSFlagsChanged:
        case 12: {
          NSUInteger modifiers = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("modifierFlags"));

          // based on NSEventModifierFlags
          union {
            struct {
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
          } keys;

          //keys.mask = (modifiers & NSDeviceIndependentModifierFlagsMask) >> 16;
          keys.mask = (modifiers & 0xffff0000UL) >> 16;

          printf("mod keys : mask %03u state %u%u%u%u%u%u%u%u\n", keys.mask, keys.alpha_shift, keys.shift, keys.control, keys.alternate, keys.command, keys.numeric_pad, keys.help, keys.function);
        } break;

        //case NSKeyDown:
        case 10: {
          id inputText = objc_msgSend_id(event, sel_registerName("characters"));
          const char *inputTextUTF8 = ((const char* (*)(id, SEL))objc_msgSend)(inputText, sel_registerName("UTF8String"));

          //you can get list of virtual key codes from Carbon HIToolbox/Events.h
          uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, sel_registerName("keyCode"));

          printf("key down %u, text '%s'\n", keyCode, inputTextUTF8);
        } break;

        //case NSKeyUp:
        case 11: {
          uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, sel_registerName("keyCode"));

          printf("key up %u\n", keyCode);
        } break;

        default: { } break;
      }

      objc_msgSend_void_id(NSApp, sel_registerName("sendEvent:"), event);

      if (terminated) {
        break;
      }

      objc_msgSend_id(NSApp, sel_registerName("updateWindows"));
    }

    // do runloop stuff
    // @Incomplete: probably we only need to do it when we resize the window
    objc_msgSend_id(opengl_context, sel_registerName("update"));

    content_view = objc_msgSend_id(window, sel_registerName("contentView"));
    NSRect original_frame = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(content_view, sel_registerName("frame"));

    // Window size and position
    {
      NSRect frame = original_frame;
      frame = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(content_view, sel_registerName("convertRectToBacking:"), frame);
      //p->window_width  = frame.size.width;
      //p->window_height = frame.size.height;

      glViewport(0, 0, frame.size.width, frame.size.height);

      frame = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(window, sel_registerName("convertRectToScreen:"), frame);
      //p->window_x = frame.origin.x;
      //p->window_y = frame.origin.y;
    }

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    objc_msgSend_id(opengl_context, sel_registerName("makeCurrentContext"));
    objc_msgSend_id(opengl_context, sel_registerName("flushBuffer"));
  }

  printf("gracefully terminated\n");

  objc_msgSend_void(pool, sel_registerName("drain"));

  return 0;
}
