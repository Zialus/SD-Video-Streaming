module PortalServerCommunication{

    sequence<string> StringSequence;

    struct Endpoint {
        string transport;
        string ip;
        string port;
    };

    struct StreamerRegistration {
        string name;
        Endpoint endpoint;
        string videoSize;
        string bitrate;
        StringSequence keywords;
    };

    interface Communication {
        void registerStream(StringSequence sr);
        void closeStream();
        void receiveInfo();
    };

};
