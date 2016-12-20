#include <Ice/Ice.h>
#include "StreamServer.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

using namespace FCUP;

StreamsMap list_of_stream_servers;


class Portal : public PortalCommunication
{
public:
    void registerStreamServer(const FCUP::StreamServerEntry&, const Ice::Current&) override;
    void closeStream(const std::string&, const Ice::Current&) override;

    StreamsMap sendStreamServersList(const Ice::Current&) override;
};

void Portal::registerStreamServer(const FCUP::StreamServerEntry& sse, const Ice::Current&)
{
    list_of_stream_servers[sse.name] = sse;

    std::cout << "I'm a portal printing some keywords for the lulz" << std::endl;
    for (auto it = sse.keywords.begin(); it != sse.keywords.end(); ++it){
        std::cout << *it << ' ';
    }
    std::cout << std::endl << "Bye" << std::endl;

}

void Portal::closeStream(const std::string& serverName, const Ice::Current&) {
    std::cout << "Going to close the stream -> " << serverName << std::endl;
    auto elem = list_of_stream_servers.find(serverName);
    if(elem != list_of_stream_servers.end()){
        list_of_stream_servers.erase(elem);
        std::cout << "Closed stream -> " << serverName << std::endl;
    }
    else{
        std::cout << "Couldn't close/find stream -> " << serverName << std::endl;
    }
}


StreamsMap Portal::sendStreamServersList(const Ice::Current&)
{
    return list_of_stream_servers;
}


int main(int argc, char* argv[])
{
    int status = 0;
    Ice::CommunicatorPtr ic;
    try {
        ic = Ice::initialize(argc, argv);
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("PortalAdapter", "default -p 9999");
        Ice::ObjectPtr object = new Portal;
        adapter->add(object, ic->stringToIdentity("Portal"));
        adapter->activate();
        ic->waitForShutdown();
    } catch (const Ice::Exception& e) {
        std::cerr << e << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }
    if (ic) {
        try {
            ic->destroy();
        } catch (const Ice::Exception& e) {
            std::cerr << e << std::endl;
            status = 1;
        }
    }
    return status;
}
