/**
 * Copyright (c) 2019, Jan Jongboom
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"

#ifndef PMS5003_H_
#define PMS5003_H_

typedef struct {
    uint16_t pm10_standard, pm25_standard, pm100_standard;
    uint16_t pm10_env, pm25_env, pm100_env;
    uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
} pms5003_data_t;

class PMS5003 {
public:
    PMS5003(PinName tx, PinName rx, PinName power, PinName reset, EventQueue *queue) :
        _serial(tx, rx),
        _power(power),
        _reset(reset),
        _buffer_ix(0),
        _in_valid_packet(false),
        _queue(queue)
    {
        memset(_buffer, 0, sizeof(_buffer));

        _serial.baud(9600);
        _serial.format(8, Serial::None, 1);

        _power = 0;
    }

    /**
     * Enable the sensor.
     * Note that this will disable deep sleep on the board.
     *
     * @param pkt_callback This will fire from an interrupt context whenever a packet is complete
     */
    void enable(Callback<void(pms5003_data_t)> pkt_callback) {
        _power = 1;

        _serial.attach(callback(this, &PMS5003::rx_irq), SerialBase::RxIrq);

        _callback = pkt_callback;
    }

    /**
     * Disable the sensor.
     * This will re-enable deep sleep on the board.
     */
    void disable() {
        _power = 0;

        _serial.attach(NULL);

        _callback = NULL;
    }

private:
    /**
     * IRQ handler when a package is received
     */
    void rx_irq() {
        if (!_serial.readable()) return;

        while (_serial.readable()) {
            char c = _serial.getc();

            // new frame
            // actually a frame is denoted of 0x42, 0x4d
            if (c == 0x42) {
                _queue->call(printf, "new frame\n");
                memset(_buffer, 0, sizeof(_buffer));
                _buffer_ix = 0;
                _in_valid_packet = true;
            }

            if (_buffer_ix == 1 && c != 0x4d) {
                // something is wrong...
                _queue->call(printf, "invalid second byte (%02x)\n", c);
                _in_valid_packet = false;
            }

            if (_in_valid_packet && _buffer_ix < sizeof(_buffer)) {
                _buffer[_buffer_ix] = static_cast<uint8_t>(c);

                _buffer_ix++;
            }

            if (_in_valid_packet && _buffer[_buffer_ix] == sizeof(_buffer)) {
                uint16_t frame_length = (_buffer[2] << 8) + _buffer[3];
                if (frame_length != 28) {
                    _queue->call(printf, "Frame length not correct (was %u)\n", frame_length);
                    return;
                }

                // verify checksum
                uint16_t expected_checksum = (_buffer[30] << 8) + _buffer[31];

                uint16_t checksum = 0;
                for (uint8_t ix = 0; ix < 30; ix++) {
                    checksum += _buffer[ix];
                }

                if (checksum != expected_checksum) {
                    _queue->call(printf, "Checksum not correct (was %u, but expected %u)\n", checksum, expected_checksum);
                    return;
                }

                // Everything OK, let's decode
                pms5003_data_t data;
                data.pm10_standard = (_buffer[4] << 8) + _buffer[5];
                data.pm25_standard = (_buffer[6] << 8) + _buffer[7];
                data.pm100_standard = (_buffer[8] << 8) + _buffer[9];
                data.pm10_env = (_buffer[10] << 8) + _buffer[11];
                data.pm25_env = (_buffer[12] << 8) + _buffer[13];
                data.pm100_env = (_buffer[14] << 8) + _buffer[15];
                data.particles_03um = (_buffer[16] << 8) + _buffer[17];
                data.particles_05um = (_buffer[18] << 8) + _buffer[19];
                data.particles_10um = (_buffer[20] << 8) + _buffer[21];
                data.particles_25um = (_buffer[22] << 8) + _buffer[23];
                data.particles_50um = (_buffer[24] << 8) + _buffer[25];
                data.particles_100um = (_buffer[26] << 8) + _buffer[27];
                // field 13 is unused

                if (_callback) {
                    _callback(data);
                }
            }
        }
    }

    RawSerial _serial;
    DigitalOut _power;
    DigitalOut _reset;

    uint8_t _buffer[32];
    uint8_t _buffer_ix;
    bool _in_valid_packet;

    EventQueue *_queue;

    Callback<void(pms5003_data_t)> _callback;
};

#endif // PMS5003_H_
