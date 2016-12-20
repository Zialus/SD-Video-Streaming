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
StreamWatcherPrx streamzPubz;

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

    std::cout << "publishing something." << std::endl;
    try {
        streamzPubz->report(sse);
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
            streamzPubz->report(elem->second);
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

class Publisher : public Ice::Application
{
public:

    virtual int run(int, char*[]);
};

int Publisher::run(int argc, char* argv[]) {
    std::string topicName = "streams";

    IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(
            communicator()->propertyToProxy("TopicManager.Proxy"));
    if (!manager) {
        std::cerr << appName() << ": invalid proxy" << std::endl;
        return EXIT_FAILURE;
    }

    IceStorm::TopicPrx topic;
    try
    {
        topic = manager->retrieve(topicName);
    }
    catch(const IceStorm::NoSuchTopic&)
    {
        try
        {
            topic = manager->create(topicName);
        }
        catch(const IceStorm::TopicExists&)
        {
            std::cerr << appName() << ": temporary failure. try again." << std::endl;
            return EXIT_FAILURE;
        }
    }

    //
    // Get the topic's publisher object, and create a Clock proxy with
    // the mode specified as an argument of this application.
    //
    Ice::ObjectPrx publisher = topic->getPublisher()->ice_oneway();


    streamzPubz = StreamWatcherPrx::uncheckedCast(publisher);


    return EXIT_SUCCESS;


}

int main(int argc, char* argv[])
{

    Publisher app;
    app.main(argc, argv,"config.pub");

    printf("lol");

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
