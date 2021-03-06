module FCUP{

    sequence<string> StringSequence;

	struct Endpoint {
		string transport;
		string ip;
		string port;
		string path;
	};

	struct StreamServerEntry {
	    string identifier;
		string name;
		Endpoint endpoint;
		string videoSize;
		string bitrate;
		StringSequence keywords;
	};

	dictionary<string, StreamServerEntry> StreamsMap;

	interface PortalServerCommunication {
		void registerStreamServer(StreamServerEntry sse);
		void closeStream(string serverIdentifier);
	};

	interface PortalClientCommunication {
		StreamsMap sendStreamServersList();
	};

    interface StreamMonitor{
        void reportAddition(StreamServerEntry sse);
        void reportRemoval(StreamServerEntry sse);
    };

	interface PortalCommunication extends PortalServerCommunication, PortalClientCommunication {

	};

};
