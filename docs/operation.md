# Highlevel overview

## At the PC

Plugging the SimpleAlt into a USB connector will result in three things happening.
1. The altimeter will automatically power on.
2. The battery will start charging.
3. The altimeter should enumerate as a USB mass storage device to allow transfer of flight data from the built in flash.

Ideally (I don't know how hard this is) we would be able to provide the data as seperate files, 1 per flight (a flight being a full power on - landing cycle as described in the "In the Field" section. Files would ideally be available as CSV, but ideally stored in flash as raw binary data (faster to write and less space). I don't know how feasible a "realtime" conversion from Binary to CSV is. Binary data dump is probably OK and certainly acceptable initially, just needs more transforming on the PC which is not a big deal. Benefit of CSV output is it allows the user to dump data directly into Excel which will have a wider appeal than a custom application.

If, prior to plugging in the altimeter into the USB port, the device is in power down mode and pins 1 (3V3) & 2 (BOOT) of the debug header are shorted together, then the altimeter will start up in DFU mode. Because this requires working software (i.e. the device needs to be able to power down which it won't necessarily do if the software does not implement this), it is advised to perform development without the battery installed. Alternatively it should be possible to force a reset by using SWD which will cause the boot pin state to be read.

For the file format that the user sees I see there are two options (there may be more, I'm open to suggestions). This depends mostly on whether we can do on the fly CSV creation in Mass Storage mode. Initial development should focus on Raw data.

### Raw data

Binary dump. PC app can calculate real altitude, velocity, acceleration, etc.

### On the fly CSV mode

CSV file with Time, Flags, Temperature, RAW Barometer Data, Calculated Altitude.

## In the Field

In the field user operation is intended to be as follows.
1. Assuming the device is in power down mode, the user will push the power button to apply power to the altimeter. The altimeter must immediately assert the "Stay Alive" pin to ensure that once the user releases the power button, power remains on. Here is where we should take a temperature measurement from the BMP280.
2. If there is record of a previous flight, the altimeter should flash out the altitude, e.g. if the previous flight was 132m, the altimeter should flash .-...-.. where a '.' represents a short (0.5s?) flash followed by a short delay (0.5s?) and '-' represents a longer delay (1s?).
3. There will be some preconfigured (possibly constant, initially assumed to be aproximately 1 minute) time during which it is intended that the user is able to install the altimeter in the rocket. During this time the altimeter should flash occasionally e.g. A single 0.5s flash every 4s.
4. At this time we are waiting for launch and should be sampling the barometer as quickly as is feasible, >20Hz into a circular buffer. 
5. If we detect an increase in altitude corresponding to ~10m over a 0.5s period we can assume a launch. We will need to look backwards in the ring buffer to determine what the "ground level" pressure was. We should record a "launch" event.
6. We should record the barometric pressure in it's raw form to the flash. How much is buffered between writes will need to be determined based on available RAM, expected flight duration, etc.
7. When the pressure starts to decrease we should record an "apogee" event. It is possible to reduce the sampling speed of the barometer now if it's helpful, though this is not a requirement.
8. When we detect that the pressure has stopped changing (i.e. < 1m/second) for ~3 seconds we can be confident we're on the ground so can record a "landed" event.
9. We should now flash out the apogee height using the previously mentioned method for ~3 minutes with an ~10 second delay between reports. The intent is that this should be enough time for the user to find the rocket and view their flight altitude, but not so long that it uses excessive battery power, or prevents the user "immediately" reloading and reusing the rocket. There might be a better way to achieve this, initial logging will be useful to help determine.
10. It's time to power down. Ensure everything is in a good state, and put the "Stay Alive" pin low. Very shortly after power will be lost so all state will be reset ready for the next launch.

Ideally the flash would be arranged into a sort of ring buffer itself, where each flight would be recorded, and each next flight would create a new entry. If there is insufficient space for a new flight then the oldest flight is erased at this point to allow the new flight to be recorded.

If we use a constant sampling rate then timing is easy to reconstruct on a PC later. If it's variable we'll need to include the sampling rate (or indicating flag) somehow into the data.

It is assumed that we'll store raw barometer data and calculate the apogee after landing, just prior to displaying it to the user. In order to turn pressure into altitude we need to know the bulk air temperature (not the temperature inside the rocket which can be higher due to solar heating). That's why it should be measured at power up when we know that the user has pressed the button. They may be artificially heating the sensor, but it's the best we can do (I think, possibly we could look for longer and use a minimum measurement?).

It is possible to measure battery voltage, I don't know what we want to do with this information at this stage. It's possible not too useful unless it's below 3.2V in which case we could basically "play dead" in order to protect the cell, by refusing to hold the "Stay Alive" pin high. We might want to indicate this to the user with some sort if LED indication before de-asserting the pin. 




