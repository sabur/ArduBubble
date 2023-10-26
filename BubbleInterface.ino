// Define Bi-Directional Data Bus (Without Pin 9, which is used for Clock)
#define ad0 11
#define ad1 10
#define ad2 8
#define ad3 7
#define ad4 6
#define ad5 5
#define ad6 4
#define ad7 3
#define adp 2 // Odd Parity

#define ClockOut 9 // Clock Output Pin
#define OcrInt 1   // 4 Mhz Clock Pulse

// Control Pins (Using Analog In)
#define rs A0 // Reset: Low Enables Reset
#define cd 12 // A0: High Selects Command/Status, Low Selects Data Register
#define rd A1 // Read: Low Enables BMC Output to Data Bus
#define wr A2 // Write: Low Enables Data Bus Input to BMC

// Base definitions for Command Register, Register Address Counter, and Status Register access (A0 = 1)
#define cmdr 0b00010000 // 0b0001CCCC

// Command Codes
#define wBootloopRegMasked  0b0000
#define initialize          0b0001
#define rBubbleData         0b0010
#define wBubbleData         0b0011
#define rSeek               0b0100
#define rBootloopReg        0b0101
#define wBootloopReg        0b0110
#define wBootloop           0b0111
#define rFsaStatus          0b1000
#define abortCmd            0b1001
#define wSeek               0b1010
#define rBootloop           0b1011
#define rCorrectedData      0b1100
#define resetFifo           0b1101
#define mbmPurge            0b1110
#define softwareReset       0b1111

// User-Accessible Registers (A0 = 0)
#define rwUtility        0b1010
#define wBlockLengthLsb  0b1011
#define wBlockLengthMsb  0b1100
#define wEnable          0b1101
#define rwAddressLsb     0b1110
#define rwAddressMsb     0b1111
#define rwFifoDataBuffer 0b0000

// User-Accessible Register Masks
#define nMbmSelectArMsb    0b01111000 // MBM Select from rwAddressMsb XAAAASSS
#define nStartAddressArMsb 0b00000111 // Start Address within each MBM from rwAddressMsb XAAAASSS

// Status Register Masks
#define fifoReady           0b00000001
#define parityError         0b00000010
#define uncorrectableError  0b00000100
#define correctableError    0b00001000
#define timingError         0b00010000
#define opFail              0b00100000
#define opComplete          0b01000000
#define busy                0b10000000

// FSA Status Register Masks
#define fsaUncorrectableError 0b00000001
#define fsaCorrectableError   0b00000010
#define fsaTimingError        0b00000100
#define errorCorrectionEnable 0b00001000
#define fifoFull              0b00010000
#define fifoEmpty             0b00100000
#define SpareTwo              0b01000000
#define SpareOne              0b10000000

// Enable Register Masks
#define interruptEnable       0b00000001
#define interruptEnableError  0b00000010
#define dmaEnable             0b00000100
#define maxFsaBmcTransferRate 0b00001000
#define writeBootloopEnable   0b00010000
#define enableRcd             0b00100000 // Read Corrected Data
#define enableIcd             0b01000000 // Internally Correct Data
#define enableParityInterrupt 0b10000000

// Buffer for reading and writing to the FIFO using single page requests
// Also used for FIFO operations
// 2 FSA Channels, 1 MBM
// 2048 Pages, 0x0000-0x07FF
// 64 Bytes + 4 Bytes Error Correction
byte rwBuffer[68];
bool rwParityBuffer[68];

/* Good Seeds
  byte seed_6ADH3_8133[40] = {
  0xFF, 0xFE, 0xFF, 0xFE, 0xFD, 0x7D, 0xCE, 0xFF,
  0xFF, 0xFF, 0xEE, 0xFF, 0xCF, 0xBF, 0xFF, 0xFF,
  0xFF, 0xBF, 0xFD, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF,
  0xEF, 0x6F, 0xFA, 0xEE, 0xFF, 0x6E, 0xE6, 0xFE,
  0x7F, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xDF
  };

// 4EC3 8021
// 77 77 57 77 77 77 77 77 wwWwwwww
// F7 FF FF FB FF FF FF FF ........
// FF FF FF BF FF FF FF FF ........
// FF FF FF FF F9 FF FF DD ........
// FF FF FF FF FF FF FF FF ........

  byte seed_4EC3_8021[40] = {
  0x77, 0x77, 0x57, 0x77, 0x77, 0x77, 0x77, 0x77,
  0xF7, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xDD,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  byte seed_A94R_8152[40] = {
  0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xF5,
  0xFF, 0xFF, 0xBA, 0xFF, 0xFF, 0xCF, 0x7F, 0xFF,
  0xDF, 0x7D, 0xFF, 0xF4, 0xFF, 0xFF, 0xEF, 0xEF,
  0xF9, 0xDF, 0xFF, 0xFF, 0xEF, 0xFE, 0xFF, 0xFF,
  0xFF, 0xFF, 0xF7, 0x7F, 0xFF, 0xDF, 0xFF, 0xFF
  };

  byte seed_B84S_8214[40] = {
  0xFF, 0xFF, 0xFB, 0xFD, 0xFF, 0xFF, 0xFF, 0xF7,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xEF,
  0xBF, 0xFF, 0xF5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xEE, 0xFF, 0xFF,
  0xBE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF
  };

  byte seed_9ADJ5_8135[40] = {
  0x77, 0x7F, 0xFF, 0x7D, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xEE, 0x7F, 0xF7, 0xFF, 0xBF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFB, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF
  };

  byte seed_1MZ8_8129[40] = {
  0xEB, 0xEE, 0xB5, 0xDF, 0xFF, 0xFF, 0xFD, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xDF, 0xFF, 0xFF,
  0xFF, 0x77, 0xFF, 0xFF, 0xFF, 0xFF, 0xDE, 0xF7,
  0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF
  };
*/

void setup() {
	// Serial.begin(115200); // Max speed for the Arduino Due
	Serial.begin(2000000); // Max Speed for the Arduino Duemilanove
	Serial.println("Starting...");

	pinMode(rs, OUTPUT);
	pinMode(cd, OUTPUT);
	pinMode(rd, OUTPUT);
	pinMode(wr, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);

	// Start 4Mhz Clock
	/* This is the 4Mhz Clock on Pin 7 for the Arduino Due
	  int32_t mask_PWM_pin = digitalPinToBitMask(7);
	  REG_PMC_PCER1 = 1 << 4;                         // activate clock for PWM controller
	  REG_PIOC_PDR |= mask_PWM_pin;                   // activate peripheral functions for pin (disables all PIO functionality)
	  REG_PIOC_ABSR |= mask_PWM_pin;                  // choose peripheral option B
	  REG_PWM_CLK = 0;                                // choose clock rate, 0 -> full MCLK as reference 84MHz
	  REG_PWM_CMR6 = 0 << 9;                          // select clock and polarity for PWM channel (pin7) -> (CPOL = 0)
	  REG_PWM_CPRD6 = 21;                             // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=10 -> 8.4MHz
	  REG_PWM_CDTY6 = 5;                              // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 10/5 = 50%
	  REG_PWM_ENA = 1 << 6;                           // enable PWM on PWM channel (pin 7 = PWML6) */

	  /* This is the 4Mhz Clock on Pin 9 for the Arduino Duemilanove */
	pinMode(ClockOut, OUTPUT);
	TCCR1A = bit(COM1A0);
	TCCR1B = bit(WGM12) | bit(CS10);
	OCR1A = OcrInt;

	// Run Bus Initialization
	softReset(true, 0);

	delay(200);

	Serial.println("Ready.");
}

void loop() {
	if (Serial.available() > 0) {
		byte command = Serial.read();
		byte parameter;

		if (Serial.available() > 0) {
			parameter = Serial.read();
		}

		if (command == 'i')
		{
			processInitBmc(parameter);
		}
		else if (command == 'c')
		{
			processCommand(parameter);
		}
		else if (command == 'r')
		{
			Serial.println("Reset...");
			softReset(false, parameter);
		}
		else if (command == 's')
		{
			rStatusRegister(readbyte(true));
		}
		else if (command == 'f')
		{
			processFifo(parameter);
		}
		else if (command == 'a')
		{
			// rAllRegisterValues();
		}
		else if (command == 'u')
		{
			// processUserRegister(parameter);
		}

		Serial.println("Ready.");
	}
}

void processInitBmc(byte initByte) {
	Serial.println("Running.");
	digitalWrite(rs, HIGH);
	digitalWrite(LED_BUILTIN, HIGH);
	delayMicroseconds(200);

	// Initialize BMC
	// Set initial RAC Value
	wRegisterAddressCounter(wBlockLengthLsb, true);

	if (initByte == '0') {
		Serial.println("Loading normal parametric registers...");

		// Write Block Length information
		// N=# of FSA Channels, P=# of Pages Transfered
		// MSB -> NNNNXPPP PPPPPPPP <- LSB
		writebyte(false, 0b00000001); // wBlockLengthLsb (1 Page Transferred)
		writebyte(false, 0b00010000); // wBlockLengthMsb (2 FSA Channels)

		// Set Enable Register
		writebyte(false, 0); // wEnable
	}
	else if (initByte == '1') {
		Serial.println("Loading diagnostic (1 FSA) parametric registers...");

		// Write Block Length information
		// N=# of FSA Channels, P=# of Pages Transfered
		// MSB -> NNNNXPPP PPPPPPPP <- LSB
		writebyte(false, 0b00000001); // wBlockLengthLsb (1 Page Transferred)
		writebyte(false, 0b00000000); // wBlockLengthMsb (1 FSA Channel, Diagnostic)

		// Set Enable Register
		writebyte(false, 0); // wEnable
	}
	else if (initByte == '2') {
		Serial.println("Loading write bootloop parametric registers...");

		// Write Block Length information
		// N=# of FSA Channels, P=# of Pages Transfered
		// MSB -> NNNNXPPP PPPPPPPP <- LSB
		writebyte(false, 0b00000001); // wBlockLengthLsb (1 Page Transferred)
		writebyte(false, 0b00010000); // wBlockLengthMsb (2 FSA Channels)

		// Set Enable Register
		writebyte(false, writeBootloopEnable); // Write Bootloop
	}

	// Write initial MBM Information
	// S=MBM Select, A=Starting Address within each MBM
	// MSB -> XSSSSAAA AAAAAAAA <- LSB
	writebyte(false, 0); // rwAddressLsb
	writebyte(false, 0); // rwAddressMsb 1 MBM in system

	// RAC is now addressed to FIFO
}

void processCommand(byte command) {
	Serial.print("Send command: ");

	// Built-In Commands
	if (command == '0')
	{
		Serial.println("Write Bootloop Register Masked");
		writebyte(true, cmdr | wBootloopRegMasked);
	}
	else if (command == '1')
	{
		Serial.println("Initialize");
		writebyte(true, cmdr | initialize);
	}
	else if (command == '2')
	{
		Serial.println("Read Bubble Data");
		writebyte(true, cmdr | rBubbleData);
	}
	else if (command == '3')
	{
		Serial.println("Write Bubble Data");
		writebyte(true, cmdr | wBubbleData);
	}
	else if (command == '4')
	{
		Serial.println("Read Seek");
		writebyte(true, cmdr | rSeek);
	}
	else if (command == '5')
	{
		Serial.println("Read Bootloop Register");
		writebyte(true, cmdr | rBootloopReg);
	}
	else if (command == '6')
	{
		Serial.println("Write Bootloop Register");
		//fillFifoWithOnes(40);
		//writebyte(true, cmdr | wBootloopReg);
	}
	else if (command == '7')
	{
		Serial.println("Write Bootloop");
		//processWriteBootloop();
	}
	else if (command == '8')
	{
		Serial.println("Read FSA Status");
		writebyte(true, cmdr | rFsaStatus);
		//processFsaStatus();
	}
	else if (command == '9')
	{
		Serial.println("Abort");
		if ((readbyte(true) & busy) != 0) {
			Serial.println("BMC Busy, issue Initialize or MBM Purge after complete.");
		}

		writebyte(true, cmdr | abortCmd);
	}
	else if (command == 'a')
	{
		Serial.println("Write Seek");
		writebyte(true, cmdr | wSeek);
	}
	else if (command == 'b')
	{
		Serial.println("Read Bootloop");
		writebyte(true, cmdr | rBootloop);
	}
	else if (command == 'c')
	{
		Serial.println("Read Corrected Data");
		writebyte(true, cmdr | rCorrectedData);
	}
	else if (command == 'd')
	{
		Serial.println("Reset FIFO");
		writebyte(true, cmdr | resetFifo);
	}
	else if (command == 'e')
	{
		Serial.println("MBM Purge");
		writebyte(true, cmdr | mbmPurge);
	}
	else if (command == 'f')
	{
		Serial.println("Software Reset");
		writebyte(true, cmdr | softwareReset);
	}
	else if (command == 'r')
	{
		Serial.println("Read Bubble Page");
		long page = processPageNumber(0);
		processReadPage(page, processPageNumber(page));
	}
	else if (command == 'w')
	{
		Serial.println("Write Bubble Page");
		long page = processPageNumber(0);
		processWritePage(page, processPageNumber(page));
	}
	else if (command == 'x')
	{
		Serial.println("Fill FIFO with 34 1's");
		//fillFifoWithOnes(34);
	}
	else if (command == 'z')
	{
		Serial.println("Fill FIFO with 40 1's");
		//fillFifoWithOnes(40);
	}

	waitForState(opComplete | opFail, 20, true);
}

long processPageNumber(long startingPage) {
	if (Serial.available()) {
		long page = Serial.parseInt();

		if (page >= startingPage && page <= 2047) {
			return page;
		}
	}

	// If Invalid, or no data, return starting page
	return startingPage;
}

void processReadPage(long startingPage, long endingPage) {
	Serial.print("Read Page ");
	Serial.print(startingPage);

	if (startingPage != endingPage) {
		Serial.print(" - ");
		Serial.println(endingPage);
	}
	else {
		Serial.println();
	}

	for (long page = startingPage; page <= endingPage; page++) {
		Serial.print("Page ");
		Serial.print(page);

		int readAttempt = 5;
		do {
			Serial.print(".");
			readAttempt--;
			readPage(page);
		} while (((waitForState(opComplete | opFail, 20, false) & opFail) > 0) && readAttempt > 0);

		if (readAttempt == 0) {
			Serial.print(" Abort...");
			return;
		}

		Serial.println();
		outputBuffer(rwBuffer, sizeof(rwBuffer), 34);
	}
}

void processWritePage(long startingPage, long endingPage) {
	Serial.print("Write Page ");
	Serial.print(startingPage);

	if (startingPage != endingPage) {
		Serial.print(" - ");
		Serial.println(endingPage);
	}
	else {
		Serial.println();
	}

	// Set buffer to pattern
	// memset(rwBuffer, 0xFF, sizeof(rwBuffer));
	memset(rwParityBuffer, 0, sizeof(rwParityBuffer));
	for (int i = 0; i < sizeof(rwBuffer); i++) {
		byte value = 0xFF;
		rwBuffer[i] = value;
		rwParityBuffer[i] = generateOddParity(value);
	}

	outputBuffer(rwBuffer, sizeof(rwBuffer), 34);

	Serial.print("Writing: ");

	for (long page = startingPage; page <= endingPage; page++) {
		Serial.print(page);

		int readAttempt = 5;
		do {
			readAttempt--;
			Serial.print(".");
			writePage(page);
		} while (((waitForState(opComplete | opFail, 20, false) & opFail) > 0) && readAttempt > 0);

		Serial.print(" ");

		if (readAttempt == 0) {
			Serial.print("Abort...");
			return;
		}
	}

	Serial.println("Done.");
}

void readPage(int pageAddress) {
	// Clear Buffer
	memset(rwBuffer, 0, sizeof(rwBuffer));
	//memset(rwParityBuffer, 0, sizeof(rwParityBuffer));

	// Set address to requested location
	if (!setStartingAddress(pageAddress)) {
		Serial.println("Invalid Address! Must be less than 2048.");
		return;
	}

	// Send Read Command to start reading from FIFO
	writebyte(true, cmdr | rBubbleData);

	// Set address pins to Input
	bus2in();

	// Wait for FIFO to start polling data
	digitalWrite(cd, true);
	while ((quickReadByte() & fifoReady) == 0);

	// Read as quickly as possible
	digitalWrite(cd, false);
	for (int i = 0; i < sizeof(rwBuffer); i++) {
		rwBuffer[i] = quickReadByte();
	}
}

void writePage(int pageAddress) {
	// Set address to requested location
	if (!setStartingAddress(pageAddress)) {
		Serial.println("Invalid Address! Must be less than 2048.");
		return;
	}

	// Reset FIFO
	writebyte(true, cmdr | resetFifo);

	// Send Write Command to start writing from FIFO
	writebyte(true, cmdr | wBubbleData);

	// Set address pins to Input
	bus2in();

	// Wait for FIFO to start polling data
	digitalWrite(cd, true);
	while ((quickReadByte() & fifoReady) == 0);

	// Set address pins to Output
	bus2out();

	// Set A0 to address FIFO
	digitalWrite(cd, false);

	// Process Bytes as fast as possible to keep up with FIFO
	for (int i = 0; i < sizeof(rwBuffer); i++) {
		quickWriteByte(rwBuffer[i], rwParityBuffer[i]);
	}
}

void outputBuffer(byte displayBuffer[], int len, int bytesPerLine) {
	if (len == 0) {
		Serial.println("Buffer Empty.");
		return;
	}

	int lines = len / bytesPerLine;
	int startIndex;
	int endIndex;

	for (int i = 0; i < lines; i++) {
		startIndex = i * bytesPerLine;
		endIndex = startIndex + bytesPerLine;

		outputHexChars(displayBuffer, startIndex, endIndex);
		outputSafeChars(displayBuffer, startIndex, endIndex);

		Serial.println();
	}

	int padding = len % bytesPerLine;

	if (padding == 0) {
		return;
	}

	startIndex = lines * bytesPerLine;
	endIndex = startIndex + padding;

	// Write Trailing HEX
	outputHexChars(displayBuffer, startIndex, endIndex);

	// Padding
	int endPadding = bytesPerLine - padding;
	for (int j = 0; j < endPadding; j++) {
		Serial.print("   ");
	}

	// Write Trailing Chars
	outputSafeChars(displayBuffer, startIndex, endIndex);

	Serial.println();
}

void outputHexChars(byte displayBuffer[], int startIndex, int endIndex) {
	char dataString[3];
	for (int i = startIndex; i < endIndex; i++) {
		sprintf(dataString, "%02X ", displayBuffer[i]);
		Serial.print(dataString);
	}
}

void outputSafeChars(byte displayBuffer[], int startIndex, int endIndex) {
	for (int i = startIndex; i < endIndex; i++) {
		char character = (char)displayBuffer[i];
		// Print safe chars after hex (like a hex editor)
		if ((character >= ' ') && (character <= '~')) {
			// Safe Characters
			Serial.print(character);
		}
		else {
			// Unsafe Characters
			Serial.print(".");
		}
	}
}

/*
  void fillFifoWithOnes(int len) {
  Serial.println("Write all ones to FIFO...");
  for (int i = 0; i < len; i++) {
	if (i % 8 == 0)
	{
	  Serial.println();
	}

	char dataString[2];
	sprintf(dataString, "%02X", 0xFF);
	Serial.print(dataString);

	writebyte(false, 0xFF);
  }
  }

  void processWriteBootloop() {
  fillFifoWithOnes(40);

  Serial.println("Write ones into Bootloop");
  wCommandRegister(wBootloop);
  waitWhileBusy(20);

  Serial.println("Load Seed Data into FIFO...");
  // wFifo(seed_1MZ8_8129, 40);
  writebyte(false, 0); // Append Zero Byte
  } */

byte waitForState(byte waitState, int waitCount, bool displayStatus) {
	byte currentStatus = 0;
	byte previousStatus = 0;
	do {
		waitCount--;
		delay(100);
		currentStatus = readbyte(true);
		if (previousStatus != currentStatus) {
			previousStatus = currentStatus;

			if (displayStatus) {
				rStatusRegister(currentStatus);
			}
		}
	} while ((currentStatus & waitState) == 0 && waitCount > 0);

	if (waitCount == 0)
	{
		Serial.println("Operation Timed Out!");
	}

	return currentStatus;
}

/* Not Needed
  void processFsaStatus() {
  // Read FIFO
  memset(rwBuffer, 0, sizeof(rwBuffer));

  // Only the first two bytes are needed for FSA A/B
  int len = rFifo(2, true);

  for (int i = 0; i < len; i++) {
	byte stats = rwBuffer[i];
	Serial.print("FSA Status:");

	if (stats == 0) {
	  Serial.print(" None");
	}

	if ((stats & fsaUncorrectableError) != 0) {
	  Serial.print(" UncorrectableError");
	}

	if ((stats & fsaCorrectableError) != 0) {
	  Serial.print(" CorrectableError");
	}

	if ((stats & fsaTimingError) != 0) {
	  Serial.print(" TimingError");
	}

	if ((stats & errorCorrectionEnable) != 0) {
	  Serial.print(" ErrorCorrectionEnable");
	}

	if ((stats & fifoFull) != 0) {
	  Serial.print(" FIFOFull");
	}

	if ((stats & fifoEmpty) != 0) {
	  Serial.print(" FIFOEmpty");
	}

	if ((stats & SpareTwo) != 0) {
	  Serial.print(" Spare2");
	}

	if ((stats & SpareOne) != 0) {
	  Serial.print(" Spare1");
	}

	Serial.println();
  }
  }

  void processUserRegister(byte rwByte) {
  if (rwByte == 'r') {
	byte rByte = rAddressedRegister(rwUtility, false);
	Serial.print("User Register Read: ");
	Serial.println(rByte, HEX);
  } else if (rwByte == 'w') {
	int dataByte = Serial.read();
	Serial.print("User Register Write: ");
	Serial.println(dataByte, HEX);
	wRegisterAddressCounter(rwUtility, false);
	writebyte(false, dataByte);
  }
  }
*/

void processFifo(byte frwByte) {
	memset(rwBuffer, 0, sizeof(rwBuffer));
	if (frwByte == 'r') {
		Serial.println("FIFO Read:");
		Serial.print(rFifo(true));
		Serial.println(" bytes read.");
	}
	else if (frwByte == 'w') {
		Serial.println("FIFO Write:");
		int i;
		// FIFO can hold up to 43 bytes before data loss occurs
		for (i = 0; i < 43; i++) {
			if (!Serial.available()) {
				break;
			}

			rwBuffer[i] = Serial.read();
		}

		if (Serial.available()) {
			Serial.print("Dropping remainder of Bytes: ");
			while (Serial.available()) {
				Serial.print(Serial.read());
			}

			Serial.println();
		}

		wFifo(i, true);

		Serial.print(i);
		Serial.println(" bytes written.");
	}
}

// Returns true if success, false otherwise
bool setStartingAddress(int page) {
	if (page >= 2047) {
		// Only values between 0x000-0x7FF
		return false;
	}

	// Write LSB using first LSB of int
	wRegisterAddressCounter(rwAddressLsb, false);
	writebyte(false, page & 0xFF);

	// Shift right one byte, mask for MSB Address bits
	// Note: MBM Select Bits are set to 0 for 1 MBM
	writebyte(false, (page >> 8) & nStartAddressArMsb);

	return true;
}

bool wRegisterAddressCounter(byte address, bool ignoreFifo) {
	// If the FIFO is not Empty, writing to the RAC will clear byte 0
	if (ignoreFifo || (readbyte(true) & fifoReady) == 0) {
		writebyte(true, address);
		return true;
	}

	Serial.print("WARNING: FIFO not empty. Writing to RAC clears byte 0 of FIFO. Operation Skipped: ");
	Serial.println(address, BIN);
	return false;
}

void wFifo(int len, bool printMessage) {
	// FIFO can only store up to 43 bytes, but we can put in more than that, the first bytes will just fall off
	for (int i = 0; i < len; i++) {
		writebyte(false, rwBuffer[i]);
	}

	if (printMessage) {
		outputBuffer(rwBuffer, len, 8);
	}
}

int rFifo(bool printMessage) {
	// FIFO can only store up to 43 bytes, but attempt to read up to the size of the buffer
	int actualBytes = 0;
	while (actualBytes < sizeof(rwBuffer)) {
		// If FIFOReady flag is not set, the FIFO is empty
		if ((readbyte(true) & fifoReady) == 0)
		{
			break;
		}

		rwBuffer[actualBytes] = readbyte(false);
		actualBytes++;
	}

	if (printMessage) {
		outputBuffer(rwBuffer, actualBytes, 8);
	}

	return actualBytes;
}

/* Not Needed
  byte rAddressedRegister(byte address, bool ignoreFifo) {
  if (wRegisterAddressCounter(address, ignoreFifo)) {
	return readbyte(false);
  }

  // Failed to address RAC so cannot return byte value
  return 0xFF;
  }

  void rAllRegisterValues() {
  Serial.println("All Register Status:");
  if (wRegisterAddressCounter(rwUtility, false)) {
	byte rUtility = readbyte(false);
	byte rAddressLsb = rAddressedRegister(rwAddressLsb, true);
	byte rAddressMsb = readbyte(false); // Auto Increment, ends on FIFO

	Serial.print("MSM Select: ");
	Serial.println((nMbmSelectArMsb & rAddressMsb) >> 3, HEX);

	Serial.print("Starting Address within each MBM: ");
	Serial.print((nStartAddressArMsb & rAddressMsb), HEX);
	Serial.println(rAddressLsb, HEX);

	Serial.println();

	Serial.print("Utility: ");
	Serial.println(rUtility, HEX);

	Serial.print("Address LSB: ");
	Serial.println(rAddressLsb, HEX);

	Serial.print("Address MSB: ");
	Serial.println(rAddressMsb, HEX);
  }
  } */

void rStatusRegister(byte stats) {
	Serial.print("Status:");

	if (stats == 0) {
		Serial.print(" None");
	}

	if ((stats & busy) != 0) {
		Serial.print(" Busy");
	}

	if ((stats & opComplete) != 0) {
		Serial.print(" OpComplete");
	}

	if ((stats & opFail) != 0) {
		Serial.print(" OpFail");
	}

	if ((stats & timingError) != 0) {
		Serial.print(" TimingError");
	}

	if ((stats & correctableError) != 0) {
		Serial.print(" CorrectableError");
	}

	if ((stats & uncorrectableError) != 0) {
		Serial.print(" UncorrectableError");
	}

	if ((stats & parityError) != 0) {
		Serial.print(" ParityError");
	}

	if ((stats & fifoReady) != 0) {
		Serial.print(" FIFOReady");
	}

	Serial.println();
}

void softReset(bool initialStart, byte force) {
	if (initialStart || (readbyte(true) & busy) == 0 || (force == 'f')) {
		// Initialize in Reset mode
		Serial.println("Reset Enabled.");
		digitalWrite(rs, LOW);
		digitalWrite(LED_BUILTIN, LOW);
		delayMicroseconds(200);

		// Initialize State
		digitalWrite(cd, LOW);
		digitalWrite(rd, HIGH);
		digitalWrite(wr, HIGH);

		// Set address pins to Output
		bus2out();

		// Zero Bus
		digitalWrite(ad0, LOW);
		digitalWrite(ad1, LOW);
		digitalWrite(ad2, LOW);
		digitalWrite(ad3, LOW);
		digitalWrite(ad4, LOW);
		digitalWrite(ad5, LOW);
		digitalWrite(ad6, LOW);
		digitalWrite(ad7, LOW);
		digitalWrite(adp, LOW);
	}
	else {
		Serial.println("BMC Busy, issue Abort command before shutting down or data loss may occur.");
	}
}

bool generateOddParity(byte data) {
	int onesCount = 0;
	for (int i = 0; i < 8; i++) {
		if (bitRead(data, i))
		{
			onesCount++;
		}
	}

	return onesCount % 2 == 0;
}

void bus2out() {
	pinMode(ad0, OUTPUT);
	pinMode(ad1, OUTPUT);
	pinMode(ad2, OUTPUT);
	pinMode(ad3, OUTPUT);
	pinMode(ad4, OUTPUT);
	pinMode(ad5, OUTPUT);
	pinMode(ad6, OUTPUT);
	pinMode(ad7, OUTPUT);
	pinMode(adp, OUTPUT);
}

void bus2in() {
	pinMode(ad0, INPUT);
	pinMode(ad1, INPUT);
	pinMode(ad2, INPUT);
	pinMode(ad3, INPUT);
	pinMode(ad4, INPUT);
	pinMode(ad5, INPUT);
	pinMode(ad6, INPUT);
	pinMode(ad7, INPUT);
	pinMode(adp, INPUT);
}

// No Parity Check
byte quickReadByte() {
	// start READ cycle
	digitalWrite(rd, LOW);

	byte r;
	bitWrite(r, 0, digitalRead(ad0));
	bitWrite(r, 1, digitalRead(ad1));
	bitWrite(r, 2, digitalRead(ad2));
	bitWrite(r, 3, digitalRead(ad3));
	bitWrite(r, 4, digitalRead(ad4));
	bitWrite(r, 5, digitalRead(ad5));
	bitWrite(r, 6, digitalRead(ad6));
	bitWrite(r, 7, digitalRead(ad7));

	// end READ cycle
	digitalWrite(rd, HIGH);

	return r;
}

byte readbyte(bool commandStatus) {
	digitalWrite(cd, commandStatus);

	// start READ cycle
	digitalWrite(rd, LOW);

	// Set address pins to Input
	bus2in();

	byte r;
	bitWrite(r, 0, digitalRead(ad0));
	bitWrite(r, 1, digitalRead(ad1));
	bitWrite(r, 2, digitalRead(ad2));
	bitWrite(r, 3, digitalRead(ad3));
	bitWrite(r, 4, digitalRead(ad4));
	bitWrite(r, 5, digitalRead(ad5));
	bitWrite(r, 6, digitalRead(ad6));
	bitWrite(r, 7, digitalRead(ad7));
	bool parity = digitalRead(adp);

	// end READ cycle
	digitalWrite(rd, HIGH);

	if (parity != generateOddParity(r)) {
		Serial.println("Parity Mismatch!");
	}

	return r;
}

void quickWriteByte(byte data, byte parity) {
	// set value on bus
	digitalWrite(ad0, bitRead(data, 0));
	digitalWrite(ad1, bitRead(data, 1));
	digitalWrite(ad2, bitRead(data, 2));
	digitalWrite(ad3, bitRead(data, 3));
	digitalWrite(ad4, bitRead(data, 4));
	digitalWrite(ad5, bitRead(data, 5));
	digitalWrite(ad6, bitRead(data, 6));
	digitalWrite(ad7, bitRead(data, 7));
	digitalWrite(adp, parity);

	// start WRITE cycle
	digitalWrite(wr, LOW);

	// end WRITE cycle
	digitalWrite(wr, HIGH);
}

void writebyte(bool commandStatus, byte data) {
	digitalWrite(cd, commandStatus);

	// Set address pins to Output
	bus2out();

	quickWriteByte(data, generateOddParity(data));
}
