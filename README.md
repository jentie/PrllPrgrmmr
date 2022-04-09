# PrllPrgrmmr

Simple Programmer for 28- / 32-pin DIL EEPROM / FRAM /SRAM

usage: commands / data via serial line usb interface

*work in progress, first test with 8K SRAM / FRAM using Arduino IDE*

current commands (see help command):
```
   PrllPrgrmmr - Programmmer for parallel EEPROM etc.");
   !h      - this help        !v        - toggle verbose flag");

   !a aaaa - apply address");
   !o dd   - output data      !i        - input data");
   !c      - complete memory access cylce");
 
   !r aaaa      - read memory location");
   !w aaaa dd   - write data to address");
   !d aaaa nnnn - dump nn bytes starting at aaaa");
   !s nnnn      - set size");
   !f dd        - fill memory with data");
   
   todo: program / flash / type?");

   : nnnn       - capture Intel HEX file");
   :20000000F3..- write Intel HEX format");
```

Todo: 
* more testing, esp. tying EEPROM
* check out direct file access (Intel HEX upload), small Pyton helper needed?
* ...
