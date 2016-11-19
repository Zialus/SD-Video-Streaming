#include <Ice/Ice.h>
#include "StreamServer.h"

using namespace std;
using namespace PortalServerCommunication;

class Portal : public Communication {
public:
    void registerStream(const PortalServerCommunication::StringSequence&, const Ice::Current&) override;
    void closeStream(const Ice::Current&) override;
    void receiveInfo(const Ice::Current&) override;
};

void Portal::registerStream(const PortalServerCommunication::StringSequence& registrationInfo, const Ice::Current&){
	cout << "I'm a portal receiving the message" << endl;
	for (auto it = registrationInfo.begin(); it != registrationInfo.end(); ++it){
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


int main(int argc, char* argv[])
{
    int status = 0;
    Ice::CommunicatorPtr ic;
    try {
        ic = Ice::initialize(argc, argv);
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("SimplePrinterAdapter", "default -p 10000");
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
