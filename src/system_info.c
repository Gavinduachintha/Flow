#include <stdio.h>

double get_cpu_frequency(double *frequency){
    FILE *file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq","r");
    if(file == NULL){
        return -1;
    }
    double raw_frequency;
    if(fscanf(file,"%lf",&raw_frequency)!=1){
        fclose(file);
        return -1;
    }
    fclose(file);
    *frequency = raw_frequency/1000000;
    return 0;
}