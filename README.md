_This manual is released by the author into the public domain._

#T24C02A EEPROM set-up:
- Connect Arduino GND to EEPROM GND.
- Connect Arduino SDA to EEPROM SDA.
- Connect Arduino SCL to EEPROM SCL.
- Connect Arduino 5V  to EEPROM Vcc.

#Arduino set-up:
- Open the Arduino IDE.
- Connect Arduino (I used USB).
- Choose correct board and serial port in Arduino IDE.
- Upload code to Arduino using the IDE.
- Open serial console (Ctrl+Shift+M).
- The board waits 4 seconds after Arduino reset (end of upload).
- Then it promps for I²C address.
- If you miss this prompt, just enter the address.
- If you don't know the address, enter the character 'P'.

#Getting started:
If you opened the serial console soon enough, you'll be prompted for an I²C address or a 'P'.

If you did not open the serial console within four seconds after the Arduino reset, you will not see the prompt, but the Arduino will still patiently wait for your input.

Enter the address, or the character 'P'.

##The address:
__The Arduino wire library works with 7-bit adresses.__

The Arduino Serial library can easily parse decimal values, not hex codes.

Therefore the default T24C02A address 0xA0 is rendered thus:

Shift right one bit to get 7-bit address: 0xA0 >> 1 → 0x50

Convert to decimal: 0x50 → 5×16 + 0 = 80
##Polling:
If you entered the character 'P', the Arduino will poll all 128 possible addresses on the I²C bus.
It wil show you the address + answer of the devices that answered.

#The main menu:
Once you have successfully entered an address, you will arrive in the main shell menu.

You will be asked for a command char:
    R — Read
    W — Write
    S — Set I²C address _useful if you set it wrong, have multiple EEPROMs or have a T24C04A, T24C08A or T24C16A IC._
    P — Poll the I²C bus.
##Reading:
- Follow the instruction:
  - Enter the start address for the read.
  - Enter the number of bytes to be read.
- The device will print all characters on a single line.
- Followed by a line of statistics.
- Optionally followed by a line indicating an error.
##Writing:
- Follow the instruction:
  - Enter the start address for the write.
  - Enter the string to be written. 
    _Escape numerical ASCII representations of characters by prefixing them with an '@'._
    _@64 is the @ character itself, in case you want to write an actual @._
- The device will print each character submitted to the device.
- The device will print a human readable error message for each unsuccessful write.
- The device will retransmit (including reprint of) the character up to NUM_TRIES times.
    _<NUM_TRIES> is set to 10 by default in the code._
__After ten failed tries the device:__
  - Will print an error message indication.
  - It will skip this address.
  - It will continue to write the next character to the next location in the EEPROM.
##Setting the I²C address:
The same procedure as detailed in the section "Getting Started", see above.
##Polling:
The same procedure as detailed in the section "Getting Started", see above.
   This polling is not done as part of a change of address. Therefore, the device will give information on the polled devices, and return to the menu immediately afterwards.
