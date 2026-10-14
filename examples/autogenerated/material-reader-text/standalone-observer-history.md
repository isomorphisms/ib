# Standalone observer is historical

The app_process observer assembled and received exact candidate bytes but did
not pass selection/reflow acceptance. Android selection consults DeviceConfig;
the standalone shell process is not a registered application. Context/Application
initialization experiments exposed attribution, lazy fonts, private class access,
missing application context and finally ActivityManager process registration.

`FailedStandaloneApplicationObserver.smali` preserves the last unaccepted setup
attempt. It is not the active acceptance path and must not be promoted to PASS.
The original diagnostic and failed artifacts remain historical evidence.

Current qualification is android-NDK's separate Android Instrumentation observer
and source-generated target APK, followed by the actual IB Activity UI. Normal
Android binding supplies Application, UID, provider registration and fonts. No
provider permission, native result or candidate behavior is replaced.
