#include "wifi_scanner.h"
#include <errno.h>
access_point::access_point(){

}

wifi_scanner::wifi_scanner(){
}

wifi_scanner::~wifi_scanner(){
    if (socket_fd > 0)
        iw_sockets_close(socket_fd);
}

bool wifi_scanner::init(std::string interface, const std::function<scanning_callback> &cb){
    socket_fd = iw_sockets_open();
    interface_name = interface;
    if (cb)
        scan_callback = cb;
         
    if(socket_fd < 0){
        std::cout<<"Could not open socket. Errno:"<<errno<<std::endl;
        return false;
    }
    if (iw_get_range_info(socket_fd, interface_name.c_str(), &range) < 0){
        std::cout<<"Could not start scanning, iw_get_range_info failed. Errno:"<<errno<<std::endl;
        return false;
    }
    return true;
}

int wifi_scanner::scan(){
    struct iwreq      wrq;
    unsigned char *   buffer = NULL;      /* Results */
    int               buflen = IW_SCAN_MAX_DATA; /* Min for compat WE<17 */
    struct timeval    tv;             /* Select timeout */
    int               timeout = 15000000;     /* 15s */
    int               scanflags = IW_SCAN_ALL_ESSID | IW_SCAN_ALL_FREQ | IW_SCAN_ALL_MODE | IW_SCAN_ALL_RATE;		/* Flags for scan */

    wrq.u.data.pointer = NULL;
    wrq.u.data.length = 0;
    wrq.u.data.length = 0;

    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    if(iw_set_ext(socket_fd, interface_name.c_str(), SIOCSIWSCAN, &wrq) < 0){
        std::cout<<"Could not start scan: ";
        if (errno == EPERM){
            std::cout<<"Insufficient permissions. THE REPORTED RESULTS WILL BE LEFT_OVERS FROM PREVIOUS SCANS";
            std::cout<<std::endl;
        }
        else{
            if (errno == EBUSY)
                std::cout<<"Device or resource busy.";
            else
                std::cout<<"iw_set_ext failed. errno:"<<errno<<std::endl;
            std::cout<<std::endl;
            return -1;
        }
    }

    timeout -= tv.tv_usec;
    bool waiting_for_scan_results = true;
    while(waiting_for_scan_results) {
        fd_set    rfds;
        int       last_fd;
        int       ret;
        FD_ZERO(&rfds);
        last_fd = -1;
        ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);
        if(ret < 0){
            if(errno == EAGAIN || errno == EINTR)
	            continue;
	        std::cout<<"Unhandled signal - exiting..."<<std::endl;
	        return -1;
	    }
        if(ret == 0){
            unsigned char *newbuf;
            while(waiting_for_scan_results){
                newbuf = (unsigned char*)realloc(buffer, buflen);
                if(newbuf == NULL){
                    if(buffer)
                        free(buffer);
                    fprintf(stderr, "%s: Allocation failed", __FUNCTION__);
	                std::cout<<"Unhandled signal - exiting..."<<std::endl;
                    return -1;
                }
                buffer = newbuf;
                // Try to read the results
                wrq.u.data.pointer = buffer;
                wrq.u.data.flags = scanflags;
                wrq.u.data.length = buflen;
                // send a scan request
                if(iw_set_ext(socket_fd, interface_name.c_str(), SIOCGIWSCAN, &wrq) < 0){
                    //process errors
                    // If the buffer was too small to store the scan results, 
                    // we will double the size of the buffr and try again
                    if((errno == E2BIG) && (range.we_version_compiled > 16)){
                        if(wrq.u.data.length > buflen)
                            buflen = wrq.u.data.length;
                        else
                            buflen *= 2;
                        continue;
                    }
                    // If results are not available, restart timer and try again
                    if(errno == EAGAIN){
                        /* Restart timer for only 100ms*/
                        tv.tv_sec = 0;
                        tv.tv_usec = 250000;
                        timeout -= tv.tv_usec;
                        if( timeout <0){
	                        std::cout<<"Timed out while waiting for scan resutls"<<std::endl;
                            waiting_for_scan_results = false;
                        }
                        break;
                    }
                    // if we got here, there was some error
                    if(buffer)
                        free(buffer);
                    std::cout<<"Failed to read scan data, errno: "<<errno<<std::endl;
                    return -1;
                }
                else
                    /* We have the results, go to process them */
                    waiting_for_scan_results = false;
            }
        }
    }
    // process the scan data
    if(wrq.u.data.length){
        struct iw_event       iwe;
        struct stream_descr   stream;
        int           ret;
        access_point *ap=0;

        iw_init_event_stream(&stream, (char *) buffer, wrq.u.data.length);
        std::chrono::duration<double> scan_time;
        scan_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
        do{
            /* Extract an event and print it */
            ret = iw_extract_event_stream(&stream, &iwe,range.we_version_compiled);
            if(ret > 0)
                process_iw_event(&stream, &iwe, &ap, scan_time.count());
        }while(ret > 0);
        // check if there is one last ap
        if(ap){
            ap_list[ap->mac_address] = *ap;
            scan_callback(*ap);
        }
    }
    free(buffer);
    return(0);
}

void wifi_scanner::process_iw_event(struct stream_descr *	stream,	/* Stream of events */
        struct iw_event *		event,	/* Extracted token */
        access_point** ap,
        double scan_time){
    char    buffer[128];    /* Temporary buffer */
     
    /* Now, let's decode the event */
    switch(event->cmd) {
        case SIOCGIWAP:{
            // if ap already points to something, then it means we finished processing an AP
            if(*ap){
                ap_list[(*ap)->mac_address] = *(*ap);
                scan_callback(*(*ap));
            }
            std::string mac_addr = std::string(get_mac_address(&event->u.ap_addr, buffer));
            // if an AP with the given mac address already exists in our map, then just update
            if (ap_list.find(mac_addr)!=ap_list.end()){
                *ap = &ap_list[mac_addr];
            }
            // New AP in the stream!
            else {
                *ap = new access_point();
                (*ap)->mac_address = mac_addr;
                (*ap)->max_quality = range.max_qual.qual;
                (*ap)->sensitivity = range.sensitivity;
            }
            (*ap)->timestamp = scan_time;
            break;}
        case SIOCGIWFREQ:
	        (*ap)->frequency = iw_freq2float(&(event->u.freq));
            break;
        case IWEVQUAL:
            if (event->u.qual.level != 0 || (event->u.qual.updated & (IW_QUAL_DBM | IW_QUAL_RCPI))) {
              (*ap)->signal_noise = static_cast<signed char>(event->u.qual.noise);
              (*ap)->signal_strength = static_cast<signed char>(event->u.qual.level);
              (*ap)->signal_quality = static_cast<int>(event->u.qual.qual);
              (*ap)->signal_updated = true;
            } else {
              (*ap)->signal_updated = false;
            }
            break;
        case SIOCGIWESSID:
            char essid[IW_ESSID_MAX_SIZE+1];
            memset(essid, '\0', sizeof(essid));
            if((event->u.essid.pointer) && (event->u.essid.length))
                memcpy(essid, event->u.essid.pointer, event->u.essid.length);
            if(event->u.essid.flags)
                (*ap)->essid = std::string(essid);
            else
                (*ap)->essid = "";
            break;
        default:
            break;
    }	/* switch(event->cmd) */
}
