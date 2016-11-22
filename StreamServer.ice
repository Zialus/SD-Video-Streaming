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

	interface PortalCommunication {
		void registerStream(StringSequence sr);
		void closeStream();
		void receiveInfo();

		StringSequence sendStreamServersList();
	};

};
