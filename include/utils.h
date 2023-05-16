#ifndef UTILS_H
#define UTILS_H


#define MIDIMAP(x, min, max) (x * (max - min) / 127 + min)
#define MIDIMAPF(x, min, max) ((float)x * (max - min) / (float)127 + min)


float mapf(long x, long in_min, long in_max, long out_min, long out_max)
{
 return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

double mapd(long x, long in_min, long in_max, double out_min, double out_max)
{
 return (double)(x - in_min) * (out_max - out_min) / (double)(in_max - in_min) + out_min;
}


#endif