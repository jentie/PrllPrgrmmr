/*
    PrllPrgrmmr - Programmmer for parallel EEPROM etc.

    2022-03-28, Jens

    v0 - 2022-03-28, starting with basic structure before hardware design
    v1 - 2022-04-05, basic functionality tested with hardware (first: SRAM)
    
    additional info:
    https://de.wikipedia.org/wiki/Intel_HEX


    ideas:
    - check size (empty check, write address in every page)
    
*/


// LED_BUILTIN        // pending memory cycle
#define nLED   12     // status - 

#define LALd   14     // low address register
#define HALd   15     // high address register
#define nAOE   16     // address output enable

#define nWE    17     // ROM /WE
#define nCE    18     // ROM /CE
#define nOE    19     // ROM /OE

// data bus - 4 .. 11
// PortB0..3 --> D0..D3
// PortD4..7 --> D4..D7


#define LINE_SIZE 256                   // maximum input string length

char cmdline[LINE_SIZE];                // warning: length check might be needed
byte lineptr = 0;
char *arg0, *arg1, *arg2, *arg3;        // pointer to args in command line

byte verbose = 1;

char cycle = ' ';                      // type of on-going memory cycle

word memsize = 0;                         // size of memory chip


void setup() {

  // set serial
  Serial.begin(115200);

  if (verbose)
    Serial.println("\nPrllPrgrmmr - Programmmer for parallel EEPROM etc.");

  // init internal LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // init status LED
  pinMode(nLED, OUTPUT);
  digitalWrite(nLED, HIGH);
  delay(100);
  digitalWrite(nLED, LOW);
  delay(100);
  digitalWrite(nLED, HIGH);           // blink, blink
  delay(100);
  digitalWrite(nLED, LOW);
  delay(100);
  digitalWrite(nLED, HIGH);

  // init control lines
  // set inactive, level first (needed?)
  digitalWrite(LALd, LOW);
  digitalWrite(HALd, LOW);
  digitalWrite(nAOE, HIGH);
  pinMode(LALd, OUTPUT);
  pinMode(HALd, OUTPUT);
  pinMode(nAOE, OUTPUT);

  digitalWrite(nWE, HIGH);
  digitalWrite(nCE, HIGH);
  digitalWrite(nOE, HIGH);
  pinMode(nWE, OUTPUT);
  pinMode(nCE, OUTPUT);
  pinMode(nOE, OUTPUT);

  // data bus, inactive is input
  DDRB = DDRB & 0xF0;       // lower part of port B as input
  DDRD = DDRD & 0x0F;       // higher part of port D as input
  //
  //  for (i = 4; i < 12; i++)
  //    pinMode(i, INPUT);
}


byte read_byte(word address) {
  byte ladr = address;
  byte hadr = address >> 8;

  DDRB = DDRB | 0x0F;             // lower part of port B is output
  DDRD = DDRD | 0xF0;             // higher part of port D is output

  PORTB = (PORTB & 0xF0) | (hadr & 0x0F);
  PORTD = (PORTD & 0x0F) | (hadr & 0xF0);
  digitalWrite(HALd, HIGH);       // take high address part
  digitalWrite(HALd, LOW);

  PORTB = (PORTB & 0xF0) | (ladr & 0x0F);
  PORTD = (PORTD & 0x0F) | (ladr & 0xF0);
  digitalWrite(LALd, HIGH);       // take low address part
  digitalWrite(LALd, LOW);

  DDRB = DDRB & 0xF0;             // lower part of port B as input
  DDRD = DDRD & 0x0F;             // higher part of port D as input

  digitalWrite(nAOE, LOW);
  delayMicroseconds(1);           // stabilize address, shortest might be 3 us

  digitalWrite(nCE, LOW);
  digitalWrite(nOE, LOW);         // memory read access
  delayMicroseconds(1);           // memory access time, shortest might be 3 ms

  byte value = (PIND & 0xF0) | (PINB & 0x0F);

  digitalWrite(nOE, HIGH);
  digitalWrite(nCE, HIGH);
  digitalWrite(nOE, HIGH);

  return value;
}


void write_byte(word address, byte dataval) {

  byte ladr = address;
  byte hadr = address >> 8;

  DDRB = DDRB | 0x0F;             // lower part of port B is output
  DDRD = DDRD | 0xF0;             // higher part of port D is output

  PORTB = (PORTB & 0xF0) | (hadr & 0x0F);
  PORTD = (PORTD & 0x0F) | (hadr & 0xF0);
  digitalWrite(HALd, HIGH);       // take high address part
  digitalWrite(HALd, LOW);

  PORTB = (PORTB & 0xF0) | (ladr & 0x0F);
  PORTD = (PORTD & 0x0F) | (ladr & 0xF0);
  digitalWrite(LALd, HIGH);       // take low address part
  digitalWrite(LALd, LOW);

  PORTB = (PORTB & 0xF0) | (dataval & 0x0F);
  PORTD = (PORTD & 0x0F) | (dataval & 0xF0);

  digitalWrite(nAOE, LOW);
  delayMicroseconds(1);           // stabilize address, shortest might be 3 us

  digitalWrite(nCE, LOW);
  digitalWrite(nWE, LOW);         // memory write access
  delayMicroseconds(1);           // memory access time, shortest might be 3 us
  digitalWrite(nWE, HIGH);

  DDRB = DDRB & 0xF0;             // lower part of port B as input
  DDRD = DDRD & 0x0F;             // higher part of port D as input

  digitalWrite(nCE, HIGH);
  digitalWrite(nOE, HIGH);
}


void print_byte(byte num) {
  if (num < 0x10)
    Serial.print("0");
  Serial.print(num, HEX);
}

void print_word(word num) {
  if (num < 0x1000)
    Serial.print("0");
  if (num < 0x0100)
    Serial.print("0");
  if (num < 0x0010)
    Serial.print("0");
  Serial.print(num, HEX);
}


byte get_byte(char* line, byte ptr) {

  char nibble;
  byte dataval;

  nibble = line[ptr];               // toUpper?
  if ( nibble >= 'A')
    dataval = nibble - 'A' + 10;
  else
    dataval = nibble - '0';
  dataval*=16;
  
  nibble = line[ptr+1];             // toUpper?
  if (nibble >= 'A')
    dataval += nibble - 'A' + 10;
  else
    dataval += nibble - '0';
  
  return dataval;
}


word get_word(char* line, byte ptr) {

  word dataval = get_byte(line, ptr) * 256;
  ptr+=2;
  dataval += get_byte(line, ptr);
  
  return dataval;
}


void cmd_help() {
  Serial.println("PrllPrgrmmr - Programmmer for parallel EEPROM etc.");
  Serial.println("!h      - this help        !v        - toggle verbose flag");
  Serial.println();
  Serial.println("!a aaaa - apply address");
  Serial.println("!o dd   - output data      !i        - input data");
  Serial.println("!c      - complete memory access cylce");
  Serial.println();
  Serial.println("!r aaaa      - read memory location");
  Serial.println("!w aaaa dd   - write data to address");
  Serial.println("!d aaaa nnnn - dump nn bytes starting at aaaa");
  Serial.println("!s nnnn      - set size");
  Serial.println("!f dd        - fill memory with data");
  Serial.println("todo: program / flash / type?");
  Serial.println();
  Serial.println(": nnnn       - capture Intel HEX file");
  Serial.println(":20000000F3..- write Intel HEX format");
}


void cmd_verbose() {
  if (verbose)
    verbose = 0;
  else
    verbose = 1;

  Serial.println("ok");
}


void cmd_addr() {

  word address;

  if (arg1 == NULL) {
    Serial.println("error: address needed");
    return;
  } else if (arg2 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  address = strtol(arg1, &arg2, 16 );

  if (verbose) {
    Serial.print("apply address ");
    print_word(address);
    Serial.println();
  }

  byte ladr = address;
  byte hadr = address >> 8;

  DDRB = DDRB | 0x0F;             // lower part of port B is output
  DDRD = DDRD | 0xF0;             // higher part of port D is output

  PORTB = (PORTB & 0xF0) | (hadr & 0x0F);
  PORTD = (PORTD & 0x0F) | (hadr & 0xF0);
  digitalWrite(HALd, HIGH);       // take high address part
  digitalWrite(HALd, LOW);

  PORTB = (PORTB & 0xF0) | (ladr & 0x0F);
  PORTD = (PORTD & 0x0F) | (ladr & 0xF0);
  digitalWrite(LALd, HIGH);       // take low address part
  digitalWrite(LALd, LOW);

  DDRB = DDRB & 0xF0;             // lower part of port B as input
  DDRD = DDRD & 0x0F;             // higher part of port D as input

  digitalWrite(nAOE, LOW);
}


void cmd_out() {

  byte dataval;

  if (arg1 == NULL) {
    Serial.println("error: data needed");
    return;
  } else if (arg2 != NULL) {
    Serial.println("error: too many arguments");
    return;
  } else {
    dataval = strtol(arg1, &arg3, 16 );

    if (verbose) {
      Serial.print("apply data byte ");
      print_byte(dataval);
      Serial.println(", with /CE and /WE");
    }

    DDRB = DDRB | 0x0F;             // lower part of port B is output
    DDRD = DDRD | 0xF0;             // higher part of port D is output

    PORTB = (PORTB & 0xF0) | (dataval & 0x0F);
    PORTD = (PORTD & 0x0F) | (dataval & 0xF0);

    digitalWrite(nCE, LOW);
    digitalWrite(nWE, LOW);         // beginning of memory write access

    cycle = 'w';                    // on-going memory cycle is write

    digitalWrite(LED_BUILTIN, HIGH);  // memory cycle pending
  }
}


void cmd_in() {

  if (arg1 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  if (verbose) {
    Serial.println("apply /CE and /OE, ready for read");
  }

  DDRB = DDRB & 0xF0;             // lower part of port B as input
  DDRD = DDRD & 0x0F;             // higher part of port D as input

  digitalWrite(nCE, LOW);
  digitalWrite(nOE, LOW);         // beginning of memory read access

  cycle = 'r';

  digitalWrite(LED_BUILTIN, HIGH);  // memory cycle pending
}


void cmd_ctrl() {

  if (arg1 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  if (verbose) {
    Serial.print("completed '");
    Serial.print(cycle);
    Serial.println("' memory cycle");
  }

  if (cycle == 'r') {
    byte datavalue = (PIND & 0xF0) | (PINB & 0x0F);
    print_byte(datavalue);
    Serial.println();
  } else
    Serial.println("ok");

  digitalWrite(nOE, HIGH);        // disable all control signals
  digitalWrite(nWE, HIGH);
  digitalWrite(nCE, HIGH);

  DDRB = DDRB & 0xF0;             // lower part of port B as input
  DDRD = DDRD & 0x0F;             // higher part of port D as input

  cycle = ' ';

  digitalWrite(LED_BUILTIN, LOW); // memory cycle completed
}


void cmd_read() {

  word address;

  if (arg1 == NULL) {
    Serial.println("error: address needed");
    return;
  } else if (arg2 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  address = strtol(arg1, &arg2, 16 );

  if (verbose) {
    Serial.print("read from address ");
    print_word(address);
    Serial.println();
  }

  byte dataval = read_byte(address);

  print_byte(dataval);
  Serial.println();

}


void cmd_write() {

  word address;
  byte dataval;

  if (arg1 == NULL) {
    Serial.println("error: address needed");
    return;
  } else if (arg2 == NULL) {
    Serial.println("error: data needed");
    return;
  }

  address = strtol(arg1, &arg3, 16 );
  dataval = strtol(arg2, &arg3, 16 );

  write_byte(address, dataval);

  if (verbose) {
    Serial.print("wrote ");
    print_byte(dataval);
    Serial.print(" to address ");
    print_word(address);
    Serial.println();
  }

  Serial.println("ok");
}


void cmd_dump() {

  word address;
  word count;

  char ascii[16];
  byte acnt = 0;

  if (arg1 == NULL) {
    Serial.println("error: address needed");
    return;
  } else if (arg2 == NULL) {
    Serial.println("error: length needed");
    return;
  }

  address = strtol(arg1, &arg3, 16 );
  count = strtol(arg2, &arg3, 16 );

  //    if (verbose) {
  //      Serial.print("list from ");
  //      print_word(address);
  //      Serial.println();
  //    }

  word n = 0;
  print_word(address);
  Serial.print(" - ");
  do {
    byte dataval = read_byte(address);
    print_byte(dataval);

    if ((dataval < ' ') || (dataval > '~'))
      ascii[acnt] = '.';                      // non-printable character
    else
      ascii[acnt] = dataval;                  // print ascii character
    acnt++;

    address++;
    n++;
    if ((n % 16) != 0)
      Serial.print(' ');
    else {
      Serial.print(" - ");
      Serial.print(ascii);
      acnt = 0;
      if (n != count) {
        Serial.println();
        print_word(address);
        Serial.print(" - ");
      }
    }
  } while (n != count);
  Serial.println();

  Serial.println("ok");
}


void cmd_size() {

  if (arg1 == NULL) {
    Serial.println("error: memory size needed");
    return;
  } else if (arg2 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  memsize = strtol(arg1, &arg3, 16 );

  if (verbose) {
    Serial.print("set size to ");
    print_word(memsize);
    Serial.print(" (");
    Serial.print((float)memsize / 1024.0, 0);
    Serial.println("K)");
  }

  Serial.println("ok");
}


void cmd_fill() {

  byte dataval;

  if (arg1 == NULL) {
    Serial.println("error: fill byte needed");
    return;
  } else if (arg2 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  dataval = strtol(arg1, &arg3, 16 );

  // verbose

  word address = 0;
  do {
    write_byte(address, dataval);
    address++;

  } while (address != memsize);

  Serial.println("ok");
}


word hex_capt(void) {

  word memsize;
  byte dataval;

  if (arg1 == NULL) {
    Serial.println("error: memory size needed");
    return;
  } else if (arg2 != NULL) {
    Serial.println("error: too many arguments");
    return;
  }

  memsize = strtol(arg1, &arg3, 16 );

  memsize &= 0xffe0;               // round to full lines of 0x20 byte

  if (verbose) {
    Serial.print("capture ");
    print_word(memsize);
    Serial.print(" (");
    Serial.print(memsize);
    Serial.println(") bytes in Intel HEX format");
  }

  word address = 0;
  
  Serial.print(":20");          // byte count
  print_word(address);
  Serial.print("00");           // type: data record

  byte checksum = 0x20;
  checksum += lowByte(address);   // currently 0
  checksum += highByte(address);

  do {
    dataval = read_byte(address);
    print_byte(dataval);
    checksum += dataval;
    address++;

    if ((address % 0x20) == 0) {    // end of line
      checksum = 0 - checksum;
      print_byte(checksum);
      Serial.println();
      
      if (address != memsize) {
        Serial.print(":20");      // new begin of line
        print_word(address);
        Serial.print("00");

        checksum = 0x20;
        checksum += lowByte(address);
        checksum += highByte(address);
      }
    }
  } while (address != memsize);

  Serial.println(":00000001FF");        // marks end of hex file
}


byte hex_prog(void) {

  // eof :00000001FF

  if ((cmdline[7] != '0') || (cmdline[8] != '0'))
    return -1;                                       // wrong type, only data accepted

  if (strncmp(cmdline, ":00000001FF", 11) == 0) {
    if (verbose)
      Serial.println("end of HEX file");
    return 0;
  }

  byte count = get_byte(cmdline, 1);

  word address = get_word(cmdline, 3);

  byte checksum = 0;
  for (byte i = 1; i < ((count * 2) + 10); i += 2) {
    checksum += get_byte(cmdline,i);
  }
  if (checksum)                           // should add to zero
    Serial.println("error: checksum");

  if (verbose) {
    Serial.print("write ");
    print_byte(count);
    Serial.print(" bytes starting from ");
    print_word(address);
    Serial.println();
  }

  byte lineptr = 9;                       // start of data section

  for (byte n = 1; n <= count; n++) {      // interate over data bytes
    
    byte dataval = get_byte(cmdline, lineptr);

    write_byte(address, dataval);
    
    address++;
    lineptr += 2;
  }

  Serial.println("ok");

  return;
}


void commands(void)
{
  if (verbose)
    Serial.println(cmdline);

  arg0 = strtok(cmdline, " \n");

  if (arg0 != NULL) {
    arg1 = strtok(NULL, " \n");
    if (arg1 != NULL) {
      arg2 = strtok(NULL, " \n");
      if (arg2 != NULL) {
        arg3 = strtok(NULL, " \n");
        if (arg3 != NULL) {
          Serial.println("too many arguments");
          return;
        }
      }
    }
  }

  if (arg0[0] == '!') {
    switch (arg0[1]) {
      case 'h': cmd_help();
        break;
      case 'v': cmd_verbose();
        break;

      case 'a': cmd_addr();
        break;
      case 'o': cmd_out();
        break;
      case 'i': cmd_in();
        break;
      case 'c': cmd_ctrl();
        break;

      case 'r': cmd_read();
        break;
      case 'w': cmd_write();
        break;
      case 'd': cmd_dump();
        break;
      case 's': cmd_size();
        break;
      case 'f': cmd_fill();
        break;
      default:
        Serial.println("error: command");
        break;
    }
  } else if (arg0[0] == ':')
    if (arg0[1] == 0)             // arg0 is only ':'
      hex_capt();
    else
      hex_prog();
  else
    Serial.println("error: input");
}


void loop() {

  if (Serial.available()) {
    char ch = Serial.read();

    if ( ch != '\n' ) {               // interface uses line feed / new line
      if ( ch >= ' ') {
        cmdline[lineptr] = ch;        // only printable characters
        lineptr++;                    // warning: length check might be needed
      }
    } else {
      cmdline[lineptr] = 0;            // end of line / string
      lineptr = 0;

      commands();
    }
  }
}
