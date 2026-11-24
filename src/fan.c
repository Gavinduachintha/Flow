#include <stdio.h>

int get_fan_speed(int *fan_speed){
    FILE *file = fopen("/sys/class/hwmon/hwmon2/fan1_input","r");
    if(file == NULL){
        return -1;
    }
    int raw_speed;
    if(fscanf(file, "%d",&raw_speed)!=1){
        fclose(file);
        return -1;
    }
    fclose(file);
    *fan_speed = raw_speed;
    return 0;
}

int set_fan_speed(int fan_speed)
{
    if (fan_speed < 0) {
        fan_speed = 0;
    } else if (fan_speed > 255) {
        fan_speed = 255;
    }

    FILE *file = fopen(
        "/sys/class/hwmon/hwmon2/pwm1",
        "w"
    );

    if (file == NULL) {
        return -1;
    }

    int result = fprintf(file, "%d", fan_speed);
    if (fclose(file) != 0 || result < 0) {
        return -1;
    }

    return 0;
}