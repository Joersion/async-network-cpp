#include "Tool.h"

void Tool::htonl2(unsigned long host, char* net) {
    for (int i = 0; i < 4; i++) {
        net[i] = (host >> (8 * (3 - i))) & 0xff;
    }
}

long Tool::ntohl2(const char* net) {
    long host = 0;
    for (int i = 0; i < 4; i++) {
        host += ((unsigned char)net[i]) << (8 * (3 - i));
    }
    return host;
}

void Tool::htons2(unsigned short host, char* net) {
    for (int i = 0; i < 2; i++) {
        net[i] = (host >> (8 * (1 - i))) & 0xff;
    }
}

short Tool::ntohs2(const char* net) {
    short host = 0;
    for (int i = 0; i < 2; i++) {
        host += ((unsigned char)net[i]) << (8 * (1 - i));
    }
    return host;
}