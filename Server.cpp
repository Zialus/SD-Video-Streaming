#include <Ice/Ice.h>
#include <IceUtil/UUID.h>
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

		StreamServerEntry allMyInfo;

		StringSequence keywords = {"basketball","Cavs","indoor","sports"};
		allMyInfo.keywords = keywords;
		allMyInfo.name = IceUtil::generateUUID();

		portal->registerStreamServer(allMyInfo);

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
