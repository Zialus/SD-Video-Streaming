// Ice includes
#include <Ice/Ice.h>
#include <sys/wait.h>

// C
#include <sys/wait.h>

// My stuff
#include "StreamServer.h"
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;

PortalCommunicationPrx portal;

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

    StringSequence streamList = portal->sendStreamServersList();
    int siz = streamList.size();
    if(siz > 0) {
        std::cout << siz << " streams available:" << std::endl;
        for (auto it = streamList.begin(); it != streamList.end(); ++it) {
            std::cout << " - " << *it << std::endl;
        }
        std::cout << std::endl << "-------------------" << std::endl;
    }
    else{
        std::cout << "No available streams atm." << std::endl;
    }
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

        while(true){
            std::string input;
            std::cout << "-> ";
            std::cin >> input;
            if (input == "list"){
                getStreamsList();
            }
            else if(input == "play"){
                playStream(argv);
            }
            else if(input == "exit"){
                return 0;
            }
            else{
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
