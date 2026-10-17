Follow these steps to wire, compile, and play the game on your Arduino setup using the Arduino IDE.

1. Hardware Connections (Pin Mapping)

Connect your components to the Arduino board exactly as listed below:
• SSD1306 OLED Display: Connect SDA to A4 and SCL to A5 (Standard I2C pins for Arduino Uno/Nano).
• Joystick X-Axis: Connect to Analog Pin A0 (Controls car steering).
• Joystick Y-Axis: Connect to Analog Pin A1 (Controls shop navigation).
• Joystick Switch (SW): Connect to Digital Pin 5 (Toggles PDA menu).
• BTN_CYCLE (Button 1): Connect to Digital Pin 2 (Cycles weapons / enters Ammu-Nation from PDA).
• BTN_SHOOT (Button 2): Connect to Digital Pin 3 (Fires weapon / buys items in shop).
• Passive Buzzer: Connect to Digital Pin 6 (Handles engine sounds, shooting, and alerts).
Connect these to a Arduino/Mega/Uno/Nano or other (If other dont work try modifying the code to port for the other circuit boards)
Note: All buttons use internal pull-up resistors (INPUT_PULLUP). Wire them directly between the specified digital pin and GND (no external resistors needed).

2. Required Libraries

Open the Library Manager in the Arduino IDE (Sketch -> Include Library -> Manage Libraries...) and install the following packages:
1. Adafruit SSD1306
2. Adafruit GFX Library

3. Gameplay & Rules Reference

• Driving: Move the joystick 360° to steer and drive. Your car will accelerate smoothly, and walls on the map will block your progress.
• The 4-Star Heat Scale:
	• 1-2 Stars (Civilian Casualties): Police will attempt a non-lethal arrest. If they touch you, the JAILED! screen triggers, you lose $300, and you respawn at the police station.
	• 3-4 Stars (Attacking Police): Officers upgrade to lethal SWAT units. They will actively drain your HP. If you drop to 0 HP, the BUSTED! screen triggers, you lose $500, and you respawn at the hospital.
• Ammu-Nation Armory: Press the joystick button (SW) to open your PDA. Press BTN1 to enter the weapon shop. Use the joystick Up/Down to scroll through the inventory inside the small UI box, and press BTN2 to purchase a weapon.
Final step (Paste the code from ata.ino)
