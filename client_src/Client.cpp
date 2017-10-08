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

PortalCommunicationPrxPtr portal;
std::string topicName = "Streams";
std::vector<pid_t> ffplaysPIDs;

char** command_name_completion(const char*, int, int);
char* command_name_generator(const char*, int);

char* command_names[] = {
        (char*) "list",
        (char*) "search",
        (char*) "play",
        (char*) "exit",
        NULL
};

void playStream(std::string name) {

    StreamsMap streamList = portal->sendStreamServersList();
    auto elem = streamList.find(name);

    if (elem != streamList.end()) {

        pid_t pid = vfork();

        if (pid < 0) {
            perror("fork failed");
            return;
        }

        if (pid == 0) {

            char** strings = nullptr;
            size_t strings_size = 0;
            AddString(&strings, &strings_size, "ffplay");

            std::string hostname = elem->second.endpoint.ip;
            std::string port = elem->second.endpoint.port;
            std::string transport = elem->second.endpoint.transport;
            std::string path = elem->second.endpoint.path;
            std::stringstream ss;
            ss << transport << "://" << hostname << ":" << port << path;
            const std::string& tmp = ss.str();
            const char* cstr = tmp.c_str();

            AddString(&strings, &strings_size, cstr);
            AddString(&strings, &strings_size, nullptr);

            FILE* ffPlayLog = fopen("/dev/null", "w+");
            if (ffPlayLog == nullptr) {
                printf("Error opening file ffPlayLog..\n");
            }
            int fd = fileno(ffPlayLog);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);

            execvp(strings[0], strings);

        } else {
            ffplaysPIDs.push_back(pid);
            printf("ffplay should start soon...\n");
        }

    } else {
        printf("There is no stream with that name..\n");
    }
}

void getStreamsList() {

    StreamsMap streamList = portal->sendStreamServersList();
    int size = (int) streamList.size();
    if (size > 0) {
        std::cout << size << " streams available:" << std::endl;
        int counter = 1;
        for (auto const& stream : streamList) {
            std::cout << "\t" << counter << ". " << stream.first << " Name: " << stream.second.name
                      << " Video Size: " << stream.second.videoSize << " BitRate: " << stream.second.bitrate
                      << std::endl;
            counter++;
        }
        std::cout << std::endl << "-------------------" << std::endl;
    } else {
        std::cout << "No available streams atm." << std::endl;
    }
}

void searchKeyword(const std::string& keyword) {

    StreamsMap streamList = portal->sendStreamServersList();
    int size = (int) streamList.size();
    if (size > 0) {
        int counter = 0;
        for (auto const& stream : streamList) {
            if (!stream.second.keywords.empty()) {
                for (const std::string& key : stream.second.keywords) {
                    if (key == keyword) {
                        std::cout << "\t" << counter + 1 << ". " << stream.first << " -> Name: " << stream.second.name
                                  << " Video Size: " << stream.second.videoSize << " Bit Rate: "
                                  << stream.second.bitrate << std::endl;
                        counter++;
                        break;
                    }
                }
            }
        }
        if (counter == 0) {
            std::cout << "There is no stream with that given keyword.." << std::endl;
        }
    } else {
        std::cout << "No available streams atm." << std::endl;
    }
}

class Client : public Ice::Application {
public:
    void interruptCallback(int signal) override;
    int run(int argc, char* argv[]) override;
    void killFFPlay();
private:
    IceStorm::TopicPrxPtr topic;
    Ice::ObjectPrxPtr subscriber;
};

class StreamMonitorI : virtual public StreamMonitor {
public:
    void reportAddition(FCUP::StreamServerEntry sse, const Ice::Current&) override {
        std::cout << std::endl << "A new stream was created... " << "Stream Identifier: " << sse.identifier
                  << " Video Name: " << sse.name << std::endl;
    }

    void reportRemoval(FCUP::StreamServerEntry sse, const Ice::Current&) override {
        std::cout << std::endl << "A stream was deleted... " << "Stream Identifier: " << sse.identifier
                  << " Video Name: " << sse.name << std::endl;
    }
};

void Client::killFFPlay() {
    printf("Killing the FFPlay processes...\n");

    for (const pid_t ffplay_pid : ffplaysPIDs) {
        kill(ffplay_pid, SIGKILL);
        printf("Killing ffplay with pid: %d...", ffplay_pid);
    }

    printf("\nDone with killing the FFPlay processes...\n");
}

void Client::interruptCallback(int signal) {
    printf("Caught the signal: %d!!\n", signal);

    Client::killFFPlay();

    topic->unsubscribe(subscriber);

    printf("Trying to exit now...\n");
    _exit(0);
}

int Client::run(int argc, char* argv[]) {

    int status = 0;

    try {

        IceStorm::TopicManagerPrxPtr manager = Ice::checkedCast<IceStorm::TopicManagerPrx>(
                communicator()->propertyToProxy("TopicManager.Proxy"));
        if (!manager) {
            std::cerr << appName() << ": invalid proxy" << std::endl;
            return EXIT_FAILURE;
        }

        try {
            topic = manager->retrieve(topicName);
        }
        catch (const IceStorm::NoSuchTopic&) {
            try {
                topic = manager->create(topicName);
            }
            catch (const IceStorm::TopicExists&) {
                std::cerr << appName() << ": temporary failure. try again." << std::endl;
                return EXIT_FAILURE;
            }
        }

        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("StreamNotifications.Subscriber");


        Ice::Identity subId;
        subId.name = IceUtil::generateUUID();

        subscriber = adapter->add(std::make_shared<StreamMonitorI>(), subId);

        adapter->activate();
        IceStorm::QoS qos;

        subscriber = subscriber->ice_oneway();

        try {
            topic->subscribeAndGetPublisher(qos, subscriber);
        }
        catch (const IceStorm::AlreadySubscribed& e) {
            e.ice_stackTrace();
        }

        callbackOnInterrupt();

        Ice::ObjectPrxPtr base = communicator()->propertyToProxy("Portal.Proxy");
        portal = Ice::checkedCast<PortalCommunicationPrx>(base);
        if (!portal) {
            throw "Invalid proxy";
        }

        rl_attempted_completion_function = command_name_completion;

        while (true) {
            char* input;
            input = readline("-> ");
            add_history(input);

            std::string str(input);

            std::vector<std::string> userCommands = split(input, ' ');

            if (userCommands.empty()) {
                continue;
            }

            if (userCommands[0] == "list") {
                getStreamsList();
            } else if (userCommands[0] == "play") {
                if (userCommands.size() == 2) {
                    playStream(userCommands[1]);
                } else {
                    std::cout << "Must include one (and only one) stream name.." << std::endl;
                }
            } else if (userCommands[0] == "search") {
                if (userCommands.size() == 2) {
                    searchKeyword(userCommands[1]);
                } else {
                    std::cout << "You need to pass only one keyword.." << std::endl;
                }
            } else if (userCommands[0] == "exit") {
                topic->unsubscribe(subscriber);
                break;
            } else {
                std::cout << "Can't find that command. Press tab (2x) to see the available commands.." << std::endl;
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

    Client::killFFPlay();

    printf("Closing the client...\n");
    return status;
}

int main(int argc, char* argv[]) {
    Client app;
    app.main(argc, argv);
}


char** command_name_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_name_generator);
}

char* command_name_generator(const char* text, int state) {
    static int list_index, len;
    char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = command_names[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return nullptr;
}
