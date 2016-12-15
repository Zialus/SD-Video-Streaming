module FCUP{

	sequence<string> StringSequence;

	struct Endpoint {
		string transport;
		string ip;
		string port;
	};

	struct StreamServerEntry {
		string name;
		Endpoint endpoint;
		string videoSize;
		string bitrate;
		StringSequence keywords;
	};

	interface PortalServerCommunication {
		void registerStreamServer(StreamServerEntry sse);
		void closeStream(string serverName);
		void receiveInfo();
	};

	interface PortalClientCommunication {
		StringSequence sendStreamServersList();
	};

	interface PortalCommunication extends PortalServerCommunication, PortalClientCommunication {

	};

};
