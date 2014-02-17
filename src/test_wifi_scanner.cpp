#include "wifi_scanner.h"

std::string iface ="wlan0";

void parse_args(int argc,char* argv[]){
    int i =0;
    while (i< argc){
        std::string t = std::string(argv[i]);
        if( t == "-i"|| t == "--interface"){
            iface = std::string(argv[i+1]);
            i=i+2;

        }
        i=i+1;
    }
};

int main(int argc, char* argv[])
{
    parse_args(argc,argv);
    std::function<scanning_callback> cb = [](access_point &ap){
         std::cout<<"Got AP:\t mac: "<<ap.mac_address<<std::endl;
         std::cout<<"        \t time: "<<ap.timestamp<<std::endl;
         std::cout<<"        \t freq: "<<ap.frequency<<std::endl;
         std::cout<<"        \t signal: "<<ap.signal_strength<<std::endl;
         std::cout<<"        \t noise: "<<ap.signal_noise<<std::endl;
         std::cout<<"        \t essid: "<<ap.essid<<std::endl;
    };
    wifi_scanner ws;
    if (ws.init(iface, cb)){
        ws.scan();
    } else {
        std::cout<<"Could not open socket on wlan0"<<std::endl;
    }
    
    return 0;
}
