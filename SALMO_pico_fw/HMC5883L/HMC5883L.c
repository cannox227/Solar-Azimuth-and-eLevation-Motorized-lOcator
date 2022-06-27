// I2Cdev library collection - HMC5883L I2C device class header file
// Based on Honeywell HMC5883L datasheet, 10/2010 (Form #900405 Rev B)
// 6/12/2012 by Jeff Rowberg <jeff@rowberg.net>
// 6/6/2015 by Andrey Voloshin <voloshin@think.in.ua>
// 03/28/2017 by Kamnev Yuriy <kamnev.u1969@gmail.com>
// 11/04/2022 by Tollardo Simone, Tommaso Canova, Lisa Santarossa, Gabriele Berretta
//
// Changelog:
//     2022-04-11 - ported to RP2040
//     2017-03-28 - ported to STM32 using Keil MDK Pack
//     2015-06-06 - ported to STM32 HAL library from Arduino code
//     2012-06-12 - fixed swapped Y/Z axes
//     2011-08-22 - small Doxygen comment fixes
//     2011-07-31 - initial release

/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#include "HMC5883L.h"
#include "HMC5883L_I2C.h"

static uint8_t slaveAddr = HMC5883L_DEFAULT_ADDRESS;
static uint8_t buffer[6];
static uint8_t mode;

// ID_* registers

/** Get identification byte A
 * @return ID_A byte (should be 01001000, ASCII value 'H')
 */
uint8_t HMC5883L_getIDA() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_ID_A, 1);
    return buffer[0];
}
/** Get identification byte B
 * @return ID_A byte (should be 00110100, ASCII value '4')
 */
uint8_t HMC5883L_getIDB() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_ID_B, 1);
    return buffer[0];
}
/** Get identification byte C
 * @return ID_A byte (should be 00110011, ASCII value '3')
 */
uint8_t HMC5883L_getIDC() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_ID_C, 1);
    return buffer[0];
}

/** Verify the I2C connection.
 * Make sure the device is connected and responds as expected.
 * @return True if connection is valid, false otherwise
 */
bool isHMC() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_ID_A, 3);
    return (buffer[0] == 'H' && buffer[1] == '4' && buffer[2] == '3');
}

//------------------------------------------------------------------------------

/** Power on and prepare for general usage.
 * This will prepare the magnetometer with default settings, ready for single-
 * use mode (very low power requirements). Default settings include 8-sample
 * averaging, 15 Hz data output rate, normal measurement bias, a,d 1090 gain (in
 * terms of LSB/Gauss). Be sure to adjust any settings you need specifically
 * after initialization, especially the gain settings if you happen to be seeing
 * a lot of -4096 values (see the datasheet for mor information).
 */
void HMC5883L_initialize() {
    // value0 << (value0_bit_in_reg - value0_lengh +1) | value1 << (value1_bit_in_reg - value1_lengh +1) ...
    // set config register to 0x01110000 (see datasheet)
    buffer[0]=((HMC5883L_AVERAGING_8 << (HMC5883L_CRA_AVERAGE_BIT - HMC5883L_CRA_AVERAGE_LENGTH + 1)) |
        (HMC5883L_RATE_15     << (HMC5883L_CRA_RATE_BIT - HMC5883L_CRA_RATE_LENGTH + 1)) |
        (HMC5883L_BIAS_NORMAL << (HMC5883L_CRA_BIAS_BIT - HMC5883L_CRA_BIAS_LENGTH + 1)));

    // write CONFIG_A register
    HMC5883L_I2C_ByteWrite(slaveAddr, buffer, HMC5883L_RA_CONFIG_A);
    
    // write CONFIG_B register
    HMC5883L_setGain(HMC5883L_GAIN_1090);

    // write MODE register
    HMC5883L_setMode(HMC5883L_MODE_SINGLE);
}

// CONFIG_A register

/** Get number of samples averaged per measurement.
 * @return Current samples averaged per measurement (0-3 for 1/2/4/8 respectively)
 * @see HMC5883L_AVERAGING_8
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_AVERAGE_BIT
 * @see HMC5883L_CRA_AVERAGE_LENGTH
 */
uint8_t HMC5883L_getSampleAveraging() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    //extract bits 6 and 7 from configuration register A
    uint8_t mask=((1 << HMC5883L_CRA_AVERAGE_LENGTH) - 1) << (HMC5883L_CRA_AVERAGE_BIT - HMC5883L_CRA_AVERAGE_LENGTH + 1);  //AKA 0x01100000 average bits in reg A
    buffer[0] &=mask;   //apply mask to get value of average bits
    buffer[0] >>= (HMC5883L_CRA_AVERAGE_BIT - HMC5883L_CRA_AVERAGE_LENGTH + 1); //reshift right to get decimal value
    return buffer[0];
}
/** Set number of samples averaged per measurement.
 * @param averaging New samples averaged per measurement setting(0-3 for 1/2/4/8 respectively)
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_AVERAGE_BIT
 * @see HMC5883L_CRA_AVERAGE_LENGTH
 */
void HMC5883L_setSampleAveraging(uint8_t averaging) {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    uint8_t mask = ((1 << HMC5883L_CRA_AVERAGE_LENGTH) - 1) << (HMC5883L_CRA_AVERAGE_BIT - HMC5883L_CRA_AVERAGE_LENGTH + 1);
    averaging <<= (HMC5883L_CRA_AVERAGE_BIT - HMC5883L_CRA_AVERAGE_LENGTH + 1); // shift data into correct position
    averaging &= mask; // zero all non-important bits in data
    buffer[0] &= ~(mask); // zero all important bits in existing byte
    buffer[0] |= averaging; // combine data with existing byte
    HMC5883L_I2C_ByteWrite(slaveAddr, buffer, HMC5883L_RA_CONFIG_A);
}
/** Get data output rate value.
 * The Table below shows all selectable output rates in continuous measurement
 * mode. All three channels shall be measured within a given output rate. Other
 * output rates with maximum rate of 160 Hz can be achieved by monitoring DRDY
 * interrupt pin in single measurement mode.
 *
 * Value | Typical Data Output Rate (Hz)
 * ------+------------------------------
 * 0     | 0.75
 * 1     | 1.5
 * 2     | 3
 * 3     | 7.5
 * 4     | 15 (Default)
 * 5     | 30
 * 6     | 75
 * 7     | Not used
 *
 * @return Current rate of data output to registers
 * @see HMC5883L_RATE_15
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_RATE_BIT
 * @see HMC5883L_CRA_RATE_LENGTH
 */
uint8_t HMC5883L_getDataRate() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    //extract bits 6 and 7 from configuration register
    uint8_t mask=((1 << HMC5883L_CRA_RATE_LENGTH) - 1) << (HMC5883L_CRA_RATE_BIT - HMC5883L_CRA_RATE_LENGTH + 1);  //AKA 0x00011100
    buffer[0] &=mask;   //apply mask
    buffer[0] >>= (HMC5883L_CRA_RATE_BIT - HMC5883L_CRA_RATE_LENGTH + 1); //reshift right to get decimal value
    return buffer[0];
}
/** Set data output rate value.
 * @param rate Rate of data output to registers
 * @see getDataRate()
 * @see HMC5883L_RATE_15
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_RATE_BIT
 * @see HMC5883L_CRA_RATE_LENGTH
 */
void HMC5883L_setDataRate(uint8_t rate) {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    uint8_t mask = ((1 << HMC5883L_CRA_RATE_LENGTH) - 1) << (HMC5883L_CRA_RATE_BIT - HMC5883L_CRA_RATE_LENGTH + 1);
    rate <<= (HMC5883L_CRA_RATE_BIT - HMC5883L_CRA_RATE_LENGTH + 1); // shift data into correct position
    rate &= mask; // zero all non-important bits in data
    buffer[0] &= ~(mask); // zero all important bits in existing byte
    buffer[0] |= rate; // combine data with existing byte
    HMC5883L_I2C_ByteWrite(slaveAddr, buffer, HMC5883L_RA_CONFIG_A);
}
/** Get measurement bias value.
 * @return Current bias value (0-2 for normal/positive/negative respectively)
 * @see HMC5883L_BIAS_NORMAL
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_BIAS_BIT
 * @see HMC5883L_CRA_BIAS_LENGTH
 */
uint8_t HMC5883L_getMeasurementBias() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    //extract bits 6 and 7 from configuration register
    uint8_t mask=((1 << HMC5883L_CRA_BIAS_LENGTH) - 1) << (HMC5883L_CRA_BIAS_BIT - HMC5883L_CRA_BIAS_LENGTH + 1);  //AKA 0x00000011
    buffer[0] &=mask;   //apply mask
    buffer[0] >>= (HMC5883L_CRA_BIAS_BIT - HMC5883L_CRA_BIAS_LENGTH + 1); //reshift right to get decimal value
    return buffer[0];
}
/** Set measurement bias value.
 * @param bias New bias value (0-2 for normal/positive/negative respectively)
 * @see HMC5883L_BIAS_NORMAL
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_BIAS_BIT
 * @see HMC5883L_CRA_BIAS_LENGTH
 */
void HMC5883L_setMeasurementBias(uint8_t bias) {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    uint8_t mask = ((1 << HMC5883L_CRA_BIAS_LENGTH) - 1) << (HMC5883L_CRA_BIAS_BIT - HMC5883L_CRA_BIAS_LENGTH + 1);
    bias <<= (HMC5883L_CRA_BIAS_BIT - HMC5883L_CRA_BIAS_LENGTH + 1); // shift data into correct position
    bias &= mask; // zero all non-important bits in data
    buffer[0] &= ~(mask); // zero all important bits in existing byte
    buffer[0] |= bias; // combine data with existing byte
    HMC5883L_I2C_ByteWrite(slaveAddr, buffer, HMC5883L_RA_CONFIG_A);
}

// CONFIG_B register

/** Get magnetic field gain value.
 * The table below shows nominal gain settings. Use the "Gain" column to convert
 * counts to Gauss. Choose a lower gain value (higher GN#) when total field
 * strength causes overflow in one of the data output registers (saturation).
 * The data output range for all settings is 0xF800-0x07FF (-2048 - 2047).
 *
 * Value | Field Range | Gain (LSB/Gauss)
 * ------+-------------+-----------------
 * 0     | +/- 0.88 Ga | 1370
 * 1     | +/- 1.3 Ga  | 1090 (Default)
 * 2     | +/- 1.9 Ga  | 820
 * 3     | +/- 2.5 Ga  | 660
 * 4     | +/- 4.0 Ga  | 440
 * 5     | +/- 4.7 Ga  | 390
 * 6     | +/- 5.6 Ga  | 330
 * 7     | +/- 8.1 Ga  | 230
 *
 * @return Current magnetic field gain value
 * @see HMC5883L_GAIN_1090
 * @see HMC5883L_RA_CONFIG_B
 * @see HMC5883L_CRB_GAIN_BIT
 * @see HMC5883L_CRB_GAIN_LENGTH
 */
uint8_t HMC5883L_getGain() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_B, 1);
    //extract bits 6 and 7 from configuration register
    uint8_t mask=((1 << HMC5883L_CRB_GAIN_LENGTH) - 1) << (HMC5883L_CRB_GAIN_BIT - HMC5883L_CRB_GAIN_LENGTH + 1);  //AKA 0x00000011
    buffer[0] &=mask;   //apply mask
    buffer[0] >>= (HMC5883L_CRB_GAIN_BIT - HMC5883L_CRB_GAIN_LENGTH + 1); //reshift right to get decimal value
    return buffer[0];
}
/** Set magnetic field gain value.
 * @param gain New magnetic field gain value
 * @see getGain()
 * @see HMC5883L_RA_CONFIG_B
 * @see HMC5883L_CRB_GAIN_BIT
 * @see HMC5883L_CRB_GAIN_LENGTH
 */
void HMC5883L_setGain(uint8_t gain) {
    //Since in register B bits 0,1,2,3,4 needs to be cleared we can simply send the
    //value of the bits 5,6,7 in a byte with others set to 0
    buffer[0]=gain << (HMC5883L_CRB_GAIN_BIT - HMC5883L_CRB_GAIN_LENGTH + 1);
    HMC5883L_I2C_ByteWrite(slaveAddr, buffer, HMC5883L_RA_CONFIG_B);
}

// MODE register

/** Get measurement mode.
 * In continuous-measurement mode, the device continuously performs measurements
 * and places the result in the data register. RDY goes high when new data is
 * placed in all three registers. After a power-on or a write to the mode or
 * configuration register, the first measurement set is available from all three
 * data output registers after a period of 2/fDO and subsequent measurements are
 * available at a frequency of fDO, where fDO is the frequency of data output.
 *
 * When single-measurement mode (default) is selected, device performs a single
 * measurement, sets RDY high and returned to idle mode. Mode register returns
 * to idle mode bit values. The measurement remains in the data output register
 * and RDY remains high until the data output register is read or another
 * measurement is performed.
 *
 * @return Current measurement mode
 * @see HMC5883L_MODE_CONTINUOUS
 * @see HMC5883L_MODE_SINGLE
 * @see HMC5883L_MODE_IDLE
 * @see HMC5883L_RA_MODE
 * @see HMC5883L_MODEREG_BIT
 * @see HMC5883L_MODEREG_LENGTH
 */
uint8_t HMC5883L_getMode() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_CONFIG_A, 1);
    //extract bits 6 and 7 from configuration register
    uint8_t mask=((1 << HMC5883L_MODEREG_LENGTH) - 1) << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1);  //AKA 0x00000011
    buffer[0] &=mask;   //apply mask
    buffer[0] >>= (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1); //reshift right to get decimal value
    return buffer[0];
}
/** Set measurement mode.
 * @param newMode New measurement mode
 * @see getMode()
 * @see HMC5883L_MODE_CONTINUOUS
 * @see HMC5883L_MODE_SINGLE
 * @see HMC5883L_MODE_IDLE
 * @see HMC5883L_RA_MODE
 * @see HMC5883L_MODEREG_BIT
 * @see HMC5883L_MODEREG_LENGTH
 */
void HMC5883L_setMode(uint8_t newMode) {
    //Since in register B bits 7,6,5,4,3,2 needs to be cleared we can simply send the
    //value of the bits 0,1 in a byte with others set to 0
    buffer[0]=newMode << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1);
    HMC5883L_I2C_ByteWrite(slaveAddr, buffer, HMC5883L_RA_MODE);
    mode = newMode; // track to tell if we have to clear bit 7 after a read
}

// DATA* registers

/** Get 3-axis heading measurements.
 * In the event the ADC reading overflows or underflows for the given channel,
 * or if there is a math overflow during the bias measurement, this data
 * register will contain the value -4096. This register value will clear when
 * after the next valid measurement is made. Note that this method automatically
 * clears the appropriate bit in the MODE register if Single mode is active.
 * @param x 16-bit signed integer container for X-axis heading
 * @param y 16-bit signed integer container for Y-axis heading
 * @param z 16-bit signed integer container for Z-axis heading
 * @see HMC5883L_RA_DATAX_H
 */
void HMC5883L_getHeading(int16_t *x, int16_t *y, int16_t *z) {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_DATAX_H, 6);

    uint8_t tmp;
    tmp=HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1);
    if (mode == HMC5883L_MODE_SINGLE){
    HMC5883L_I2C_ByteWrite(slaveAddr, &tmp, HMC5883L_RA_MODE);
    }
    *x = (((int16_t)buffer[0]) << 8) | buffer[1];
    *y = (((int16_t)buffer[4]) << 8) | buffer[5];
    *z = (((int16_t)buffer[2]) << 8) | buffer[3];
}
/** Get X-axis heading measurement.
 * @return 16-bit signed integer with X-axis heading
 * @see HMC5883L_RA_DATAX_H
 */
int16_t HMC5883L_getHeadingX() {
    // each axis read requires that ALL axis registers be read, even if only
    // one is used; this was not done ineffiently in the code by accident
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_DATAX_H, 6);

    uint8_t tmp;
    tmp=HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1);
    if (mode == HMC5883L_MODE_SINGLE){
    HMC5883L_I2C_ByteWrite(slaveAddr, &tmp, HMC5883L_RA_MODE);
    }
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
/** Get Y-axis heading measurement.
 * @return 16-bit signed integer with Y-axis heading
 * @see HMC5883L_RA_DATAY_H
 */
int16_t HMC5883L_getHeadingY() {
    // each axis read requires that ALL axis registers be read, even if only
    // one is used; this was not done ineffiently in the code by accident
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_DATAX_H, 6);

    uint8_t tmp;
    tmp=HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1);
    if (mode == HMC5883L_MODE_SINGLE){
    HMC5883L_I2C_ByteWrite(slaveAddr, &tmp, HMC5883L_RA_MODE);
    }
    return (((int16_t)buffer[4]) << 8) | buffer[5];
}
/** Get Z-axis heading measurement.
 * @return 16-bit signed integer with Z-axis heading
 * @see HMC5883L_RA_DATAZ_H
 */
int16_t HMC5883L_getHeadingZ() {
    // each axis read requires that ALL axis registers be read, even if only
    // one is used; this was not done ineffiently in the code by accident
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_DATAX_H, 6);

    uint8_t tmp;
    tmp=HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1);
    if (mode == HMC5883L_MODE_SINGLE){
    HMC5883L_I2C_ByteWrite(slaveAddr, &tmp, HMC5883L_RA_MODE);
    }
    return (((int16_t)buffer[2]) << 8) | buffer[3];
}

// STATUS register

/** Get data output register lock status.
 * This bit is set when this some but not all for of the six data output
 * registers have been read. When this bit is set, the six data output registers
 * are locked and any new data will not be placed in these register until one of
 * three conditions are met: one, all six bytes have been read or the mode
 * changed, two, the mode is changed, or three, the measurement configuration is
 * changed.
 * @return Data output register lock status
 * @see HMC5883L_RA_STATUS
 * @see HMC5883L_STATUS_LOCK_BIT
 */
bool HMC5883L_getLockStatus() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_STATUS, 1);

	buffer[0] = (buffer[0] >> HMC5883L_STATUS_LOCK_BIT) & 0x01; //shift and AND to get bool value

    return buffer[0];
}
/** Get data ready status.
 * This bit is set when data is written to all six data registers, and cleared
 * when the device initiates a write to the data output registers and after one
 * or more of the data output registers are written to. When RDY bit is clear it
 * shall remain cleared for 250 us. DRDY pin can be used as an alternative to
 * the status register for monitoring the device for measurement data.
 * @return Data ready status
 * @see HMC5883L_RA_STATUS
 * @see HMC5883L_STATUS_READY_BIT
 */
bool HMC5883L_getReadyStatus() {
    HMC5883L_I2C_BufferRead(slaveAddr, buffer, HMC5883L_RA_STATUS, 1);

	buffer[0] = (buffer[0] >> HMC5883L_STATUS_READY_BIT) & 0x01; //shift and AND to get bool value

    return buffer[0];
}


