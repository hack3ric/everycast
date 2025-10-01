#ifndef EVERYCAST_NETNS_H
#define EVERYCAST_NETNS_H

int netns_create(int* orig, int* new);
int netns_set(int netns);
long netns_if_nametoindex(int netns, const char* ifname);

#endif  // EVERYCAST_NETNS_H
