#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include <IceStorm/IceStorm.h>
#include <StreamServer.h>

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

using namespace FCUP;

StreamsMap list_of_stream_servers;
StreamNotificationsPrx streamNotifier;

std::string IceStormInterfaceName = "StreamNotifications";

class Portal : public PortalCommunication,  public Ice::Application
{
public:
    void registerStreamServer(const FCUP::StreamServerEntry&, const Ice::Current&) override;
    void closeStream(const std::string&, const Ice::Current&) override;

    StreamsMap sendStreamServersList(const Ice::Current&) override;

    virtual int run(int, char*[]) override;

private:
    int CheckRecheck();
};


int Portal::CheckRecheck() {

    IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(
            communicator()->propertyToProxy("TopicManager.Proxy"));
    if(!manager)
    {
        std::cerr << appName() << ": invalid proxy" << std::endl;
        return EXIT_FAILURE;
    }

    IceStorm::TopicPrx topic;
    try {
        topic = manager->retrieve(IceStormInterfaceName);
    }
    catch (const IceStorm::NoSuchTopic &) {
        try {
            topic = manager->create(IceStormInterfaceName);
        }
        catch (const IceStorm::TopicExists &) {
            std::cerr << appName() << ": topic exists, please try again." << std::endl;
            return EXIT_FAILURE;
        }
    }

    Ice::ObjectPrx obj = topic->getPublisher();
    streamNotifier = StreamNotificationsPrx::uncheckedCast(obj);
    return EXIT_SUCCESS;
}


void Portal::registerStreamServer(const FCUP::StreamServerEntry& sse, const Ice::Current&)
{
    list_of_stream_servers[sse.name] = sse;


    std::cout << "I'm a portal printing some keywords for the lulz" << std::endl;
    for (auto it = sse.keywords.begin(); it != sse.keywords.end(); ++it){
        std::cout << *it << ' ';
    }
    std::cout << std::endl << "Bye" << std::endl;

    std::cout << "publishing something." << std::endl;

    CheckRecheck();

    try {
        streamNotifier->reportAddition(sse);
    }
    catch(const Ice::CommunicatorDestroyedException&) {
        // Ignore
    }

}

void Portal::closeStream(const std::string& serverName, const Ice::Current&) {
    std::cout << "Going to close the stream -> " << serverName << std::endl;
    auto elem = list_of_stream_servers.find(serverName);
    if(elem != list_of_stream_servers.end()){
        std::cout << "publishing something." << std::endl;
        try {
            streamNotifier->reportRemoval(elem->second);
        }
        catch(const Ice::CommunicatorDestroyedException&) {
            // Ignore
        }
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


int Portal::run(int argc, char* argv[]) {

    if(argc > 1)
    {
        std::cerr << appName() << ": too many arguments" << std::endl;
        return EXIT_FAILURE;
    }

    Portal::CheckRecheck();

    int status = 0;
    try {
        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("PortalAdapter", "default -p 9999");
        Ice::ObjectPtr object = new Portal;
        adapter->add(object, communicator()->stringToIdentity("Portal"));
        adapter->activate();
    } catch (const Ice::Exception& e) {
        std::cerr << e << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    communicator()->waitForShutdown();

    return status;

}



int main(int argc, char* argv[])
{

    Portal app;
    return app.main(argc, argv,"config.pub");

}
