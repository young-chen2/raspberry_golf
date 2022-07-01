/*
 Initializes the mcp3008 serial device to communicate to
 the Pi using a SPI connection.
 */
void mcp3008_init(void);

/*
 Reads the analog data from the mcp3008 device.
 The channel numbers range from 0-7.
 */
unsigned int mcp3008_read( unsigned int channel );