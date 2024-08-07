# SimpleAlt
A simple barometer based peak and logging altimeter for model rocketry with USB C charging and data transfer.

## Technical details
SimpleAlt uses a Bosch BMP280 to measure the barometric altitude of the altimeter. When recording is started by pressing the button it takes a reference measurement which it calls "ground altitude". This is subtracted from the maximum altitude used in displaying the altitude achieved, however the raw barometric altitude is what is recorded onto the flash.

The micro controller is an STM32F042, chosen for it's small physical size and built in USB peripheral.

The flash chip is a Windbond W26Q16 which provides 16Mb (2MB) of recording. The current configuration of 5 bytes per sample (a 1 byte label and 32bit altitude) at 50Hz sample rate (this will probably be reduced to 25Hz in the future) provides a bit over 2hrs of continuous recording. 

The 30mAh battery will last approximately 3-4 hours in idle or recording mode, and about 24 hours in sleep mode, hough work is being done to extend this as the micro controller is not currently achieving the expected current draw. The shutdown mode will last approximately forever, however there is a strange bug in the current hardware revision whereby putting the STNS01 into shutdown mode does not fully succeed until the button is released and pressed again to put a load on the LDO. The cause of this is currently unknown. 

Charging, battery protection and the LDO are all supplied by the STNS01.

## Using SimpleAlt
SimpleAlt has a simple user interface consisting of a single button and a single green LED. This allows it to be as light as possible but may take some getting used to.

To show how this works this document describes LED flashes between angled brackets <>. A number between the brackets indicates the number of times the LED will flash. A '.' indicates a short pause and a '-' indicates a long pause. e.g. < 9 . 10 . 6 - > indicates 9 flashes, a short pause, 10 flashes, a short pause and then 6 flashes followed by a long pause. Most flashing sequences will repeat, the exception being the Button mode.

### The Button
All operations are performed with a single press of the button. To change which options you want you simply hold the button down longer. The options available are:
    1 - Wake from Sleep - If the SimpleAlt is in sleep mode, a quick press of the button will wake it up into Idle mode.
    2 - Start/Stop recording - A slightly longer press on the button, until you see the first flash on the green LED < 1 >, will start or stop recording. 
    3 - Enter Sleep mode - holding the button for longer will result in a double flash of the LED after the first flash < 1 . 2 >. Releasing the button now will cause the SimpleAlt to go into sleep mode.
    4 - Enter Shutdown mode - holding the button for longer still will result in a triple flash after the double flash < 1 . 2 . 3 >. Releasing the button now will result in the SimpleAlt going into Shutdown mode. The only way to wake SimpleAlt from Shutdown mode is to plug a power supply into the USB connector.

Thats it! 

### The Green LED

< 1 - >     Idle mode
< 1 . 2 . > Recording mode

The LED is used to provide status and instant altitude information. Depending on the state of the SimpleAlt it will be displaying one of the below bits of information:
    1 - Idle mode - a single flash repeating < 1 - >.
    2 - Recording mode - the led will alternate between 1 flash and 2 flashes with a short delay between them < 1 . 2 . >.
    3 - Button mode - while holding down the button the led will flash once, then twice, then three times to indicate when to release to achieve different functions as described in the button section.
    4 - Altitude mode - After exiting recording mode, so long as there was an altitude gain of at least 1m, the led will flash out the maximum altitude in metres. The number of flashes indicates the digit for each place in the number. Leading zero's are not flashed out, and a zero is otherwise flashed out as 10 flashes. So for example if your rocket got to 207m the SimpleAlt will flash < 2 . 10 . 7 - > and then repeat.

### Charging
Plugging the SimpleAlt into a USB C power supply will result in it charging the battery. While charging the red LED will illuminate. When the battery is full the red LED will turn off. If a fault occured during charging (e.g. the battery voltage never rises high enough) the red LED will flash. This indicates a fault with the hardware or battery and will need to be repaired.

### Putting it all together
A day flying might look like the following. 

Charge the SimpleAlt by plugging it in. When the red LED turns off, it's charged so unplug it and put it into Sleep mode by holding down the button until you see the double flash.

Get to the flying field and get set up. When the rocket is ready to launch a quick press on the button wakes up the SimpleAlt and it starts slowly flashing that it's in Idle mode. Holding the button until the first flash puts the SimpleAlt into recording mode (and also sets the ground level), it starts flashing < 1 . 2 . 1 . 2 > put it into the payload bay and launch your rocket.

On recovery of your rocket press the button until the green LED gives a single flash < 1 > and release to stop recording. The SimpleAlt will now flash out the altitude your rocket achieved in metres.

Hold the button until the double flash to put the altimeter back into sleep mode, or load it straight into the next rocket and start recording again.

Back at a computer use the plug the SimpleAlt into your computer using a USB C cable and use the Simple application to download your flights. The flights are added onto the list in order that they occured and the application also gives you the duration of the flight (from starting to stopping the recording) to help you find the right flights to download. For more details see the app guide.

