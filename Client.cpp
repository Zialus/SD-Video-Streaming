#include <Ice/Ice.h>
#include "StreamServer.h"

using namespace std;
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

		cout << "---CLIENT START------" << endl;
		for (auto it = streamList.begin(); it != streamList.end(); ++it) {
			cout << *it << ' ';
		}
		cout << endl << "----CLIENT END-------" << endl;

		int pid = fork();
		if ( pid < 0 ) {
			perror("fork failed");
			return 1;
		}
		
		if ( pid == 0 ) {

			char* argv[3] = {"ffplay","tcp://127.0.0.1:10000",NULL};

			for (int i = 0; argv[i] != NULL; ++i)
			{
				printf("|%s|\n",argv[i]);
			}
			execvp(argv[0], argv);
		} else {
			wait(NULL);
		}

	} catch (const Ice::Exception& ex) {
		cerr << ex << endl;
		status = 1;
	} catch (const char* msg) {
		cerr << msg << endl;
		status = 1;
	}

	if (ic){
		ic->destroy();
	}

	return status;
}
