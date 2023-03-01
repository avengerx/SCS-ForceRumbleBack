# The Force Rumbleback SCS games Plugin

The idea behind this plugin is to provide an immersive experience with vibration-only steering wheel game controllers.

The way the game currently implements force feedback is -- at least to my device -- very strange. The effects played to the vibration controller do not seem to reflect the situation in game.

This plug-in should properly implement dedicated support for "rumble" forcefeedback devices by limiting the effects to what it should have been in a real truck's cabin. Things like revving up, having the engine "tremble" on low RPM, and the uneven effect while killing or firing up the engine.

At this point it's not clear how outside "bumps" should reflect in this plugin's effects. Currently with SCS implementation, I can not tell when a bump is hit, as the only thing the controller can do is vibrate; such events would need an effect different than just vibration, so it will probably be left out of the plug in.
