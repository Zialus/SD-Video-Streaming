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
		Ice::ObjectPrx base = ic->stringToProxy("Portal:default -p 9999");
		PortalCommunicationPrx portal = PortalCommunicationPrx::checkedCast(base);
		if (!portal){
			throw "Invalid proxy";
		}

		StreamServerEntry allMyInfo;

		StringSequence keywords = {"basketball","Cavs","indoor","sports"};
		allMyInfo.keywords = keywords;
		allMyInfo.name = IceUtil::generateUUID();

		portal->registerStreamServer(allMyInfo);

		int pid = fork();

		if ( pid == 0 ) {

			char* argv[200] = {"ffmpeg","-i","/Users/rmf/Downloads/PopeyeAliBaba_512kb.mp4","-loglevel","warning","-analyzeduration","500k","-probesize","500k","-r","30","-s","640x360","-c:v","libx264","-preset","ultrafast","-pix_fmt","yuv420p","-tune","zerolatency","-preset","ultrafast","-b:v","500k","-g","30","-c:a","flac","-profile:a","aac_he","-b:a","32k","-f","mpegts","tcp://127.0.0.1:10000?listen=1",NULL};

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
