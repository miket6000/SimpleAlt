# Highlevel overview

## Hardware must knows.

### The debug header

The debug header is the 5 pin header. It exposes 3V3, Boot, SWDCLK, SWDIO and GND. 

### Getting into DFU mode

In order to enter DFU mode we need a reset or power up to occur. This can be forced by shorting both pins of the shutdown header (the small two pin header beside the debug header). This forces the STNS01 to disable the LDO and all power is lost to the STM32F042. We can re-enable power by plugging in the USB, but first you need to short the boot pin to 3V3 which can be done by shorting pins 1 (3V3) and 2 (Boot) of the debug header. 

### The STNS01 (Magic Chip)

This chip does the charging, battery protection, and provides the LDO for the rest of the system. 
The shutdown pin is useful for killing power to everything. This will maximise the life of the battery but needs the board to be plugged into USB in order to wake it up, so isn't very user friendly.
There is a CEN (Charge ENable) pin on the device. This can disable charging, though I don't think we ever want to do this so this pin should probably just be left high always. There is an internal pullup, but the datasheet recommends pulling it high for reliability.
The !CHG pin is driven by the STNS01. It will go low when charging, and high when charging completes. If there is an error it will pulse this pin at 1Hz and stop charging. We can re-enable charging by pulsing the CEN pin low then high, but this seems unwise.
The charging current is set to ~40mA which is approximately 1C charge for the included battery.

### The BMP280

The barometer. 
The datasheet shows the calculations (in fixed point, thankfully) for correcting the measurements to obtain atmospheric pressure using the built in calibration data. I'm assuming there are several pre-implemented libraries for this device already.

### The Flash

Yuck, I hate flash. 
2MB is available in 256B pages. Datasheet is in docs folder, but again I assume this is well covered by pre-implemented libraries.

### STM32F042G6U6

Main brain. 
Of particular note is PA0 which is connected to the button. PA0 is also the WKUP0 pin which can be used to wake the micro from any sleep mode so options exist there for deep sleep which is easier to wake from than using the STNS01 shutdown pin, however system power draw will be significantly higher. It might not make sense to actually use this mode as it's then impossible to transition to a lower power state, or indicate to the user that they're not in the lowest possible power state. 

It is possible to measure the battery voltage, however to reduce power consumption you will need to enable the voltage divider by putting the !SENSE\_EN pin low first. Being a lithium battery the voltage curve is pretty flat, until it's not. We can react by preemtively putting the STNS01 into shutdown mode earlier than the battery protection circuit would in order to protect the battery. Maybe at 3.2V?

## Desired end product behaviour

### In the Field

In the field user operation is intended to be as follows.
1. Assuming the device is in power down mode, the user will push the power button to wake up the altimeter. 
2. Here is where we should take a temperature measurement from the BMP280. This turns out to not be used in the calculations for competition grade altimeters. This is done manually and recorded in the submission sheet, however having a record of ambient temperature is required for the calculation, so we may as well capture it now.
3. If there is record of a previous flight, the altimeter should flash out the altitude, e.g. if the previous flight was 132m, the altimeter should flash .-...-.. where a '.' represents a short (0.5s?) flash followed by a short delay (0.5s?) and '-' represents a longer delay (1s?).
4. There will be some preconfigured (possibly constant, initially assumed to be aproximately 1 minute) time during which it is intended that the user is able to install the altimeter in the rocket. During this time the altimeter should flash occasionally e.g. A single 0.5s flash every 4s.
5. After the delay timer has expired, we are waiting for launch and should start sampling the barometer quickly (>20Hz) into a circular buffer.
6. If we detect an increase in altitude corresponding to ~10m (70Pa) over a 0.5s period we can assume a launch. We will need to look backwards in the ring buffer to determine what the "ground level" pressure was. We should record a "launch" event.
7. We should record the barometric pressure in it's raw form to the flash. How much is buffered between writes will need to be determined based on available RAM, expected flight duration, etc.
8. When the pressure starts to increase again we should record an "apogee" event.
9. When we detect that the pressure has stopped changing (i.e. < 1m/second, or <7Pa/s) for ~3 seconds we can be confident we're on the ground so can record a "landed" event.
10. We should now flash out the apogee height using the previously mentioned method for ~3 minutes with an ~10 second delay between reports. The intent is that this should be enough time for the user to find the rocket and view their flight altitude, but not so long that it uses excessive battery power, or prevents the user "immediately" reloading and reusing the rocket. There might be a better way to achieve this, initial logging will be useful to help determine.
11. It's time to power down. I'm not sure exactly what to do here. Go into some form of low power mode that still allows us to flash the LED every 10s or so, to indicate that we're not in power off mode? Screw it and go into proper power off mode and require the user to plug the logger into USB again? Neither are perfect. If we do go into a "sleep" mode, we'll want to fall back into proper power off mode after some period (an hour maybe?) of inactivity anyway to save the battery. It'll depend on power usage.

Ideally the flash would be arranged into a sort of ring buffer itself, where each flight would be recorded, and each next flight would create a new entry. If there is insufficient space for a new flight then the oldest flight is erased at this point to allow the new flight to be recorded.

I'm assuming that we'll store raw barometer data and calculate the apogee after landing, just prior to displaying it to the user. That said, if we're waiting for the next reading from the BMP280 anyway, we may as well calculate the altitude for each sample? 

The NAR (National Association of Rocketry) in the US expects all altimeters to use the standard model of atmosphere which is the one described above (see wikipedia for more information). This model assumes an air temperature of 15C which they expect the altimeter user to manually adjust for using a linear adjustment from absolute zero. For the sake of simplicity, it's probably worth us doing the same. 

#### Button behaviour

Good question.
Short press = wake up please and show me the last peak altitude reading (I want to fly, or see what my last height was).
Long press = go to sleep please, whatever state you were in, forget it (This flight is a scrub, or I just wanted to know what the last flight was again).
Very long press = power off and don't come back (I'm done for the day and going home).
?

#### LED behaviour

Flashing out altitude - discussed above.
"Pre-flight delay" = occasional flash           .--------.--------?
"Armed and waiting for launch" = double blink   .-.---.-.---.-.---?
"Snoozing" = very occasional flash              .-----------------?

### At the PC

Plugging the SimpleAlt into a USB connector should result in the folowing things happening..
1. The battery will start charging (this is automatic)
2. The altimeter should enumerate as a USB mass storage device to allow transfer of flight data from the built in flash.

Ideally (I don't know how hard this is) we would be able to provide the data as seperate files, 1 per flight (a flight being a full power on - landing cycle as described in the "In the Field" section. Files would ideally be available as CSV, but ideally stored in flash as raw binary data (faster to write and less space). I don't know how feasible a "realtime" conversion from Binary to CSV is. Binary data dump is probably OK and certainly acceptable initially, just needs more transforming on the PC which is not a big deal. Benefit of CSV output is it allows the user to dump data directly into Excel which will have a wider appeal than a custom application.


