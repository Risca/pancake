Texas Instruments, Inc.

TIMAC-CC2530 Release Notes

---------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------

Version 1.4.0
October 06, 2011


Notices:
 - TIMAC-CC2530-1.4.0 has achieved certification from National Technical
   Systems for conformance to the ZigBee Alliance IEEE 802.15.4 Test Plan.

 - The library files have been built and tested with EW8051 version 8.10.1
   (8.10.1.40194) and may not work with other versions of the IAR tools.
   You can obtain the 8.10 installer and patches from the IAR website.

 - TIMAC has been built and tested with IAR's CLIB library, which provides a
   light-weight C library which does not support Embedded C++. Use of DLIB
   is not recommended since TIMAC is not tested with that library.

 - When programming a device for the first time with this release, select the
   "Erase Flash" in the "Debugger->Texas Instruments->Download" tab in the
   project options. 


Changes:
 - [3964] Added an MSA sample application to support the CC2531DK_Dongle.
   See the "MAC User's Guide - CC2531DK" for programming/usage details.

 - [3872] Modified MAC_McpsDataReq() to add support for ZigBee Green Power.
   Refer to section 4.3.1 of the "802.15.4 MAC API" document for details.

 - [3816] Enhanced MAC control of sleep mode timing and optimized elapsed
   timer computations to reduce power consumption of sleeping devices.

 - [3809] Changed the CCA threshold to -70dBm for configurations that include
   the CC2590 or CC2591 PA/LNA support.

 - [3799] Updated the macRadioDefsTxPwrCC2590[] table to reflect the latest
   characterized values for CC2590 TX power, ranging from -15 to +11 dBm.

 - [3776] Added the osal_osal_msg_push_front() function to push a command
   message to the head of the OSAL queue. Useful when an application uses the
   osal_message_receive() function to pull an OSAL message from the queue,
   decides that it can't be processed, and needs to put it back in the queue.

 - [3710] Added the osal_self() function that returns the current OSAL task ID.
   Useful when an application has multiple endpoints that share code that need
   to set an event timer (requires task ID). Refer to the "OSAL API" for details.

 - [3538] Updated the macRadioDefsTxPwrBare[] table to provide characterized
   values for CC2533 TX power, ranging from -21 to +4 dBm.

 - [3437] Added capability to control the "OSAL task processing loop" from an
   external process. A new function, osal_run_system(), runs one pass of the
   OSAL task processor, called from the forever loop in osal_start_system().

 - [3232] Added support for Enhanced Beacon Request, specified for ZigBee IP
   usage. See the "802.15.4 MAC API" document for details.

 - [3180] Added support for 802.15.4 MAC 2006 security. The MSA sample
   application now provides secure and non-secure configurations. Refer to
   the "802.15.4 MAC API" document for details.

 - [1367] Modified the OSAL Timer system to support 32-bit timers, increasing
   timer delays from ~65 seconds to ~8 days. The halSleep() function has also
   been updated to support 32-bit sleep times. NOTE: this upgrade resulted in
   API changes in the OSAL_Timers.h and hal_sleep.h files.


Bug Fixes:
 - [3978] Fixed a problem where the Pending bit could be set in the ACK frame
   for Association Request.

 - [3883] Fixed a problem with the LED blink logic where a lit LED would not
   blink after a call to HalLedSet() the HAL_LED_MODE_BLINK attribute.

 - [3830] Corrected a possible problem race condition in processing the MAC
   backoff timer trigger callback. This could result in timer calls being preempted
   by the FIFOP interrupt, possibly causing timer realignment in beacon-mode.

 - [3654] Fixed a rare problem in the MAC where a Timer2 overflow compare
   interrupt could be missed. This could cause an occasional missed beacon in
   a beacon-enabled network.

 - [3626] Fixed a rare problem in osalTimeUpdate() where an interrupt during
   the call to macMcuPrecisionCount() could return a corrupt value, causing the
   OSAL_Clock and OSAL_Timers to use an invalid elapsed milliseconds value.
   
 - [3624] Fixed a problem where a device, with busy radio traffic, might waste
   power by not sleeping when the next scheduled event timer was long.

 - [3569] Fixed a problem where the OSAL timer would not be updated after
   sleep if T2CTRL.SYNC was disabled. NOTE: this is not the out-of-box setting.

 - [3567] Fixed a problem where an incorrect radio channel was selected when
   a call to SystemResetSoft() followed setting of a channel with ZMacSetReq().

 - [3564] Fixed a possible problem where a failed attempt to build a MAC frame
   would not be terminated properly, causing a mal-formed frame.


Known Issues:
 - [1441] Device may not track beacon if beacon payload is too long.

 - Battery life extension for beacon-enabled networks is not supported.

 - GTS is not supported.

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
