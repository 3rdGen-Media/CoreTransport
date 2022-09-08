//
//  CTDeviceInput.h
//  HappyEyeballs
//
//  Created by Joe Moulton on 7/24/22.
//

#ifndef CT_DEVICE_INPUT_H
#define CT_DEVICE_INPUT_H

//Keyboard Event Loop Includes and Definitions (TO DO: Move this to its own header file)
#if !defined(__APPLE__) && ! defined(_WIN32)
//#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdbool.h>
#include <string.h>
//#include "../../Git/libdispatch/dispatch/dispatch.h"

struct termios orig_termios;
typedef struct KEY_EVENT_RECORD
{
    bool bKeyDown;
    unsigned short wRepeatCount;
    unsigned short wVirtualKeyCode;
    unsigned short wVirtualScanCode;
    union {
        wchar_t UnicodeChar;
        char   AsciiChar;
    } uChar;
    unsigned long dwControlKeyState;
}KEY_EVENT_RECORD;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}


#elif defined (__APPLE__)
#include <TargetConditionals.h>
//#include <ApplicationServices/ApplicationServices.h> //Cocoa
#include  <CoreFoundation/CoreFoundation.h>            //Core Foundation
//Public Core Graphics API
#include <CoreGraphics/CoreGraphics.h>

#if !TARGET_OS_OSX

/* Event types */

#define NX_NULLEVENT        0    /* internal use */

/* mouse events */

#define NX_LMOUSEDOWN        1    /* left mouse-down event */
#define NX_LMOUSEUP        2    /* left mouse-up event */
#define NX_RMOUSEDOWN        3    /* right mouse-down event */
#define NX_RMOUSEUP        4    /* right mouse-up event */
#define NX_MOUSEMOVED        5    /* mouse-moved event */
#define NX_LMOUSEDRAGGED    6    /* left mouse-dragged event */
#define NX_RMOUSEDRAGGED    7    /* right mouse-dragged event */
#define NX_MOUSEENTERED        8    /* mouse-entered event */
#define NX_MOUSEEXITED        9    /* mouse-exited event */

/* other mouse events
 *
 * event.data.mouse.buttonNumber should contain the
 * button number (2-31) changing state.
 */
#define NX_OMOUSEDOWN        25    /* other mouse-down event */
#define NX_OMOUSEUP        26    /* other mouse-up event */
#define NX_OMOUSEDRAGGED    27    /* other mouse-dragged event */

/* keyboard events */

#define NX_KEYDOWN        10    /* key-down event */
#define NX_KEYUP        11    /* key-up event */
#define NX_FLAGSCHANGED        12    /* flags-changed event */

/* composite events */

#define NX_KITDEFINED        13    /* application-kit-defined event */
#define NX_SYSDEFINED        14    /* system-defined event */
#define NX_APPDEFINED        15    /* application-defined event */
/* There are additional DPS client defined events past this point. */

/* Scroll wheel events */

#define NX_SCROLLWHEELMOVED    22

/* Zoom events */
#define NX_ZOOM             28

/* tablet events */

#define NX_TABLETPOINTER    23    /* for non-mousing transducers */
#define NX_TABLETPROXIMITY    24  /* for non-mousing transducers */

/* event range */

#define NX_FIRSTEVENT        0
#define NX_LASTEVENT        28
#define NX_NUMPROCS        (NX_LASTEVENT-NX_FIRSTEVENT+1)

/* Event masks */
#define NX_NULLEVENTMASK        (1 << NX_NULLEVENT)     /* NULL event */
#define NX_LMOUSEDOWNMASK       (1 << NX_LMOUSEDOWN)    /* left mouse-down */
#define NX_LMOUSEUPMASK         (1 << NX_LMOUSEUP)      /* left mouse-up */
#define NX_RMOUSEDOWNMASK       (1 << NX_RMOUSEDOWN)    /* right mouse-down */
#define NX_RMOUSEUPMASK         (1 << NX_RMOUSEUP)      /* right mouse-up */
#define NX_OMOUSEDOWNMASK       (1 << NX_OMOUSEDOWN)    /* other mouse-down */
#define NX_OMOUSEUPMASK         (1 << NX_OMOUSEUP)      /* other mouse-up  */
#define NX_MOUSEMOVEDMASK       (1 << NX_MOUSEMOVED)    /* mouse-moved */
#define NX_LMOUSEDRAGGEDMASK    (1 << NX_LMOUSEDRAGGED)    /* left-dragged */
#define NX_RMOUSEDRAGGEDMASK    (1 << NX_RMOUSEDRAGGED)    /* right-dragged */
#define NX_OMOUSEDRAGGEDMASK    (1 << NX_OMOUSEDRAGGED)    /* other-dragged */
#define NX_MOUSEENTEREDMASK     (1 << NX_MOUSEENTERED)    /* mouse-entered */
#define NX_MOUSEEXITEDMASK      (1 << NX_MOUSEEXITED)    /* mouse-exited */
#define NX_KEYDOWNMASK          (1 << NX_KEYDOWN)       /* key-down */
#define NX_KEYUPMASK            (1 << NX_KEYUP)         /* key-up */
#define NX_FLAGSCHANGEDMASK     (1 << NX_FLAGSCHANGED)    /* flags-changed */
#define NX_KITDEFINEDMASK       (1 << NX_KITDEFINED)    /* kit-defined */
#define NX_SYSDEFINEDMASK       (1 << NX_SYSDEFINED)    /* system-defined */
#define NX_APPDEFINEDMASK       (1 << NX_APPDEFINED)    /* app-defined */
#define NX_SCROLLWHEELMOVEDMASK    (1 << NX_SCROLLWHEELMOVED)    /* scroll wheel moved */
#define NX_ZOOMMASK             (1 << NX_ZOOM)          /* Zoom */
#define NX_TABLETPOINTERMASK    (1 << NX_TABLETPOINTER)    /* tablet pointer moved */
#define NX_TABLETPROXIMITYMASK    (1 << NX_TABLETPROXIMITY)    /* tablet pointer proximity */

#define EventCodeMask(type)    (1 << (type))
#define NX_ALLEVENTS        -1    /* Check for all events */

/* sub types for mouse and move events */

#define NX_SUBTYPE_DEFAULT                    0
#define NX_SUBTYPE_TABLET_POINT                1
#define NX_SUBTYPE_TABLET_PROXIMITY            2
#define NX_SUBTYPE_MOUSE_TOUCH              3

/* sub types for system defined events */

#define NX_SUBTYPE_POWER_KEY                1
#define NX_SUBTYPE_AUX_MOUSE_BUTTONS        7

/*
 * NX_SUBTYPE_AUX_CONTROL_BUTTONS usage
 *
 * The incoming NXEvent for other mouse button down/up has event.type
 * NX_SYSDEFINED and event.data.compound.subtype NX_SUBTYPE_AUX_MOUSE_BUTTONS.
 * Within the event.data.compound.misc.L[0] contains bits for all the buttons
 * that have changed state, and event.data.compound.misc.L[1] contains the
 * current button state as a bitmask, with 1 representing down, and 0
 * representing up.  Bit 0 is the left button, bit one is the right button,
 * bit 2 is the center button and so forth.
 */
#define NX_SUBTYPE_AUX_CONTROL_BUTTONS        8

#define NX_SUBTYPE_EJECT_KEY                10
#define NX_SUBTYPE_SLEEP_EVENT                11
#define NX_SUBTYPE_RESTART_EVENT            12
#define NX_SUBTYPE_SHUTDOWN_EVENT            13
#define NX_SUBTYPE_MENU               16
#define NX_SUBTYPE_ACCESSIBILITY      17



#define NX_SUBTYPE_STICKYKEYS_ON            100
#define NX_SUBTYPE_STICKYKEYS_OFF            101
#define NX_SUBTYPE_STICKYKEYS_SHIFT            102
#define NX_SUBTYPE_STICKYKEYS_CONTROL            103
#define NX_SUBTYPE_STICKYKEYS_ALTERNATE            104
#define NX_SUBTYPE_STICKYKEYS_COMMAND            105
#define NX_SUBTYPE_STICKYKEYS_RELEASE            106
#define NX_SUBTYPE_STICKYKEYS_TOGGLEMOUSEDRIVING    107

// New stickykeys key events
// These were created to send an event describing the
// different state of the modifiers
#define NX_SUBTYPE_STICKYKEYS_SHIFT_DOWN        110
#define NX_SUBTYPE_STICKYKEYS_CONTROL_DOWN        111
#define NX_SUBTYPE_STICKYKEYS_ALTERNATE_DOWN        112
#define NX_SUBTYPE_STICKYKEYS_COMMAND_DOWN        113
#define NX_SUBTYPE_STICKYKEYS_FN_DOWN            114

#define NX_SUBTYPE_STICKYKEYS_SHIFT_LOCK        120
#define NX_SUBTYPE_STICKYKEYS_CONTROL_LOCK        121
#define NX_SUBTYPE_STICKYKEYS_ALTERNATE_LOCK        122
#define NX_SUBTYPE_STICKYKEYS_COMMAND_LOCK        123
#define NX_SUBTYPE_STICKYKEYS_FN_LOCK            124

#define NX_SUBTYPE_STICKYKEYS_SHIFT_UP            130
#define NX_SUBTYPE_STICKYKEYS_CONTROL_UP        131
#define NX_SUBTYPE_STICKYKEYS_ALTERNATE_UP        132
#define NX_SUBTYPE_STICKYKEYS_COMMAND_UP        133
#define NX_SUBTYPE_STICKYKEYS_FN_UP            134



// SlowKeys
#define NX_SUBTYPE_SLOWKEYS_START            200
#define NX_SUBTYPE_SLOWKEYS_ABORT            201
#define NX_SUBTYPE_SLOWKEYS_END                202

// HID Parameter Property Modified
#define NX_SUBTYPE_HIDPARAMETER_MODIFIED        210

/* Masks for the bits in event.flags */

/* device-independent */

#define    NX_ALPHASHIFTMASK    0x00010000
#define    NX_SHIFTMASK        0x00020000
#define    NX_CONTROLMASK        0x00040000
#define    NX_ALTERNATEMASK    0x00080000
#define    NX_COMMANDMASK        0x00100000
#define    NX_NUMERICPADMASK    0x00200000
#define    NX_HELPMASK        0x00400000
#define    NX_SECONDARYFNMASK    0x00800000
#define NX_ALPHASHIFT_STATELESS_MASK    0x01000000

/* device-dependent (really?) */

#define    NX_DEVICELCTLKEYMASK    0x00000001
#define    NX_DEVICELSHIFTKEYMASK    0x00000002
#define    NX_DEVICERSHIFTKEYMASK    0x00000004
#define    NX_DEVICELCMDKEYMASK    0x00000008
#define    NX_DEVICERCMDKEYMASK    0x00000010
#define    NX_DEVICELALTKEYMASK    0x00000020
#define    NX_DEVICERALTKEYMASK    0x00000040
#define NX_DEVICE_ALPHASHIFT_STATELESS_MASK 0x00000080
#define NX_DEVICERCTLKEYMASK    0x00002000

/*
 * Additional reserved bits in event.flags
 */

#define NX_STYLUSPROXIMITYMASK    0x00000080    /* deprecated */
#define NX_NONCOALSESCEDMASK    0x00000100

/* An opaque type that represents a low-level hardware event.

   Low-level hardware events of this type are referred to as Quartz events.
   A typical event in Mac OS X originates when the user manipulates an input
   device such as a mouse or a keyboard. The device driver associated with
   that device, through the I/O Kit, creates a low-level event, puts it in
   the window server’s event queue, and notifies the window server. The
   window server creates a Quartz event, annotates the event, and dispatches
   the event to the appropriate run-loop port of the target process. There
   the event is picked up by the Carbon Event Manager and forwarded to the
   event-handling mechanism appropriate to the application environment. You
   can use event taps to gain access to Quartz events at several different
   steps in this process.

   This opaque type is derived from `CFType' and inherits the properties
   that all Core Foundation types have in common. */

typedef struct CF_BRIDGED_TYPE(id) __CGEvent *CGEventRef;

/* Constants that specify buttons on a one, two, or three-button mouse. */
typedef CF_ENUM(uint32_t, CGMouseButton) {
  kCGMouseButtonLeft = 0,
  kCGMouseButtonRight = 1,
  kCGMouseButtonCenter = 2
};

/* Constants that specify the unit of measurement for a scrolling event. */
typedef CF_ENUM(uint32_t, CGScrollEventUnit) {
  kCGScrollEventUnitPixel = 0,
  kCGScrollEventUnitLine = 1,
};

/* Constants that specify momentum scroll phases. */
typedef CF_ENUM(uint32_t, CGMomentumScrollPhase) {
    kCGMomentumScrollPhaseNone = 0,
    kCGMomentumScrollPhaseBegin = 1,
    kCGMomentumScrollPhaseContinue = 2,
    kCGMomentumScrollPhaseEnd = 3
};

/* Constants that specify scroll phases. */
typedef CF_ENUM(uint32_t, CGScrollPhase) {
    kCGScrollPhaseBegan = 1,
    kCGScrollPhaseChanged = 2,
    kCGScrollPhaseEnded = 4,
    kCGScrollPhaseCancelled = 8,
    kCGScrollPhaseMayBegin = 128
};

/* Constants that specify gesture phases. */
typedef CF_ENUM(uint32_t, CGGesturePhase) {
    kCGGesturePhaseNone = 0,
    kCGGesturePhaseBegan = 1,
    kCGGesturePhaseChanged = 2,
    kCGGesturePhaseEnded = 4,
    kCGGesturePhaseCancelled = 8,
    kCGGesturePhaseMayBegin = 128
};

typedef CF_OPTIONS(uint64_t, CGEventFlags) { /* Flags for events */
/* Device-independent modifier key bits. */
kCGEventFlagMaskAlphaShift =          NX_ALPHASHIFTMASK,
kCGEventFlagMaskShift =               NX_SHIFTMASK,
kCGEventFlagMaskControl =             NX_CONTROLMASK,
kCGEventFlagMaskAlternate =           NX_ALTERNATEMASK,
kCGEventFlagMaskCommand =             NX_COMMANDMASK,

/* Special key identifiers. */
kCGEventFlagMaskHelp =                NX_HELPMASK,
kCGEventFlagMaskSecondaryFn =         NX_SECONDARYFNMASK,

/* Identifies key events from numeric keypad area on extended keyboards. */
kCGEventFlagMaskNumericPad =          NX_NUMERICPADMASK,

/* Indicates if mouse/pen movement events are not being coalesced */
kCGEventFlagMaskNonCoalesced =        NX_NONCOALSESCEDMASK
};

/* Constants that specify the different types of input events. */
typedef CF_ENUM(uint32_t, CGEventType) {
/* The null event. */
kCGEventNull = NX_NULLEVENT,

/* Mouse events. */
kCGEventLeftMouseDown = NX_LMOUSEDOWN,
kCGEventLeftMouseUp = NX_LMOUSEUP,
kCGEventRightMouseDown = NX_RMOUSEDOWN,
kCGEventRightMouseUp = NX_RMOUSEUP,
kCGEventMouseMoved = NX_MOUSEMOVED,
kCGEventLeftMouseDragged = NX_LMOUSEDRAGGED,
kCGEventRightMouseDragged = NX_RMOUSEDRAGGED,

/* Keyboard events. */
kCGEventKeyDown = NX_KEYDOWN,
kCGEventKeyUp = NX_KEYUP,
kCGEventFlagsChanged = NX_FLAGSCHANGED,

/* Specialized control devices. */
kCGEventScrollWheel = NX_SCROLLWHEELMOVED,
kCGEventTabletPointer = NX_TABLETPOINTER,
kCGEventTabletProximity = NX_TABLETPROXIMITY,
kCGEventOtherMouseDown = NX_OMOUSEDOWN,
kCGEventOtherMouseUp = NX_OMOUSEUP,
kCGEventOtherMouseDragged = NX_OMOUSEDRAGGED,

/* Out of band event types. These are delivered to the event tap callback
  to notify it of unusual conditions that disable the event tap. */
kCGEventTapDisabledByTimeout = 0xFFFFFFFE,
kCGEventTapDisabledByUserInput = 0xFFFFFFFF
};

/* Event timestamp; roughly, nanoseconds since startup. */
typedef uint64_t CGEventTimestamp;

/* Constants used as keys to access specialized fields in low-level events. */
typedef CF_ENUM(uint32_t, CGEventField) {
/* Key to access an integer field that contains the mouse button event
  number. Matching mouse-down and mouse-up events will have the same
  event number. */
kCGMouseEventNumber = 0,

/* Key to access an integer field that contains the mouse button click
state. A click state of 1 represents a single click. A click state of 2
represents a double-click. A click state of 3 represents a
triple-click. */
kCGMouseEventClickState = 1,

/* Key to access a double field that contains the mouse button pressure.
  The pressure value may range from 0 to 1, with 0 representing the mouse
  being up. This value is commonly set by tablet pens mimicking a
  mouse. */
kCGMouseEventPressure = 2,

/* Key to access an integer field that contains the mouse button
  number. */
kCGMouseEventButtonNumber = 3,

/* Key to access an integer field that contains the horizontal mouse delta
  since the last mouse movement event. */
kCGMouseEventDeltaX = 4,

/* Key to access an integer field that contains the vertical mouse delta
  since the last mouse movement event. */
kCGMouseEventDeltaY = 5,

/* Key to access an integer field. The value is non-zero if the event
  should be ignored by the Inkwell subsystem. */
kCGMouseEventInstantMouser = 6,

/* Key to access an integer field that encodes the mouse event subtype as
  a `kCFNumberIntType'. */
kCGMouseEventSubtype = 7,

/* Key to access an integer field, non-zero when this is an autorepeat of
  a key-down, and zero otherwise. */
kCGKeyboardEventAutorepeat = 8,

/* Key to access an integer field that contains the virtual keycode of the
  key-down or key-up event. */
kCGKeyboardEventKeycode = 9,

/* Key to access an integer field that contains the keyboard type
  identifier. */
kCGKeyboardEventKeyboardType = 10,

/* Key to access an integer field that contains scrolling data. This field
  typically contains the change in vertical position since the last
  scrolling event from a Mighty Mouse scroller or a single-wheel mouse
  scroller. */
kCGScrollWheelEventDeltaAxis1 = 11,

/* Key to access an integer field that contains scrolling data. This field
  typically contains the change in horizontal position since the last
  scrolling event from a Mighty Mouse scroller. */
kCGScrollWheelEventDeltaAxis2 = 12,

/* This field is not used. */
kCGScrollWheelEventDeltaAxis3 = 13,

/* Key to access a field that contains scrolling data. The scrolling data
  represents a line-based or pixel-based change in vertical position
  since the last scrolling event from a Mighty Mouse scroller or a
  single-wheel mouse scroller. The scrolling data uses a fixed-point
  16.16 signed integer format. If this key is passed to
  `CGEventGetDoubleValueField', the fixed-point value is converted to a
  double value. */
kCGScrollWheelEventFixedPtDeltaAxis1 = 93,

/* Key to access a field that contains scrolling data. The scrolling data
  represents a line-based or pixel-based change in horizontal position
  since the last scrolling event from a Mighty Mouse scroller. The
  scrolling data uses a fixed-point 16.16 signed integer format. If this
  key is passed to `CGEventGetDoubleValueField', the fixed-point value is
  converted to a double value. */
kCGScrollWheelEventFixedPtDeltaAxis2 = 94,

/* This field is not used. */
kCGScrollWheelEventFixedPtDeltaAxis3 = 95,

/* Key to access an integer field that contains pixel-based scrolling
  data. The scrolling data represents the change in vertical position
  since the last scrolling event from a Mighty Mouse scroller or a
  single-wheel mouse scroller. */
kCGScrollWheelEventPointDeltaAxis1 = 96,

/* Key to access an integer field that contains pixel-based scrolling
  data. The scrolling data represents the change in horizontal position
  since the last scrolling event from a Mighty Mouse scroller. */
kCGScrollWheelEventPointDeltaAxis2 = 97,

/* This field is not used. */
kCGScrollWheelEventPointDeltaAxis3 = 98,
 
/*  */
kCGScrollWheelEventScrollPhase = 99,
 
/* rdar://11259169 */
kCGScrollWheelEventScrollCount = 100,
 
kCGScrollWheelEventMomentumPhase = 123,
 
/* Key to access an integer field that indicates whether the event should
  be ignored by the Inkwell subsystem. If the value is non-zero, the
  event should be ignored. */
kCGScrollWheelEventInstantMouser = 14,

/* Key to access an integer field that contains the absolute X coordinate
  in tablet space at full tablet resolution. */
kCGTabletEventPointX = 15,

/* Key to access an integer field that contains the absolute Y coordinate
  in tablet space at full tablet resolution. */
kCGTabletEventPointY = 16,

/* Key to access an integer field that contains the absolute Z coordinate
  in tablet space at full tablet resolution. */
kCGTabletEventPointZ = 17,

/* Key to access an integer field that contains the tablet button state.
  Bit 0 is the first button, and a set bit represents a closed or pressed
  button. Up to 16 buttons are supported. */
kCGTabletEventPointButtons = 18,

/* Key to access a double field that contains the tablet pen pressure. A
  value of 0.0 represents no pressure, and 1.0 represents maximum
  pressure. */
kCGTabletEventPointPressure = 19,

/* Key to access a double field that contains the horizontal tablet pen
  tilt. A value of 0 represents no tilt, and 1 represents maximum tilt. */
kCGTabletEventTiltX = 20,

/* Key to access a double field that contains the vertical tablet pen
  tilt. A value of 0 represents no tilt, and 1 represents maximum tilt. */
kCGTabletEventTiltY = 21,

/* Key to access a double field that contains the tablet pen rotation. */
kCGTabletEventRotation = 22,

/* Key to access a double field that contains the tangential pressure on
  the device. A value of 0.0 represents no pressure, and 1.0 represents
  maximum pressure. */
kCGTabletEventTangentialPressure = 23,

/* Key to access an integer field that contains the system-assigned unique
  device ID. */
kCGTabletEventDeviceID = 24,

/* Key to access an integer field that contains a vendor-specified value. */
kCGTabletEventVendor1 = 25,

/* Key to access an integer field that contains a vendor-specified value. */
kCGTabletEventVendor2 = 26,

/* Key to access an integer field that contains a vendor-specified value. */
kCGTabletEventVendor3 = 27,

/* Key to access an integer field that contains the vendor-defined ID,
  typically the USB vendor ID. */
kCGTabletProximityEventVendorID = 28,

/* Key to access an integer field that contains the vendor-defined tablet
  ID, typically the USB product ID. */
kCGTabletProximityEventTabletID = 29,

/* Key to access an integer field that contains the vendor-defined ID of
  the pointing device. */
kCGTabletProximityEventPointerID = 30,

/* Key to access an integer field that contains the system-assigned device
  ID. */
kCGTabletProximityEventDeviceID = 31,

/* Key to access an integer field that contains the system-assigned unique
  tablet ID. */
kCGTabletProximityEventSystemTabletID = 32,

/* Key to access an integer field that contains the vendor-assigned
  pointer type. */
kCGTabletProximityEventVendorPointerType = 33,

/* Key to access an integer field that contains the vendor-defined pointer
  serial number. */
kCGTabletProximityEventVendorPointerSerialNumber = 34,

/* Key to access an integer field that contains the vendor-defined unique
  ID. */
kCGTabletProximityEventVendorUniqueID = 35,

/* Key to access an integer field that contains the device capabilities
  mask. */
kCGTabletProximityEventCapabilityMask = 36,

/* Key to access an integer field that contains the pointer type. */
kCGTabletProximityEventPointerType = 37,

/* Key to access an integer field that indicates whether the pen is in
  proximity to the tablet. The value is non-zero if the pen is in
  proximity to the tablet and zero when leaving the tablet. */
kCGTabletProximityEventEnterProximity = 38,

/* Key to access a field that contains the event target process serial
  number. The value is a 64-bit value. */
kCGEventTargetProcessSerialNumber = 39,

/* Key to access a field that contains the event target Unix process ID. */
kCGEventTargetUnixProcessID = 40,

/* Key to access a field that contains the event source Unix process ID. */
kCGEventSourceUnixProcessID = 41,

/* Key to access a field that contains the event source user-supplied
  data, up to 64 bits. */
kCGEventSourceUserData = 42,

/* Key to access a field that contains the event source Unix effective
  UID. */
kCGEventSourceUserID = 43,

/* Key to access a field that contains the event source Unix effective
  GID. */
kCGEventSourceGroupID = 44,

/* Key to access a field that contains the event source state ID used to
  create this event. */
kCGEventSourceStateID = 45,
 
/* Key to access an integer field that indicates whether a scrolling event
  contains continuous, pixel-based scrolling data. The value is non-zero
  when the scrolling data is pixel-based and zero when the scrolling data
  is line-based. */
kCGScrollWheelEventIsContinuous = 88,

/* Added in 10.5; made public in 10.7 */
kCGMouseEventWindowUnderMousePointer = 91,
kCGMouseEventWindowUnderMousePointerThatCanHandleThisEvent = 92,

/* Unaccelerated pointer movement */
kCGEventUnacceleratedPointerMovementX = 170,
kCGEventUnacceleratedPointerMovementY = 171
};

/* Constants used with the `kCGMouseEventSubtype' event field. */
typedef CF_ENUM(uint32_t, CGEventMouseSubtype) {
kCGEventMouseSubtypeDefault           = 0,
kCGEventMouseSubtypeTabletPoint       = 1,
kCGEventMouseSubtypeTabletProximity   = 2
};


typedef CF_ENUM(uint32_t, CGEventTapLocation) {
  kCGHIDEventTap = 0,
  kCGSessionEventTap,
  kCGAnnotatedSessionEventTap
};

/* Constants that specify where a new event tap is inserted into the list of
   active event taps. */
typedef CF_ENUM(uint32_t, CGEventTapPlacement) {
  kCGHeadInsertEventTap = 0,
  kCGTailAppendEventTap
};

/* Constants that specify whether a new event tap is an active filter or a
   passive listener. */
typedef CF_ENUM(uint32_t, CGEventTapOptions) {
  kCGEventTapOptionDefault = 0x00000000,
  kCGEventTapOptionListenOnly = 0x00000001
};

/* A mask that identifies the set of Quartz events to be observed in an
   event tap. */
typedef uint64_t CGEventMask;

/* Generate an event mask for a single type of event. */
#define CGEventMaskBit(eventType)       ((CGEventMask)1 << (eventType))

/* An event mask that represents all event types. */
#define kCGEventMaskForAllEvents        (~(CGEventMask)0)

/* An opaque type that represents state within the client application that’s
   associated with an event tap. */
typedef struct __CGEventTapProxy *CGEventTapProxy;

/* A client-supplied callback function that’s invoked whenever an associated
   event tap receives a Quartz event.

   The callback is passed a proxy for the tap, the event type, the incoming
   event, and the user-defined data specified when the event tap was
   created. The function should return the (possibly modified) passed-in
   event, a newly constructed event, or NULL if the event is to be deleted.

   The event passed to the callback is retained by the calling code, and is
   released after the callback returns and the data is passed back to the
   event system. If a different event is returned by the callback function,
   then that event will be released by the calling code along with the
   original event, after the event data has been passed back to the event
   system. */

typedef CGEventRef __nullable (*CGEventTapCallBack)(CGEventTapProxy _Nonnull  proxy,
                                                    CGEventType type, CGEventRef _Nonnull  event, void * __nullable userInfo);


extern CFMachPortRef __nullable CGEventTapCreate(CGEventTapLocation tap, CGEventTapPlacement place, CGEventTapOptions options, CGEventMask eventsOfInterest, CGEventTapCallBack _Nonnull callback, void * __nullable userInfo);
    
#endif


bool CheckAccessibilityPrivileges(void)
{
#if TARGET_OS_OSX
    //We don't care about storing settings in a file right now
    //auto data_dir = env.output_path.c_str();
    //auto no_permission_filename = format("%s/no_permission.txt", data_dir);

    // Only show prompt if file hasn't been created yet.
    CFBooleanRef show_prompt = kCFBooleanTrue;
    //if (fs::exists(no_permission_filename.c_str())) {
    //    show_prompt = kCFBooleanTrue;
    //} else {
    //    show_prompt = kCFBooleanFalse;
    //}

    bool result;
    const void *keys[] = { kAXTrustedCheckOptionPrompt };
    const void *values[] = { show_prompt };
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        sizeof(keys) / sizeof(*keys),
        &kCFCopyStringDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );

    result = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);

    //We don't care about storing settings in a file right now
    /*
    if (!result) {
        // Create a file to remember not to ask next time. This is important when this
        // binary is being scheduled in a daemon
        std::ofstream output(no_permission_filename);
    } else {
        remove(no_permission_filename.c_str());
    }
    */
    
    return result;
#else
    return false;
#endif
}

const char* key_code_to_str(int keyCode, bool shift, bool caps) {
    switch ((int) keyCode) {
            case 0:   return shift || caps ? "A" : "a";
            case 1:   return shift || caps ? "S" : "s";
            case 2:   return shift || caps ? "D" : "d";
            case 3:   return shift || caps ? "F" : "f";
            case 4:   return shift || caps ? "H" : "h";
            case 5:   return shift || caps ? "G" : "g";
            case 6:   return shift || caps ? "Z" : "z";
            case 7:   return shift || caps ? "X" : "x";
            case 8:   return shift || caps ? "C" : "c";
            case 9:   return shift || caps ? "V" : "v";
            case 11:  return shift || caps ? "B" : "b";
            case 12:  return shift || caps ? "Q" : "q";
            case 13:  return shift || caps ? "W" : "w";
            case 14:  return shift || caps ? "E" : "e";
            case 15:  return shift || caps ? "R" : "r";
            case 16:  return shift || caps ? "Y" : "y";
            case 17:  return shift || caps ? "T" : "t";
            case 18:  return shift ? "!" : "1";
            case 19:  return shift ? "@" : "2";
            case 20:  return shift ? "#" : "3";
            case 21:  return shift ? "$" : "4";
            case 22:  return shift ? "^" : "6";
            case 23:  return shift ? "%" : "5";
            case 24:  return shift ? "+" : "=";
            case 25:  return shift ? "(" : "9";
            case 26:  return shift ? "&" : "7";
            case 27:  return shift ? "_" : "-";
            case 28:  return shift ? "*" : "8";
            case 29:  return shift ? ")" : "0";
            case 30:  return shift ? "}" : "]";
            case 31:  return shift || caps ? "O" : "o";
            case 32:  return shift || caps ? "U" : "u";
            case 33:  return shift ? "{" : "[";
            case 34:  return shift || caps ? "I" : "i";
            case 35:  return shift || caps ? "P" : "p";
            case 37:  return shift || caps ? "L" : "l";
            case 38:  return shift || caps ? "J" : "j";
            case 39:  return shift ? "\"" : "'";
            case 40:  return shift || caps ? "K" : "k";
            case 41:  return shift ? ":" : ";";
            case 42:  return shift ? "|" : "\\";
            case 43:  return shift ? "<" : ",";
            case 44:  return shift ? "?" : "/";
            case 45:  return shift || caps ? "N" : "n";
            case 46:  return shift || caps ? "M" : "m";
            case 47:  return shift ? ">" : ".";
            case 50:  return shift ? "~" : "`";
            case 65:  return "[decimal]";
            case 67:  return "[asterisk]";
            case 69:  return "[plus]";
            case 71:  return "[clear]";
            case 75:  return "[divide]";
            case 76:  return "[enter]";
            case 78:  return "[hyphen]";
            case 81:  return "[equals]";
            case 82:  return "0";
            case 83:  return "1";
            case 84:  return "2";
            case 85:  return "3";
            case 86:  return "4";
            case 87:  return "5";
            case 88:  return "6";
            case 89:  return "7";
            case 91:  return "8";
            case 92:  return "9";
            case 36:  return "[return]";
            case 48:  return "[tab]";
            case 49:  return " ";
            case 51:  return "[del]";
            case 53:  return "[esc]";
            case 54:  return "[right-cmd]";
            case 55:  return "[left-cmd]";
            case 56:  return "[left-shift]";
            case 57:  return "[caps]";
            case 58:  return "[left-option]";
            case 59:  return "[left-ctrl]";
            case 60:  return "[right-shift]";
            case 61:  return "[right-option]";
            case 62:  return "[right-ctrl]";
            case 63:  return "[fn]";
            case 64:  return "[f17]";
            case 72:  return "[volup]";
            case 73:  return "[voldown]";
            case 74:  return "[mute]";
            case 79:  return "[f18]";
            case 80:  return "[f19]";
            case 90:  return "[f20]";
            case 96:  return "[f5]";
            case 97:  return "[f6]";
            case 98:  return "[f7]";
            case 99:  return "[f3]";
            case 100: return "[f8]";
            case 101: return "[f9]";
            case 103: return "[f11]";
            case 105: return "[f13]";
            case 106: return "[f16]";
            case 107: return "[f14]";
            case 109: return "[f10]";
            case 111: return "[f12]";
            case 113: return "[f15]";
            case 114: return "[help]";
            case 115: return "[home]";
            case 116: return "[pgup]";
            case 117: return "[fwddel]";
            case 118: return "[f4]";
            case 119: return "[end]";
            case 120: return "[f2]";
            case 121: return "[pgdown]";
            case 122: return "[f1]";
            case 123: return "[left]";
            case 124: return "[right]";
            case 125: return "[down]";
            case 126: return "[up]";
        }
        return "[unknown]";
}

#pragma mark -- CFNotification Center Notifications

void mainWindowChangedNotificationCallback(CFNotificationCenterRef center, void * observer, CFStringRef name, const void * object, CFDictionaryRef userInfo) {
    
    printf("\nMain Window Changed!\n");
    
    CFShow(CFSTR("Received notification (dictionary): "));
    CFShow(name);
    assert(object);
    assert(userInfo);
    // print out user info
    const void * keys;
    const void * values;
    CFDictionaryGetKeysAndValues(userInfo, &keys, &values);
    
    CFStringRef* cfKeys = (CFStringRef*)keys;
    CFStringRef* cfValues = (CFStringRef*)values;

    for (int i = 0; i < CFDictionaryGetCount(userInfo); i++) {
        const char * keyStr = CFStringGetCStringPtr((CFStringRef)&cfKeys[i], CFStringGetSystemEncoding());
        const char * valStr = CFStringGetCStringPtr((CFStringRef)&cfValues[i], CFStringGetSystemEncoding());
        printf("\t\t \"%s\" = \"%s\"\n", keyStr, valStr);
    }
    
}

void notificationCallback(CFNotificationCenterRef center, void * observer, CFStringRef name, const void * object, CFDictionaryRef userInfo) {
    
    printf("\nnotification callback\n");
    /*
     CFShow(CFSTR("Received notification (dictionary):"));
     // print out user info
     const void * keys;
     const void * values;
     CFDictionaryGetKeysAndValues(userInfo, &keys, &values);
     for (int i = 0; i < CFDictionaryGetCount(userInfo); i++) {
     const char * keyStr = CFStringGetCStringPtr((CFStringRef)&keys[i], CFStringGetSystemEncoding());
     const char * valStr = CFStringGetCStringPtr((CFStringRef)&values[i], CFStringGetSystemEncoding());
     printf("\t\t \"%s\" = \"%s\"\n", keyStr, valStr);
     }
     */
}

static void RegisterNotificationObservers()
{
    CFNotificationCenterRef center = CFNotificationCenterGetLocalCenter();
    assert(center);
    
    // add an observer
    CFNotificationCenterAddObserver(center, NULL, notificationCallback,
                                    CFSTR("NSApplicationDidBecomeActiveNotification"), NULL,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);
    
    CFNotificationCenterAddObserver(center, NULL, notificationCallback,
                                    CFSTR("NSApplicationDidResignActiveNotification"), NULL,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);
    
    CFNotificationCenterAddObserver(center, NULL, mainWindowChangedNotificationCallback,
                                    CFSTR("CGWindowDidBecomeMainNotification"), NULL,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);
    
    /*
     // post a notification
     CFDictionaryKeyCallBacks keyCallbacks = {0, NULL, NULL, CFCopyDescription, CFEqual, NULL};
     CFDictionaryValueCallBacks valueCallbacks  = {0, NULL, NULL, CFCopyDescription, CFEqual};
     CFMutableDictionaryRef dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 1,
     &keyCallbacks, &valueCallbacks);
     CFDictionaryAddValue(dictionary, CFSTR("TestKey"), CFSTR("TestValue"));
     CFNotificationCenterPostNotification(center, CFSTR("MyNotification"), NULL, dictionary, TRUE);
     CFRelease(dictionary);
     */
    
    // remove oberver
    //CFNotificationCenterRemoveObserver(center, NULL, CFSTR("TestValue"), NULL);
}


#elif defined (_WIN32)


#endif


#endif /* CT_DEVICE_INPUT */
