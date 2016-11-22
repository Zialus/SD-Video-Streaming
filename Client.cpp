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
		Ice::ObjectPrx base = ic->stringToProxy("Portal:default -p 10000");
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
