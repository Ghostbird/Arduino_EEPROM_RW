/** This code is released by the author into the public domain.

This program can run on an Arduino connected to a T24C02A EEPROM on the I²C bus.
The Arduino offers an interactive shell on the serial interface that can be used to read and write the EEPROM. (Hardware write protection must be removed manually of course)

Designed using an Arduino Leonardo, but should work with any Arduino that has an I²C interface.

This work is based on this datasheet for the T24C02A IC: http://www.ic-jiazhi.com/upload/PDF/T24C02A_E.pdf
Currently writing to pages of T24C04A, T24C08A and T24C16A ICs requires setting the I²C address' three least significant bytes to account for the page number, as detailed in the datasheet.
Other sources for this work are: http://blog.opensecurityresearch.com/2012/10/hacking-usb-webkeys.html
And of course lots of reading  : http://arduino.cc/en/Reference/HomePage mainly for the specifics on the Wire library. It is a fairly abstracted library, and careful reading is required to understand how to send the START, STOP, ACK and NACK messages correctly.
**/
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  //Wait, so the user can open the serial interface.
  delay(4000);
}

void loop() 
{
  //Start interactive shell
  EEPROM_I2C_interactive(EEPROM_I2C_setAddress());
}

//Prompt the user for an address on the EEPROM to read from, and the amount of bytes to read from there.
void EEPROM_I2C_read(char I2C_Address)
{
  Serial.println("Enter EEPROM address (decimal) to read from:");
  while(!Serial.available());
  int address = Serial.parseInt();
  Serial.println("Enter number of bytes to read.");
  while(!Serial.available());
  byte amount = Serial.parseInt();
  Serial.print("Reading ");
  Serial.print(amount);
  Serial.print(" bytes starting at EEPROM address ");
  Serial.print(address);
  Serial.println(":");
  /*
  This part is a bit weird, because the chip only returns up to 32 bytes.
  Thus we try to read [amount-readcount] bytes from EEPROM address [address+readcount] and increment readcount for each received byte.
  The while loop ensures that as long as we haven't received the full amount of bytes, we'll read the next batch of up to 32 bytes.
  */
  
  //Count received bytes in this variable.
  int readcount = 0;
  while (readcount < amount)
  {
    //Begin a transmission to the I2C device.
    Wire.beginTransmission(I2C_Address);
    //Write the address offset by the amount of bytes we've already read.
    Wire.write(address + readcount);
    //Write the buffered transmission, send a restart instead of a stop byte. (see T24C02A datasheet: Random read and Sequential read)
    char result = Wire.endTransmission(false);
    //Check return code, normally reads always succeed.
    if (result > 0)
      //Print human readable error message if applicable.
      EEPROM_I2C_printreturncode(result);
    //Request amount-readcount bytes of data from the I2C device (the EEPROM). It will return up to 32 bytes.
    Wire.requestFrom(int(I2C_Address),amount-readcount);
    //Read the data available on the I2C bus.
    while(Wire.available())
    {
      //Receive a byte, cast it to char, print it.
      Serial.print(char(Wire.read()));
      //Increment readcount, this is important, because the function relies on accurate counting of the received number of bytes.
      readcount++;
    }
  }
  //Write newline and the stats about the read.
  Serial.println();
  Serial.print("Received ");
  Serial.print(readcount);
  Serial.print(" of ");
  Serial.print(amount);
  Serial.println(" bytes");
  //NOTE: Artifacts, error lines may in fact never be reached. Did not check thoroughly.
  //Write error messages if no data was received, or if less than the expected amount of bytes was received.
  if (readcount == 0)
    Serial.println("No bytes received, please check address, connection and power.");
  else if (readcount < amount)
    Serial.println("Received less than target amount of bytes!");  
}

void EEPROM_I2C_write(char I2C_Address)
{
  //Constant for the number of tries. With 10 you're fine, I never had 10 write failures in succession.
  const char NUM_TRIES = 10;
  //Promt user for EEPROM address to start write.
  Serial.println("Enter EEPROM startaddress to write to:");
  //Wait for user input.
  while(!Serial.available());
  int address = Serial.parseInt();
  //Prompt user for string to write to the EEPROM, this is explained to the user fairly well.
  Serial.print("Enter string to write to ");
  Serial.print(address);
  Serial.println(".");
  Serial.println("Any ASCII value can be entered by sending its decimal representation prefixed by an '@' character.");
  Serial.println("To write and actual '@', write @64 since 64 is the ASCII value for the @ character.");
  //Wait for user input.
  while(!Serial.available());
  Serial.print("Writing: ");
  //Write each received byte, and increment the address after the write operation, so the next byte is written to the next address.
  for (; Serial.available(); address++)
  {
    //Grab one byte from the Serial buffer.
    char received = Serial.read();
    //Check whether it's an '@' followed by some data. This is the escape code for decimal input of character values (e.g. for non-printable characters).
    if (received == '@' && Serial.available())
      //Parse the number that follows the '@'. This will mess up if people send an '@' followed by a non-numerical character. The user should be sensible and not do that, otherwise it will write something else than intended.
      received = Serial.parseInt();
    //Initialise [tries] counter to zero.
    char tries = 0;
    //Start with resultcode 4 (other error), loop as long as it isn't zero (success) and [tries] < [NUM_TRIES] constant (see start of function). Increment [tries] for each attempt.
    for (char result = 4; result > 0 && tries < NUM_TRIES; tries++)
    {
      //Begin a transmission to the I²C device (the EEPROM).
      Wire.beginTransmission(I2C_Address);
      //Write the EEPROM address to the I²C bus, this sets the start address of the write operation.
      Wire.write(address);
      //Write the byte grabbed from the I²C bus.
      Wire.write(received);
      //Write the byte back to the serial line as well, to visually confirm to the user that it has been written.
      Serial.print(received);
      //Execute the buffered I²C transmission and store the result code.
      result = Wire.endTransmission();
      //Check for error codes (0 is success, higher is error, lower should not exist).
      if (result > 0)
        //Print error message in human readable format.
        EEPROM_I2C_printreturncode(result);
      //Note: If the returned error code is not zero at this point, The enclosing for loop will cause a retransmission attempt up to NUM_TRIES times.
    }
    //If the maximum amount of tries has been reached, the write has not succeeded.
    if (tries == NUM_TRIES)
    {
      //Notify the user of the failed write.
      Serial.print(int(NUM_TRIES));
      Serial.print(" failed attempts to write ");
      Serial.print(received);
      Serial.print(" to address ");
      Serial.println(address);
      //NOTE: The function does not abort at this point, it will acknowledge the failure to write to this address, and continue with the next address and the next string character.
    }
  }
  //Print a newline to ensure that the next message is not printed on the same line as the function output.
  Serial.println();
}

char EEPROM_I2C_setAddress()
{
  while(true)
  {
    //Prompt the user for input. Either an address or a P to poll the I²C bus.
    Serial.println("Enter I2C 7-bit address of the EEPROM in decimal:");
    Serial.println("Enter P to poll the entire I2C bus for compatible devices:");
    //Wait for user input.
    while(!Serial.available());
    //If the data in the Serial buffer is the carachter 'P', run the poll function.
    if (Serial.peek() == 'P')
    {
      //Read the 'P' out of the buffer, otherwise the program will continue polling perpetually.
      Serial.read();
      EEPROM_I2C_pollbus();
    }
    else //No characer 'P' is in the Serial buffer, read the data as an integer address.
    {
      //Parse integer address from the Serial.
      int address = Serial.parseInt();
      //Check whether the address is in the valid range.
      if (address >= 0 && address < 128)
      {
        //Show the user that the address has been set.
        Serial.print("I2C address of EEPROM set to: ");
        Serial.println(address);
        //Return the address.
        return address;
      }
      //Show error message to the user.
      Serial.println("Invalid address. Address must be between 0 and 127 inclusive.");
    }
  }
}
  
void EEPROM_I2C_interactive(char address)
{
  char I2C_Address = address;
  //Loop forever
  while(true)
  {
    //Prompt the user for an action. (No Oxford comma there, I'm normally quite fond of it, but I think the text is unambiguous enough, and the extra comma looks weird.
    Serial.println("Enter P, R, S or W.");
    Serial.println("S - Set I2C 7-bit address of EEPROM");
    Serial.println("R - Read data from EEPROM");
    Serial.println("W - Write data to EEPROM");
    Serial.println("P - Poll the I2C bus.");
    //Wait for user input.
    while(!Serial.available());
    //Retrieve a single command character from the serial buffer.
    //Note: Since only one character is read, users can queue subsequent commands.
    //For example: W24Test@255@    will write to address 24 of the EEPROM, the string "Test" followed by ASCII character 255, followed by an actual '@' character.
    //             R11 17          will read seventeen bytes starting from EEPROM address 11.
    char command = Serial.read();
    if (command == 'W')
      //Start the write function if the command character was a 'W'.
      EEPROM_I2C_write(I2C_Address);
    else if (command == 'R')
      //Start the read function if the command character was an 'R'.
      EEPROM_I2C_read(I2C_Address);
    else if (command == 'S')
      //Start the setAddress funtion if the command character was an 'S'.
      I2C_Address = EEPROM_I2C_setAddress();
    else if (command == 'P')
      //Start the function that polls the I2C bus.
      EEPROM_I2C_pollbus();
    else
    {
      //Print an error message for any other command character.
      Serial.print("Unknown command: "); 
      Serial.println(command);
    }
  }
}

void EEPROM_I2C_pollbus()
{
  //Loop trough the available 7-bit addresses of the I²C bus.
  for (int address = 0; address < 128; address++)
  {
    //Print the address to the user.
    //Request up to 32 characters from this address on the I²C bus. 
    Wire.requestFrom(address, 32);
    //If there's an answer, tell the user so, and read the answer.
    if (Wire.available())
    {
      Serial.print("Answer from ");
      Serial.print(address);
      Serial.print(": ");
      //Try to read up to 32 characters from the I²C bus.
      for (byte readcount = 0; Wire.available() && readcount < 32; readcount++)
        //Print each received byte as a character.
        Serial.print(char(Wire.read()));
      //Print a newline.
      Serial.println();
    }
  }
  //Note: The user can now skim the output and decide where on the bus the device is located, by looking for addresses where data was received.
}

void EEPROM_I2C_printreturncode(char code)
{
  String returncodes[5] = {"SUCCESS","DATA TOO LONG","NACKED ADDRESS", "NACKED DATA"};
  //Print a newline character to ensure that the error isn't lost at the end of a long line of text.
  Serial.println();
  //Print the returncode from Wire.endTransmission in a human readable format.
  Serial.println(returncodes[code]);
}
