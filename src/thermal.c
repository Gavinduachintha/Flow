#include <stdio.h>

int get_cpu_temperature(double *temperature)
{
    FILE *file = fopen(
        "/sys/class/thermal/thermal_zone0/temp",
        "r"
    );

    if (file == NULL) {
        return -1;
    }

    int raw_temperature;

    if (fscanf(file, "%d", &raw_temperature) != 1) {
        fclose(file);
        return -1;
    }

    fclose(file);

    *temperature = raw_temperature / 1000.0;

    return 0;
}