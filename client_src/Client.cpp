// Ice includes
#include <Ice/Ice.h>
#include <sys/wait.h>

// C
//#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

// My stuff
#include "StreamServer.h"
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;

PortalCommunicationPrx portal;

char **command_name_completion(const char *, int, int);
char *command_name_generator(const char *, int);

char *command_names[] = {
        "stream list",
        "stream search",
        "stream play",
        "exit",
        NULL
};




void playStream(char* argvzinho[]){

    int pid = fork();
    if ( pid < 0 ) {
        perror("fork failed");
        exit(1);
    }

    if ( pid == 0 ) {

        char** strings = NULL;
        size_t strings_size = 0;
        AddString(&strings, &strings_size, "ffplay");

        char* hostname = argvzinho[1];
        int port = atoi(argvzinho[2]);
        std::stringstream ss;
        ss << "tcp://" << hostname << ":" << port;
        const std::string& tmp = ss.str();
        const char* cstr = tmp.c_str();

        AddString(&strings, &strings_size, cstr );
        AddString(&strings, &strings_size, NULL);

        for (int i = 0; strings[i] != NULL; ++i) {
            printf("|%s|\n",strings[i]);
        }

        execvp(strings[0], strings);

    } else {
        wait(NULL);
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

}

int main(int argc, char* argv[])
{
    int status = 0;
    Ice::CommunicatorPtr ic;
    try {
        ic = Ice::initialize(argc, argv);
        Ice::ObjectPrx base = ic->stringToProxy("Portal:default -p 9999");
        portal = PortalCommunicationPrx::checkedCast(base);
        if (!portal){
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

            if(userCommands[0] == "stream") {

                if (userCommands[1] == "list") {
                    getStreamsList();
                } else if (userCommands[1] == "play") {
                    playStream(argv);
                } else if (userCommands[1] == "search") {
                    if(userCommands.size()>2) {
                        searchKeyword(userCommands[2]);
                    } else {
                        std::cout << "You need to pass one or more keywords.." << std::endl;
                    }
                } else{
                    //por os comandos disponiveis
                    std::cout << "Can't find that command" << std::endl;
                }
            }
            else if (userCommands[0] == "exit") {
                return 0;
            }
            else{
                //por os comandos disponiveis
                std::cout << "Can't find that command" << std::endl;
            }

        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    if (ic){
        ic->destroy();
    }

    return status;
}


char ** command_name_completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_name_generator);
}

char * command_name_generator(const char *text, int state)
{
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