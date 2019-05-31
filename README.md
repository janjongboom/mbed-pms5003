# Mbed OS PMS5003 driver

Mbed OS 5 driver for the [PLANTOWER PMS5003](http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf) digital universal particle concentration sensor. This device is also sold as the [Adafruit PM2.5 Air Quality sensor](https://learn.adafruit.com/pm25-air-quality-sensor).

## Usage

You'll need two pins: UART TX, UART RX. Additionally you can specify a power pin, in case you have a power switch in your design. This is recommended to shut down the sensor when it's not being used. If you don't have a power pin just mark it as `NC`.

```cpp
#include "PMS3005.h"

PMS5003 pm25(D8, D2, D3); // UART TX, UART RX, POWER

static EventQueue queue; // The callback from the driver is triggered from IRQ, use EventQueue to debounce to normal context

void pm25_data_callback(pms5003_data_t data) {
    printf("---------------------------------------\n");
    printf("Concentration Units (standard)\n");
    printf("PM 1.0: %u", data.pm10_standard);
    printf("\t\tPM 2.5: %u", data.pm25_standard);
    printf("\t\tPM 10: %u\n", data.pm100_standard);
    printf("---------------------------------------\n");
    printf("Concentration Units (environmental)\n");
    printf("PM 1.0: %u", data.pm10_env);
    printf("\t\tPM 2.5: %u", data.pm25_env);
    printf("\t\tPM 10: %u\n", data.pm100_env);
    printf("---------------------------------------\n");
    printf("Particles > 0.3um / 0.1L air: %u\n", data.particles_03um);
    printf("Particles > 0.5um / 0.1L air: %u\n", data.particles_05um);
    printf("Particles > 1.0um / 0.1L air: %u\n", data.particles_10um);
    printf("Particles > 2.5um / 0.1L air: %u\n", data.particles_25um);
    printf("Particles > 5.0um / 0.1L air: %u\n", data.particles_50um);
    printf("Particles > 10.0 um / 0.1L air: %u\n", data.particles_100um);
    printf("---------------------------------------\n");
}

int main() {
    printf("Hello world\n");

    // This callback runs in an interrupt context, thus we debounce to the event queue here
    pm25.enable(queue.event(&pm25_data_callback));

    queue.dispatch_forever();
}
```
