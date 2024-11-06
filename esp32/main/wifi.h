
#ifndef WIFI_H
#define WIFI_H

#define CHANNEL 6

int wifi_start_ap(const char *ssid, const char *password);
int wifi_start_client(const char *ssid, const char *password);

#endif

