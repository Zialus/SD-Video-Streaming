#include <Ice/Ice.h>

#include <sys/wait.h>

// My stuff
#include "StreamServer.h"
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;

int
main(int argc, char* argv[])
{
	int status = 0;
	Ice::CommunicatorPtr ic;
	try {
		ic = Ice::initialize(argc, argv);
		Ice::ObjectPrx base = ic->stringToProxy("Portal:default -p 9999");
		PortalCommunicationPrx portal = PortalCommunicationPrx::checkedCast(base);
		if (!portal){
			throw "Invalid proxy";
		}

		StringSequence streamList = portal->sendStreamServersList();

		std::cout << "---CLIENT START------" << std::endl;
		for (auto it = streamList.begin(); it != streamList.end(); ++it) {
			std::cout << *it << ' ';
		}
		std::cout << std::endl << "----CLIENT END-------" << std::endl;

		int pid = fork();
		if ( pid < 0 ) {
			perror("fork failed");
			return 1;
		}

		if ( pid == 0 ) {

			char** strings = NULL;
			size_t count = 0;
			AddString(&strings, &count, "ffplay");

			char* hostname = argv[1];
			int port = atoi(argv[2]);
			std::stringstream ss;
			ss << "tcp://" << hostname << ":" << port;

			const std::string& tmp = ss.str();
			const char* cstr = tmp.c_str();

			AddString(&strings, &count, cstr );
			AddString(&strings, &count, NULL);

			for (int i = 0; strings[i] != NULL; ++i)
			{
				printf("|%s|\n",strings[i]);
			}
			execvp(strings[0], strings);
		} else {
			wait(NULL);
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
