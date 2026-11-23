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