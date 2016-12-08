#include <Ice/Ice.h>
#include "StreamServer.h"

using namespace FCUP;

StringSequence list_of_stream_servers;

class Portal : public PortalCommunication
{
public:
    void registerStreamServer(const FCUP::StreamServerEntry&, const Ice::Current&) override;
    void closeStream(const Ice::Current&) override;
    void receiveInfo(const Ice::Current&) override;

    StringSequence sendStreamServersList(const Ice::Current&) override;
};

void Portal::registerStreamServer(const FCUP::StreamServerEntry& sse, const Ice::Current&)
{
    std::string server_name = sse.name;
    list_of_stream_servers.push_back(server_name);

    std::cout << "I'm a portal printing some keywords for the lulz" << std::endl;
    for (auto it = sse.keywords.begin(); it != sse.keywords.end(); ++it){
        std::cout << *it << ' ';
    }
    std::cout << std::endl << "Bye" << std::endl;

}

void Portal::closeStream(const Ice::Current&)
{

}

void Portal::receiveInfo(const Ice::Current&)
{

}

StringSequence Portal::sendStreamServersList(const Ice::Current&)
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
