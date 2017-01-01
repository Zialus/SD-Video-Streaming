// Ice includes
#include <Ice/Ice.h>
#include <IceUtil/UUID.h>
#include <IceUtil/CtrlCHandler.h>

// C
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

// C++
#include <fstream>
#include <tclap/CmdLine.h>

// My stuff
#include <StreamServer.h>
#include "../auxiliary/Auxiliary.h"

#define BUFFERSIZE 32

using namespace FCUP;

pid_t regularFFmpegPID = -9;
pid_t hlsFFmpegPID = -9;
pid_t dashFFmpegPID = -9;

std::string hostname;
std::string moviename;
int portForFFMPEG;
int portForClients;
std::string videosize;
std::string bitrate;
std::string encoder;
std::string filename;
std::string transportType;
StringSequence keywords;
bool useHLS;
bool useDASH;

class Server : public Ice::Application {
public:
    virtual void interruptCallback(int) override;
    virtual int run(int, char *[]) override;
private:
    void closeStream();
    void killFFMpeg();
    PortalCommunicationPrx portal;
    std::string serverIdentifier;
    std::list<int> clientsSocketList;
};

void Server::killFFMpeg() {
    printf("Killing the ffmpeg process...\n");

    if(regularFFmpegPID != -9) {
        kill(regularFFmpegPID, SIGKILL);
    }
    if(hlsFFmpegPID != -9) {
        kill(hlsFFmpegPID, SIGKILL);
    }
    if(dashFFmpegPID != -9) {
        kill(dashFFmpegPID, SIGKILL);
    }
}

void Server::closeStream() {
    printf("Trying to close the stream...\n");
    portal->closeStream(serverIdentifier);
    printf("Stream closed\n");
}

void Server::interruptCallback(int signal) {
    printf("Caught the signal: %d!!\n",signal);

    Server::closeStream();

    Server::killFFMpeg();

    printf("Trying to exit now...\n");
    _exit(0);
}

int Server::run(int argc, char* argv[]) {

    callbackOnInterrupt();

    int status = 0;

    try {

        serverIdentifier = IceUtil::generateUUID();
        Ice::ObjectPrx base = communicator()->propertyToProxy("Portal.Proxy");
        portal = PortalCommunicationPrx::checkedCast(base);
        if (!portal){
            throw "Invalid proxy";
        }

        StreamServerEntry allMyInfo;

        allMyInfo.identifier = serverIdentifier;
        allMyInfo.name = moviename;

        allMyInfo.keywords = keywords;
        allMyInfo.videoSize = videosize;
        allMyInfo.bitrate = bitrate;
        allMyInfo.endpoint.ip = hostname;
        allMyInfo.endpoint.port = std::to_string(portForClients).c_str();
        allMyInfo.endpoint.transport = transportType;


        printf("\nI'm going to register myself on the portal...\n\n");
        portal->registerStreamServer(allMyInfo);
        printf("Portal registration done!\n\n");

        regularFFmpegPID = vfork();
        if ( regularFFmpegPID < 0 ) {
            perror("regularFFmpeg fork failed");
            return 1;
        }

        if ( regularFFmpegPID == 0 ) { // Child process to create TCP stream

            std::stringstream ss;
            ss << transportType << "://" << hostname << ":" << portForFFMPEG << "?listen=1";
            const std::string& tmp = ss.str();
            const char* whereToListen = tmp.c_str();

            std::cout << "| "<< whereToListen << " |" << std::endl;

            execlp("ffmpeg","ffmpeg","-re","-i",filename.c_str(),"-loglevel","warning",
                   "-analyzeduration","500k","-probesize","500k","-r","30","-s",videosize.c_str(),"-c:v",encoder.c_str(),"-preset","ultrafast","-pix_fmt",
                   "yuv420p","-tune","zerolatency","-preset","ultrafast","-b:v", bitrate.c_str(),"-g","30","-c:a","flac","-profile:a","aac_he","-b:a",
                   "32k","-f","mpegts",whereToListen,NULL);

        } else { // Parent will only start executing after child calls execvp because we are using vfork()

            sleep(1); //wait for initial ffmpeg to be execed

            if (useDASH) {
                dashFFmpegPID = vfork();
                if ( dashFFmpegPID < 0 ) {
                    perror("dashFFmpeg fork failed");
                    return 1;
                }

                if ( dashFFmpegPID == 0 ) { // Child process to create DASH stream

                    std::stringstream ss;
                    ss << transportType << "://" << hostname << ":" << portForClients;
                    const std::string& tmp = ss.str();
                    const char* whereToListenFrom = tmp.c_str();

                    std::cout << "| "<< whereToListenFrom << " |" << std::endl;

                    execlp("ffmpeg","ffmpeg","-re","-i",whereToListenFrom,"-loglevel","warning","-vcodec","libx264","-vprofile",
                           "baseline","-acodec","libmp3lame","-ar","44100","-ac","1","-f","flv","rtmp://localhost:1935/dash/movie",NULL);
                } else {
                    printf("DASH is starting...");
                }
            }

            if (useHLS) {

                hlsFFmpegPID = vfork();
                if ( hlsFFmpegPID < 0 ) {
                    perror("fork failed");
                    return 1;
                }

                if ( hlsFFmpegPID == 0 ) { // Child process to create DASH stream

                    std::stringstream ss;
                    ss << transportType << "://" << hostname << ":" << portForClients;
                    const std::string& tmp = ss.str();
                    const char* whereToListenFrom = tmp.c_str();

                    std::cout << "| "<< whereToListenFrom << " |" << std::endl;

                    execlp("ffmpeg","ffmpeg","-re","-i",whereToListenFrom,"-loglevel","warning", "-vcodec","libx264","-vprofile",
                           "baseline","-acodec","libmp3lame","-ar","44100","-ac","1","-f","flv","rtmp://localhost:1935/hls/movie",NULL);
                } else {
                    printf("HLS is starting...");
                }

            }

            printf("LOL\n");

            int n;
            int socketToReceiveVideoFD;
            struct addrinfo hints, *res, *ressave;

            const char *portToReceiveVideo;
            const char *ffmpegServer;
            char ffmpegBuffer[BUFFERSIZE];

            ffmpegServer = hostname.c_str();
            portToReceiveVideo = std::to_string(portForFFMPEG).c_str();

            printf("Video vai ser recebido na porta %s e no adress %s\n", portToReceiveVideo, ffmpegServer);


            bzero( (char *) &hints, sizeof(addrinfo) );

            bzero(&hints, sizeof(struct addrinfo));
            hints.ai_family=AF_UNSPEC;
            hints.ai_socktype=SOCK_STREAM;
            hints.ai_protocol=IPPROTO_TCP;

            if((n=getaddrinfo(ffmpegServer, portToReceiveVideo, &hints, &res))!=0) {
                printf("getaddrinfo error for %s, %s; %s", ffmpegServer, portToReceiveVideo, gai_strerror(n));
            }

            ressave=res;
            do{
                socketToReceiveVideoFD=socket(res->ai_family, res->ai_socktype, res->ai_protocol);

                if(socketToReceiveVideoFD<0)
                    continue;  /*ignore this returned Ip addr*/

                if(connect(socketToReceiveVideoFD, res->ai_addr, res->ai_addrlen)==0) {
                    printf("connection ok!\n"); /* success*/
                    break;
                } else{
                    perror("connecting stream socket");
                }

            } while ((res=res->ai_next)!= NULL);

            freeaddrinfo(ressave);

            printf("Connection to FFMPEG succeded!!\n");

            //--------------SERVER PART-----------------//

            struct sockaddr_in server_address;

            //open socket
            int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket_fd < 0){
                perror("ERROR opening socket");
                return 1;
            }

            int rc = fcntl(server_socket_fd, F_SETFL, O_NONBLOCK);
            if(rc<0){
                perror("fcntl() failed");
                close(server_socket_fd);
                return 1;
            }
            // set all values in server_adress to 0
            bzero((char *) &server_address, sizeof(server_address));

            server_address.sin_family = AF_INET;
            server_address.sin_port = htons((uint16_t) portForClients);
            server_address.sin_addr.s_addr = INADDR_ANY;


            int bind_result = bind(server_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
            if ( bind_result < 0 ) {
                perror("ERROR on binding");
                close(server_socket_fd);
                return 1;
            }

            rc = listen(server_socket_fd, 32);
            if (rc < 0) {
                perror("listen() failed");
                close(server_socket_fd);
                return 1;
            }


            printf("Ready to send to clients!\n");

            int numberOfWrittenElements = 0;
            int counter = 0;

            while (true) {
                counter++;
                int newClientFD = accept(server_socket_fd, NULL, NULL);
                if (newClientFD > 0){
                    clientsSocketList.push_back(newClientFD);
                    printf("ADDED Client with SOCKET ---> %d !!!\n", newClientFD);
                }

                numberOfWrittenElements = (int) read(socketToReceiveVideoFD, ffmpegBuffer, BUFFERSIZE);

                if (numberOfWrittenElements < 0){
                    perror("ERROR reading from socket");
                    return 1;
                }   else if (numberOfWrittenElements == 0){
                    printf("Stream is over..\n");
                }
                else{
//                    printf("%d. Bytes Read-> %d\n", counter, numberOfWrittenElements);
                }

                clientsSocketList.remove_if([ffmpegBuffer](int clientSocket)  {

                    int bytesWritten = (int) write(clientSocket, ffmpegBuffer, BUFFERSIZE);
                    if (bytesWritten < 0) {
                        printf("REMOVED Client with SOCKET ---> %d !!!\n", clientSocket);
                        return true;
                    }

//                    printf("%%Client |%d| ---> received %d bytes %%\n", clientSocket, bytesWritten);
                    return false;
                });

            }
        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    std::cout << "It's over" << std::endl;
    return status;

}

void commandLineParsing(int argc, char* argv[]) {

    try {
        TCLAP::CmdLine cmd("Streaming Server", ' ', "1.0",true);

        std::vector<std::string> allowedEnconders;
        allowedEnconders.push_back("libx264");
        allowedEnconders.push_back("libx265");
        TCLAP::ValuesConstraint<std::string> allowedEnc( allowedEnconders );

        std::vector<std::string> allowedTransportTypes;
        allowedTransportTypes.push_back("tcp");
        allowedTransportTypes.push_back("upd");
        TCLAP::ValuesConstraint<std::string> allowedTT( allowedTransportTypes);

        TCLAP::ValueArg<std::string> hostNameArg("","host","FFmpeg hostname",false,"localhost","address");
        TCLAP::ValueArg<std::string> movieNameArg("n","name","Movie name",true,"","name string");

        TCLAP::ValueArg<int> ffmpegPortArg("","ff_port","Port where FFMPEG is running",true,0,"port number");
        TCLAP::ValueArg<int> clientsPortArg("","my_port","Port that will listen to clients",true,0,"port number");

        TCLAP::SwitchArg hlsSwitchArg("","hls","Produce HLS stream",false);
        TCLAP::SwitchArg dashSwitchArg("","dash","Produce DASH stream", false);

        TCLAP::ValueArg<std::string> videoSizeArg("v","videosize","WIDTHxHEIGHT",true,"","WIDTHxHEIGHT");
        TCLAP::ValueArg<std::string> bitRateArg("b","bitrate","bitrate",true,"","bitrate in a string");
        TCLAP::ValueArg<std::string> encoderArg("e","enconder","Enconder",true,"",&allowedEnc);
        TCLAP::ValueArg<std::string> filenameArg("f","filename","Movie file name",true,"","path string");
        TCLAP::ValueArg<std::string> transportTypeArg("t","transport_type","Transport Type",false,"tcp",&allowedTT);

        TCLAP::MultiArg<std::string> keywordsArgs("k","keyword","keywords",false,"keyword");

        cmd.add(hostNameArg);
        cmd.add(movieNameArg);
        cmd.add(ffmpegPortArg);
        cmd.add(clientsPortArg);
        cmd.add(hlsSwitchArg);
        cmd.add(dashSwitchArg);
        cmd.add(videoSizeArg);
        cmd.add(bitRateArg);
        cmd.add(encoderArg);
        cmd.add(filenameArg);
        cmd.add(transportTypeArg);
        cmd.add(keywordsArgs);

        cmd.parse(argc,argv);

        hostname = hostNameArg.getValue();
        moviename = movieNameArg.getValue();
        portForFFMPEG = ffmpegPortArg.getValue();
        portForClients = clientsPortArg.getValue();
        useDASH = dashSwitchArg.getValue();
        useHLS = hlsSwitchArg.getValue();
        videosize = videoSizeArg.getValue();
        bitrate = bitRateArg.getValue();
        encoder = encoderArg.getValue();
        filename = filenameArg.getValue();
        transportType = transportTypeArg.getValue();
        keywords = keywordsArgs.getValue();

    } catch (TCLAP::ArgException &e) {  // catch any exceptions
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    }

}

int main(int argc, char* argv[]) {

    commandLineParsing(argc,argv);

    Server app;
    app.main(argc, argv, "config.server");
}
