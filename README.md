# MIOTY™ AT-Client

## Release

## Information

Client to communicate with a MIOTY™ modem that uses MIOTY™ ENDPOINT AT-Protol v2.2.x. 

## Integration

The user needs to implement the functions:  

- miotyAtClientWrite
- miotyAtClientRead

In order to react to the MYON's 'Transmit Active' events, which are sent via the serial interface, the user must implement the following functions: 
- miotyAtClientTx_start_cb
- miotyAtclientTx_stop_cb

Arduino libraries can be installed manually as described in [https://www.arduino.cc/en/Guide/Libraries#toc5](https://www.arduino.cc/en/Guide/Libraries#toc5)


## Test
This project is unit tested with the Ceedling framework. Visit their (homepage)[https://www.throwtheswitch.org/ceedling] for more information and to install. All unit tests are stored in the test subdirectory and can be extended as needed. To run the tests, simply navigate to the project's root directory and enter the command `ceedling test`. 