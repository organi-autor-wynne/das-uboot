// ----------------------------------------------------------------------------
//         ATMEL Microcontroller Software Support  -  Colorado Springs, CO -
// ----------------------------------------------------------------------------
// DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
// DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------
/** \file
 *  \brief Definitions for Hardware Dependent Part of SHA204 Physical Layer
 *         Using GPIO for Communication
 *  \author Atmel Crypto Products
 *  \date October 21, 2010
 */
#ifndef BITBANG_CONFIG_H
#   define BITBANG_CONFIG_H

#define swi_enable_interrupts   //!< enable interrupts
#define swi_disable_interrupts  //!< disable interrupts

#define PORT_OUT         (gpio_direction_output(IMX_GPIO_NR(3, 24), 1))    //!< output port register for device id 0
#define PORT_IN          (gpio_direction_input(IMX_GPIO_NR(3, 24)))     //!< input port register for device id 0
#define PORT_DATPH         (gpio_set_value(IMX_GPIO_NR(3, 24), 1))   
#define PORT_DATPL         (gpio_set_value(IMX_GPIO_NR(3, 24), 0))   
#define PORT_DATG          (gpio_get_value(IMX_GPIO_NR(3, 24)))   


// debug pin that indicates pulse edge detection. Only enabled if compilation switch _DEBUG is used.
// To debug timing, disable host power (H1 and H2 on CryptoAuth daughter board) and connect logic analyzer
// or storage oscilloscope to the H2 pin that is closer to the H1 header.
#ifdef DEBUG_BITBANG
#   define DEBUG_PORT_DDR  (DDRB)                              //!< direction register for debug pin
#   define DEBUG_PORT_OUT  (PORTB)                             //!< output port register for debug pin
#   define DEBUG_BIT       (6)                                 //!< what pin to use for debugging
#   define DEBUG_LOW       DEBUG_PORT_OUT &= ~_BV(DEBUG_BIT)   //!< set debug pin low
#   define DEBUG_HIGH      DEBUG_PORT_OUT |= _BV(DEBUG_BIT)    //!< set debug pin high
#else
#   define DEBUG_LOW
#   define DEBUG_HIGH
#endif

// time to drive bits at 230.4 kbps
// Using the value below we measured 4340 ns with logic analyzer (10 ns resolution).
//! time it takes to toggle the pin at CPU clock of 16 MHz (ns)
#define PORT_ACCESS_TIME   (630)
//! width of start pulse (ns)
#define START_PULSE_WIDTH  (4340)
//! delay macro for width of one pulse (start pulse or zero pulse, in ns)
#define BIT_DELAY_1       udelay(2)// _delay_ns(START_PULSE_WIDTH - PORT_ACCESS_TIME)
//! time to keep pin high for five pulses plus stop bit (used to bit-bang CryptoAuth 'zero' bit, in ns)
#define BIT_DELAY_5        udelay(5*2)// _delay_ns(6 * START_PULSE_WIDTH - PORT_ACCESS_TIME)
//! time to keep pin high for seven bits plus stop bit (used to bit-bang CryptoAuth 'one' bit)
#define BIT_DELAY_7        udelay(7*2)//_delay_ns(7 * START_PULSE_WIDTH - PORT_ACCESS_TIME)
//! turn around time when switching from receive to transmit
#define RX_TX_DELAY        udelay(15)

// One loop iteration for edge detection takes about 0.6 us on this hardware.
// Lets set the timeout value for start pulse detection to the uint8_t maximum.
/** \brief This value is decremented while waiting for the falling edge of a start pulse. */
#define START_PULSE_TIME_OUT  (255)

// We measured a loop count of 8 for the start pulse. That means it takes about
// 0.6 us per loop iteration. Maximum time between rising edge of start pulse
// and falling edge of zero pulse is 8.6 us. Therefore, a value of 26 (around 15 us)
// gives ample time to detect a zero pulse and also leaves enough time to detect
// the following start pulse.
/** \brief This value is decremented while waiting for the falling edge of a zero pulse. */
#define ZERO_PULSE_TIME_OUT    (26)

#endif
