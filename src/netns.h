#ifndef EVERYCAST_NETNS_H
#define EVERYCAST_NETNS_H

// Create and join new netns, returning fd of both original and new netns.
int create_netns(int* orig, int* new);

#endif  // EVERYCAST_NETNS_H
