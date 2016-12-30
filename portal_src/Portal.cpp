#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <StreamServer.h>

using namespace FCUP;

StreamsMap list_of_stream_servers;
StreamMonitorPrx streamNotifier;

std::string topicName = "Streams";

class Portal : public PortalCommunication,  public Ice::Application {
public:
    void registerStreamServer(const FCUP::StreamServerEntry&, const Ice::Current&) override;
    void closeStream(const std::string&, const Ice::Current&) override;
    StreamsMap sendStreamServersList(const Ice::Current&) override;

private:
    int run(int, char*[]) override;
    int refreshTopicManager();
};

int Portal::refreshTopicManager() {

    IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(
            communicator()->propertyToProxy("TopicManager.Proxy"));

    if(!manager) {
        std::cerr << appName() << ": invalid proxy" << std::endl;
        return EXIT_FAILURE;
    }

    IceStorm::TopicPrx topic;

    try {
        topic = manager->retrieve(topicName);
    } catch (const IceStorm::NoSuchTopic&) {
        try {
            topic = manager->create(topicName);
        } catch (const IceStorm::TopicExists&) {
            std::cerr << appName() << ": topic exists, please try again." << std::endl;
            return EXIT_FAILURE;
        }
    }

    Ice::ObjectPrx obj = topic->getPublisher();
    streamNotifier = StreamMonitorPrx::uncheckedCast(obj);
    return EXIT_SUCCESS;
}

void Portal::registerStreamServer(const FCUP::StreamServerEntry& sse, const Ice::Current&) {

    list_of_stream_servers[sse.name] = sse;

    std::cout << std::endl << "_________" << std::endl;
    std::cout << "Added a new stream server: " << sse.name << std::endl;

    std::cout << "Video keywords:  ";
    for (auto it = sse.keywords.begin(); it != sse.keywords.end(); ++it){
        std::cout << *it << ' ';
    }
    std::cout << std::endl << "---------" << std::endl;

    refreshTopicManager();

    try {
        streamNotifier->reportAddition(sse);
        std::cout << "Publishing a stream addition." << std::endl;
    } catch (const Ice::Exception& e) {
        std::cerr << e << std::endl;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
    }

}

void Portal::closeStream(const std::string& serverName, const Ice::Current&) {

    std::cout << "Closing the stream: " << serverName << std::endl;
    auto elem = list_of_stream_servers.find(serverName);

    if(elem != list_of_stream_servers.end()){
        std::cout << "Closed stream -> " << serverName << std::endl;

        try {
            streamNotifier->reportRemoval(elem->second);
            std::cout << "Publishing a stream removal." << std::endl;
        } catch (const Ice::Exception& e) {
            std::cerr << e << std::endl;
        } catch (const char* msg) {
            std::cerr << msg << std::endl;
        }
        list_of_stream_servers.erase(elem);


    } else{
        std::cout << "Couldn't close/find stream -> " << serverName << std::endl;
    }
}

StreamsMap Portal::sendStreamServersList(const Ice::Current&) {
    return list_of_stream_servers;
}

int Portal::run(int argc, char* argv[]) {

    if(argc > 1) {
        std::cerr << appName() << ": too many arguments" << std::endl;
        return EXIT_FAILURE;
    }

    Portal::refreshTopicManager();

    int status = 0;
    try {
        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Portal");
        Ice::ObjectPtr object = new Portal;
        adapter->add(object, communicator()->stringToIdentity("portal"));
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

int main(int argc, char* argv[]) {

    Portal app;
    return app.main(argc, argv,"config.portal");
}
