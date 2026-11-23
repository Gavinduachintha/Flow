
// #include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
int get_cpu_temperature(double *temperature);
int get_fan_speed(int *fan_speed);
double get_cpu_frequency(double *frequency);

int main(int argc, char **argv){
    int running = 1;
    while (running){
        double temperature;
        int fan_speed;
        double cpu_frequency;

        if (get_cpu_temperature(&temperature) != 0) {
            printf("Failed to read CPU temperature\n");
            return 1;
        }
        if(get_fan_speed(&fan_speed)!=0){
            printf("Failed to read the fan speed\n");
            return 1;
        }
        if(get_cpu_frequency(&cpu_frequency)!=0){
            printf("Failed to read the frequency\n");
            return 1;
        }

        printf("CPU Temperature: %.2f °C\n", temperature);
        printf("Fan Speed: %d\n",fan_speed);
        printf("CPU Frequncy: %f\n",cpu_frequency);
        sleep(2);
    }
    
    return 0;

}
