/*
 * Rocket Project Prometheus
 * 2020-2021
 * PadDAQ
 */

// pin numbers
#define PIN_PT0 A5

// configurable parameters
#define DEBUGGING false
#define BAUD_RATE 57600 // bits per second being transmitted
#define OFFSET_DELAY_TX 25 // specify additional flat "delay" added to minimum calculated delay to avoid corruption
#define SERIAL_TRANSFER_TIMEOUT 50
#define SERIAL_TRANSFER_DEBUGGING true
#define NUM_OF_PT 1
#define NUM_OF_TC 0
#define NUM_OF_LC 0

/* NOTES:
 *  - TX_DELAY, the lower it is, leads to more often CRC errors, and eventually stale packet issues
 *  - 25 ms seems to be not bad, with the default TIMEOUT value (50)
 *  - TIMEOUT is used to determine a timeout threshold for parsing a datapacket from the Serial connection
 *     - if timeout too short for the length of packet, then will repeatedly cause "ERROR: STALE PACKET"
 *  - errors in datastream of the form of "ERROR: ..."; this CAN be disabled, but we should probably leave as is so we can do easy debugging
 */


// macros
#define MIN_DELAY_TX (1/(BAUD_RATE/10))*1000*2 // refer to `https://github.com/UCLA-Rocket-Project/prometheus-groundsys-2021/blob/main/docs/safer_serial_transmission_practices.md`
#define DELAY_TX MIN_DELAY_TX + OFFSET_DELAY_TX

// calibration factors
const float PT_OFFSET[NUM_OF_PT] = {-245.38}; // PT5, PT3: -209.38, 11.924
const float PT_SCALE[NUM_OF_PT] = {248.41}; // PT5, PT3: 244.58, 178.51

// link necessary libraries
#include <SerialTransfer.h>
#include <SoftwareSerial.h>

// init SoftwareSerial connection to bunker
SoftwareSerial to_bunker_connection(4, 8); // RX, TX

// init SerialTransfer
SerialTransfer transfer_to_bunker;

// data packet structure
struct Datapacket
{
  // data portion
  unsigned long timestamp;
  float pt0_data;

  // data checksum
  float checksum;
};

// init buffer for datapacket
Datapacket dp;

void setup()
{
  // start Serial connections
  to_bunker_connection.begin(BAUD_RATE);
  transfer_to_bunker.begin(to_bunker_connection, SERIAL_TRANSFER_DEBUGGING, Serial, SERIAL_TRANSFER_TIMEOUT);
  transfer_to_bunker.reset();

  if (DEBUGGING)
  {
    Serial.begin(9600);
  }
}

void loop()
{
  // reset data buffers
  reset_buffers(dp);

  // poll data, preprocess (if needed), and store in datapacket
  dp.timestamp = millis();
  dp.pt0_data = get_psi_from_raw_pt_data(analogRead(PIN_PT0), 0);

  // compute and update checksum
  update_checksum(dp);

  // transmit packet
  transfer_to_bunker.sendDatum(dp);

  // output to Serial debugging information
  if (DEBUGGING)
  {
    Serial.print(dp.timestamp);
    Serial.print(',');
    Serial.print(dp.pt0_data);
  }

  // delay so we don't sample/transmit too fast :)
  delay(DELAY_TX);
}

/*
 * update_valid()
 * -------------------------
 * Update inputed valid signal `valid_sig` with specified encoding
 * `valid_encoding` if `Data` instance at index `i` in buffer `buf`
 * is valid (member == true). Otherwise, do nothing.
 * 
 * Inputs:
 *   - valid_sig: pointer to integer representing valid signal transmitted
 *   - buf: `Data` buffer to lookup into
 *   - i: index of `Data` instance to analyze
 *   - valid_encoding: bitmask encoding for valid signal for particular data entry
 */
/*void update_valid(Datapacket& dp, int valid_encoding)
{
  // check if data is valid
  dp.valid |= valid_encoding;
}*/

/*
 * reset_buffers()
 * -------------------------
 * Resets all used global data buffers to 0 (and/or false).
 */
void reset_buffers(Datapacket& dp)
{
  dp.timestamp = 0;
  dp.pt0_data = 0;
}

/*
 * get_psi_from_raw_pt_data()
 * -------------------------
 * Converts raw voltage reading from PT to human-readable PSI value
 * for pressure. Utilizes pre-set calibration factors (scale and offset).
 * 
 * Inputs:
 *   - raw_data: integer representing raw voltage reading from PT
 *   - pt_num: integer representing index number of PT (0 to NUM_OF_PT)
 * 
 * Returns: `Data` instance, containing float representing pressure (in PSI) corresponding to PT reading
 */
float get_psi_from_raw_pt_data(int raw_data, int pt_num)
{
  // get calibration factors for specified PT
  float offset = PT_OFFSET[pt_num];
  float scale = PT_SCALE[pt_num];

  // convert to voltage
  float voltage = raw_data * (5.0/1023.0); // arduino-defined conversion from raw analog value to voltage

  // convert to psi (consider calibration factors)
  float psi = voltage*scale + offset; 

  // return value
  return psi;
}

/*float get_lbf_from_raw_lc_data(int raw_data)
{
  // get calibration factors for LC
  float offset = LC_OFFSET;
  float scale = LC_SCALE;

  // convert to voltage
  float voltage = raw_data * (5.0/1023.0); // arduino-defined conversion from raw analog value to voltage

  // convert to lbf (consider calibration factors)
  float lbf = voltage*scale + offset; 

  // return value
  return lbf;
}*/

void update_checksum(Datapacket& dp)
{
  dp.checksum = dp.pt0_data;
}
