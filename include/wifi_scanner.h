/*
 * RRTPlanner.h
 *
 *  Created on: 2011-02-19
 *      Author: juancamilo
 */
#ifndef WIFI_LOGGER_H
#define WIFI_LOGGER_H

#include <iostream>
#include <chrono>
#include <map>
#include <string>
#include <functional>
#include "iwlib.h"
#include <chrono>

// ooutputs mac address as a string
static inline char *
get_mac_address(const struct sockaddr *sap, char* bufp)
{
  iw_ether_ntop((const struct ether_addr *) sap->sa_data, bufp);
  return bufp;
};


class access_point{
    public:
       access_point();
       double timestamp;
       std::string mac_address;
       int signal_strength;
       int signal_quality;
       int signal_noise;
       int sensitivity;
       int max_quality;
       bool signal_updated;
       double frequency;
       std::string essid;
};

typedef void scanning_callback( access_point & );

class wifi_scanner{
    public:
        wifi_scanner();
        ~wifi_scanner();
        bool init(std::string, const std::function<scanning_callback> &cb = nullptr);
        //void passive_scan(int milliseconds=1000);
        int scan();
        void process_iw_event(struct stream_descr *	stream, struct iw_event * event, access_point **ap, double scan_time);
        std::map<std::string, access_point> ap_list;

    private:
        int socket_fd;
        struct iw_range   range;
        std::string interface_name;
        std::function<scanning_callback> scan_callback;
};


#endif //WIFI_LOGGER_H
