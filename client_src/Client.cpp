// Ice includes
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <IceUtil/IceUtil.h>

// C
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <fcntl.h>

// My stuff
#include <StreamServer.h>
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;

PortalCommunicationPrx portal;
std::string topicName = "Streams";

char **command_name_completion(const char *, int, int);
char *command_name_generator(const char *, int);



void playStream(std::string name){

    StreamsMap streamList = portal->sendStreamServersList();
    auto elem = streamList.find(name);

    if(elem != streamList.end()) {

        int pid = vfork();

        if (pid < 0) {
            perror("fork failed");
            return;
        }

        if (pid == 0) {

            char **strings = NULL;
            size_t strings_size = 0;
            AddString(&strings, &strings_size, "ffplay");

            std::string hostname = elem->second.endpoint.ip;
            std::string port = elem->second.endpoint.port;
            std::string transport = elem->second.endpoint.transport;
            std::stringstream ss;
            ss << transport << "://" << hostname << ":" << port;
            const std::string &tmp = ss.str();
            const char *cstr = tmp.c_str();

            AddString(&strings, &strings_size, cstr);
            AddString(&strings, &strings_size, NULL);

            FILE *ffPlayLog = fopen("/dev/null", "w+");

            if(ffPlayLog == NULL){
                printf("Error opening file ffPlayLog..\n");
            }
            int fd = fileno( ffPlayLog );
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);

            execvp(strings[0], strings);


        } else {
            printf("ffplay should start soon...\n");
        }

    } else {
        printf("There is no stream with that name..\n");
    }
}

void getStreamsList(){

    StreamsMap streamList = portal->sendStreamServersList();
    int size = (int) streamList.size();
    if(size > 0) {
        std::cout << size << " streams available:" << std::endl;
        int counter = 1;
        for (auto const& stream : streamList) {
            std::cout << "\t" << counter << ". " << stream.first << " Video Size: " << stream.second.videoSize << " Bit Rate: " << stream.second.bitrate << std::endl;
            counter++;
        }
        std::cout << std::endl << "-------------------" << std::endl;
    }
    else{
        std::cout << "No available streams atm." << std::endl;
    }
}

void searchKeyword(std::string keyword) {
    StreamsMap streamList = portal->sendStreamServersList();
    int size = (int) streamList.size();
    if(size > 0){
        int counter=0;
        for(auto const& stream : streamList){
            if(!stream.second.keywords.empty()){
                for(std::string key : stream.second.keywords){
                    if(key == keyword){
                        std::cout << "\t" << counter+1 << ". " << stream.first << " Video Size: " << stream.second.videoSize << " Bit Rate: " << stream.second.bitrate << std::endl;
                        counter++;
                        break;
                    }
                }
            }
        }
        if(counter == 0){
            std::cout << "There is no stream with that given keyword.." << std::endl;
        }
    }
    else{
        std::cout << "No available streams atm." << std::endl;
    }
    return;
}

class Client : public Ice::Application {
public:
    virtual void interruptCallback(int) override;
    virtual int run(int, char*[]) override;
    void killFFPlay();
};

class StreamMonitorI : virtual public StreamMonitor {
public:
    virtual void reportAddition(const FCUP::StreamServerEntry& sse, const Ice::Current& ){
        std::cout << std::endl << "A new stream was created... -> " << sse.name << std::endl;
    }
    virtual void reportRemoval(const FCUP::StreamServerEntry& sse, const Ice::Current&){
        std::cout << std::endl << "A stream was deleted... -> " << sse.name << std::endl;

    }
};

void Client::killFFPlay(){
    printf("Killing the ffplay processes...\n");
    //NOT IMPLEMENTED YET
}

void Client::interruptCallback(int signal) {
    printf("Caught the signal: %d!!\n",signal);

    Client::killFFPlay();

    printf("Trying to exit now...\n");
    _exit(0);
}

int Client::run(int argc, char* argv[]) {

    int status = 0;
    IceStorm::TopicPrx topic;

    try {

        IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(
                communicator()->propertyToProxy("TopicManager.Proxy"));
        if(!manager)
        {
            std::cerr << appName() << ": invalid proxy" << std::endl;
            return EXIT_FAILURE;
        }

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

        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("StreamNotifications.Subscriber");


        Ice::Identity subId;
        subId.name = IceUtil::generateUUID();

        Ice::ObjectPrx subscriber = adapter->add(new StreamMonitorI, subId);

        adapter->activate();
        IceStorm::QoS qos;

        subscriber = subscriber->ice_oneway();

        try {
            topic->subscribeAndGetPublisher(qos, subscriber);
        }
        catch(const IceStorm::AlreadySubscribed&) {

        }

        shutdownOnInterrupt();

        Ice::ObjectPrx base = communicator()->propertyToProxy("Portal.Proxy");
        portal = PortalCommunicationPrx::checkedCast(base);
        if (!portal) {
            throw "Invalid proxy";
        }

        rl_attempted_completion_function = command_name_completion;

        while(true){
            char *input;
            input = readline("-> ");
            printf("|%s|\n",input );
            add_history(input);

            std::string str(input);

            std::vector<std::string> userCommands = split(input, ' ');

            if (userCommands.empty()){
                continue;
            }

            if(userCommands[0] == "stream") {

                if (userCommands[1] == "list") {
                    getStreamsList();
                } else if (userCommands[1] == "play") {
                    if(userCommands.size()==3) {
                        playStream(userCommands[2]);
                    } else{
                        std::cout << "Must include one (and only one) stream name.." << std::endl;
                    }
                } else if (userCommands[1] == "search") {
                    if(userCommands.size()>2) {
                        searchKeyword(userCommands[2]);
                    } else {
                        std::cout << "You need to pass one or more keywords.." << std::endl;
                    }
                } else{

                    std::cout << "Can't find that command. Press tab (2x) to see the available commands.." << std::endl;
                }
            }
            else if (userCommands[0] == "exit") {
                topic->unsubscribe(subscriber);
                return 0;
            }
            else{
                //por os comandos disponiveis
                std::cout << "Can't find that command" << std::endl;
            }
            free(input);
        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    if (communicator()){
        communicator()->destroy();
    }

    communicator()->waitForShutdown();

    return status;
}

int main(int argc, char* argv[]) {
    Client app;
    app.main(argc,argv, "config.client");
}


char *command_names[] = {
        (char*) "stream list",
        (char*) "stream search",
        (char*) "stream play",
        (char*) "exit",
        NULL
};

char **command_name_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_name_generator);
}

char *command_name_generator(const char *text, int state) {
    static int list_index, len;
    char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = command_names[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}
