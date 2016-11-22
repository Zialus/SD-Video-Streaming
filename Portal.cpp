#include <Ice/Ice.h>
#include "StreamServer.h"

using namespace std;
using namespace FCUP;

StringSequence list_of_stream_servers;

class Portal : public PortalCommunication
{
public:
	void registerStreamServer(const FCUP::StreamServerEntry&, const Ice::Current&) override;
	void closeStream(const Ice::Current&) override;
	void receiveInfo(const Ice::Current&) override;

	StringSequence sendStreamServersList(const Ice::Current&) override;
};

void Portal::registerStreamServer(const FCUP::StreamServerEntry& sse, const Ice::Current&){

	string server_name = sse.name;
	list_of_stream_servers.push_back(server_name);

	cout << "I'm a portal printing some keywords for the lulz" << endl;
	for (auto it = sse.keywords.begin(); it != sse.keywords.end(); ++it){
		cout << *it << ' ';
	}
	cout << endl << "Bye" << endl;

}

void Portal::closeStream(const Ice::Current&)
{

}

void Portal::receiveInfo(const Ice::Current&)
{

}

StringSequence Portal::sendStreamServersList(const Ice::Current&)
{
	return list_of_stream_servers;
}


int main(int argc, char* argv[])
{
	int status = 0;
	Ice::CommunicatorPtr ic;
	try {
		ic = Ice::initialize(argc, argv);
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("PortalAdapter", "default -p 10000");
		Ice::ObjectPtr object = new Portal;
		adapter->add(object, ic->stringToIdentity("Portal"));
		adapter->activate();
		ic->waitForShutdown();
	} catch (const Ice::Exception& e) {
		cerr << e << endl;
		status = 1;
	} catch (const char* msg) {
		cerr << msg << endl;
		status = 1;
	}
	if (ic) {
		try {
			ic->destroy();
		} catch (const Ice::Exception& e) {
			cerr << e << endl;
			status = 1;
		}
	}
	return status;
}
