#include "wifi_scanner.h"

int main(){
    std::function<scanning_callback> cb = [](access_point &ap){
         std::cout<<"Got AP:\t mac: "<<ap.mac_address<<std::endl;
         std::cout<<"        \t freq: "<<ap.frequency<<std::endl;
         std::cout<<"        \t signal: "<<ap.signal_strength<<std::endl;
         std::cout<<"        \t noise: "<<ap.signal_noise<<std::endl;
         std::cout<<"        \t essid: "<<ap.essid<<std::endl;
    };
    wifi_scanner ws;
    if (ws.init("wlan0", cb)){
        ws.scan();
    } else {
        std::cout<<"Could not open socket on wlan0"<<std::endl;
    }
    
    return 0;
}
