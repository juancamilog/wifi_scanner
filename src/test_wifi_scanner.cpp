#include "wifi_scanner.h"
#include <thread>
#include <chrono>

std::string iface ="wlan0";
std::chrono::time_point<std::chrono::system_clock> start;
std::chrono::duration<double> elapsed;

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
    std::function<scanning_callback> cb = [&elapsed,&start](access_point &ap){
         std::cout<<"Got AP:\t mac: "<<ap.mac_address<<std::endl;
         std::cout<<"        \t time: "<<ap.timestamp<<std::endl;
         std::cout<<"        \t freq: "<<ap.frequency<<std::endl;
         std::cout<<"        \t signal: "<<ap.signal_strength<<std::endl;
         std::cout<<"        \t noise: "<<ap.signal_noise<<std::endl;
         std::cout<<"        \t essid: "<<ap.essid<<std::endl;
         elapsed = std::chrono::system_clock::now() - start;
         std::cout<<"Elapsed time: "<<elapsed.count()<<std::endl;
    };
    wifi_scanner ws;
    std::chrono::milliseconds sleep_duration(2000);
    start = std::chrono::system_clock::now();
    if (ws.init(iface, cb)){
        while (true){
            ws.scan();
            std::this_thread::sleep_for(sleep_duration);
        }
    } else {
        std::cout<<"Could not open socket on wlan0"<<std::endl;
    }
    
    return 0;
}
