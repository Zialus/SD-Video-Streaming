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
		ServerPortalCommunicationPrx portal = ServerPortalCommunicationPrx::checkedCast(base);
		if (!portal){
			throw "Invalid proxy";
		}

		StringSequence registrationInfo;
		registrationInfo.push_back("basketball");
		registrationInfo.push_back("Cavs");
		registrationInfo.push_back("indoor");
		registrationInfo.push_back("sports");

		cout << "I'm a Server sending the message" << endl;
		for (auto it = registrationInfo.begin(); it != registrationInfo.end(); ++it){
			cout << *it << ' ';
		}
		cout << endl << "Hello" << endl;

		portal->registerStream(registrationInfo);
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
