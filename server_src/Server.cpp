// Ice includes
#include <Ice/Ice.h>
#include <IceUtil/UUID.h>
#include <IceUtil/CtrlCHandler.h>

// C
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <cstdio>

// C++
#include <fstream>

// TCLAP Library
#include <tclap/CmdLine.h>

// My stuff
#include <StreamServer.h>
#include "../auxiliary/Auxiliary.h"

#define BUFFERSIZE 188
#define MAGICNEGATIVE (-9)

using namespace FCUP;

int server_socket_fd;

pid_t regularFFmpegPID = MAGICNEGATIVE;
pid_t hlsFFmpegPID = MAGICNEGATIVE;
pid_t dashFFmpegPID = MAGICNEGATIVE;

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

void commandLineParsing(int argc, char* argv[]);

class Server : public Ice::Application {
public:
    void interruptCallback(int signal) override;
    int run(int argc, char* argv[]) override;
private:
    void closeStream();
    void killFFMpeg();
    PortalCommunicationPrxPtr portal;
    std::list<std::string> serverIdentifierList;
    std::list<int> clientsSocketList;
};

void Server::killFFMpeg() {
    printf("Killing the FFMpeg processes...\n");

    if (regularFFmpegPID != MAGICNEGATIVE) {
        printf("Regular FFMpeg killed...");
        kill(regularFFmpegPID, SIGKILL);
    }
    if (hlsFFmpegPID != MAGICNEGATIVE) {
        printf("HLS FFMpeg killed...");
        kill(hlsFFmpegPID, SIGKILL);
    }
    if (dashFFmpegPID != MAGICNEGATIVE) {
        printf("DASH FFMpeg killed...");
        kill(dashFFmpegPID, SIGKILL);
    }

    printf("\nDone with killing FFMpeg processes...\n");

}

void Server::closeStream() {
    printf("Trying to close the stream...\n");

    for (const std::string& serverIdentifier : serverIdentifierList) {
        portal->closeStream(serverIdentifier);
    }
    printf("Stream closed\n");
}

void Server::interruptCallback(int signal) {
    printf("Caught the signal: %d!!\n", signal);

    Server::closeStream();

    Server::killFFMpeg();

    printf("Closing the server socket\n");
    close(server_socket_fd);

    printf("Trying to exit now...\n");
    _exit(0);
}

int Server::run(int argc, char* argv[]) {

    callbackOnInterrupt();
    commandLineParsing(argc, argv);

    int status = 0;

    try {

        std::string UUID = IceUtil::generateUUID();
        Ice::ObjectPrxPtr base = communicator()->propertyToProxy("Portal.Proxy");
        portal = Ice::checkedCast<PortalCommunicationPrx>(base);
        if (!portal) {
            throw "Invalid proxy";
        }

        StreamServerEntry allMyInfoTCP;

        std::string serverIdentifierTCP = UUID.substr(0, 8) + "-TCP";
        serverIdentifierList.push_back(serverIdentifierTCP);
        allMyInfoTCP.identifier = serverIdentifierTCP;

        allMyInfoTCP.name = moviename;
        allMyInfoTCP.keywords = keywords;
        allMyInfoTCP.videoSize = videosize;
        allMyInfoTCP.bitrate = bitrate;

        allMyInfoTCP.endpoint.ip = hostname;
        allMyInfoTCP.endpoint.port = std::to_string(portForClients).c_str();
        allMyInfoTCP.endpoint.transport = transportType;

        printf("\nI'm going to register a regular TCP stream on the portal...\n\n");
        portal->registerStreamServer(allMyInfoTCP);
        printf("Portal registration done!\n\n");

        regularFFmpegPID = vfork();
        if (regularFFmpegPID < 0) {
            perror("regularFFmpeg fork failed");
            raise(SIGINT);
            return 1;
        }

        if (regularFFmpegPID == 0) { // Child process to create TCP stream

            std::stringstream ss;
            ss << transportType << "://" << hostname << ":" << portForFFMPEG << "?listen=1";
            const std::string& tmp = ss.str();
            const char* whereToListen = tmp.c_str();

            std::cout << "|" << whereToListen << "|" << std::endl;

            execlp("ffmpeg", "ffmpeg", "-re", "-i", filename.c_str(), "-loglevel", "warning", "-s", videosize.c_str(),
                   "-c:v", encoder.c_str(), "-preset", "ultrafast", "-tune", "zerolatency", "-b:v", bitrate.c_str(),
                   "-f", "mpegts", whereToListen, NULL);

        } else { // Parent will only start executing after child calls execvp because we are using vfork()

            sleep(1); //wait for initial ffmpeg to be execed

            if (useDASH) {

                StreamServerEntry allMyInfoDASH;

                std::string serverIdentifierDASH = UUID.substr(0, 8) + "-DASH";
                serverIdentifierList.push_back(serverIdentifierDASH);
                allMyInfoDASH.identifier = serverIdentifierDASH;

                allMyInfoDASH.name = moviename;
                allMyInfoDASH.keywords = keywords;
                allMyInfoDASH.videoSize = videosize;
                allMyInfoDASH.bitrate = bitrate;
                allMyInfoDASH.endpoint.ip = hostname;
                allMyInfoDASH.endpoint.port = "8080";
                allMyInfoDASH.endpoint.transport = "http";
                allMyInfoDASH.endpoint.path = "/dash/" + UUID + ".mpd";

                printf("\nI'm going to register a regular DASH stream on the portal...\n\n");
                portal->registerStreamServer(allMyInfoDASH);
                printf("Portal registration of DASH is done!\n\n");

                dashFFmpegPID = vfork();
                if (dashFFmpegPID < 0) {
                    perror("dashFFmpeg fork failed");
                    raise(SIGINT);
                    return 1;
                }

                if (dashFFmpegPID == 0) { // Child process to create DASH stream

                    std::stringstream ss;
                    ss << transportType << "://" << hostname << ":" << portForClients;
                    const std::string& tmp = ss.str();
                    const char* whereToListenFrom = tmp.c_str();

                    std::stringstream ss2;
                    ss2 << "rtmp://localhost:1935/dash/" << UUID;
                    const std::string& tmp2 = ss2.str();
                    const char* rtmpURL = tmp2.c_str();

                    std::cout << "|" << rtmpURL << "|" << std::endl;

                    std::cout << "|" << whereToListenFrom << "|" << std::endl;

                    execlp("ffmpeg", "ffmpeg", "-re", "-i", whereToListenFrom, "-loglevel", "warning", "-vcodec",
                           "libx264", "-vprofile",
                           "baseline", "-acodec", "aac", "-ar", "44100", "-ac", "1", "-f", "flv", rtmpURL, NULL);
                } else {
                    printf("DASH is starting...");
                }
            }

            if (useHLS) {

                StreamServerEntry allMyInfoHLS;

                std::string serverIdentifierHLS = UUID.substr(0, 8) + "-HLS";
                serverIdentifierList.push_back(serverIdentifierHLS);
                allMyInfoHLS.identifier = serverIdentifierHLS;

                allMyInfoHLS.name = moviename;
                allMyInfoHLS.keywords = keywords;
                allMyInfoHLS.videoSize = videosize;
                allMyInfoHLS.bitrate = bitrate;
                allMyInfoHLS.endpoint.ip = hostname;
                allMyInfoHLS.endpoint.port = "8080";
                allMyInfoHLS.endpoint.transport = "http";
                allMyInfoHLS.endpoint.path = "/hls/" + UUID + ".m3u8";

                printf("\nI'm going to register a regular HLS stream on the portal...\n\n");
                portal->registerStreamServer(allMyInfoHLS);
                printf("Portal registration of HLS is done!\n\n");

                hlsFFmpegPID = vfork();
                if (hlsFFmpegPID < 0) {
                    perror("fork failed");
                    raise(SIGINT);
                    return 1;
                }

                if (hlsFFmpegPID == 0) { // Child process to create DASH stream

                    std::stringstream ss;
                    ss << transportType << "://" << hostname << ":" << portForClients;
                    const std::string& tmp = ss.str();
                    const char* whereToListenFrom = tmp.c_str();

                    std::stringstream ss2;
                    ss2 << "rtmp://localhost:1935/hls/" << UUID;
                    const std::string& tmp2 = ss2.str();
                    const char* rtmpURL = tmp2.c_str();

                    std::cout << "|" << rtmpURL << "|" << std::endl;

                    std::cout << "|" << whereToListenFrom << "|" << std::endl;

                    execlp("ffmpeg", "ffmpeg", "-re", "-i", whereToListenFrom, "-loglevel", "warning", "-vcodec",
                           "libx264", "-vprofile",
                           "baseline", "-acodec", "aac", "-ar", "44100", "-ac", "1", "-f", "flv", rtmpURL, NULL);
                } else {
                    printf("HLS is starting...\n");
                }

            }

            int n;
            int socketToReceiveVideoFD;
            struct addrinfo hints, *res, *ressave;

            const char* portToReceiveVideo;
            const char* ffmpegServer;
            char ffmpegBuffer[BUFFERSIZE];

            ffmpegServer = hostname.c_str();
            portToReceiveVideo = std::to_string(portForFFMPEG).c_str();

            printf("Will connect to FFMPEG on address |%s| and port |%s|\n", ffmpegServer, portToReceiveVideo);


            bzero((char*) &hints, sizeof(addrinfo));

            bzero(&hints, sizeof(struct addrinfo));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            if ((n = getaddrinfo(ffmpegServer, portToReceiveVideo, &hints, &res)) != 0) {
                printf("getaddrinfo error for %s, %s; %s", ffmpegServer, portToReceiveVideo, gai_strerror(n));
            }

            ressave = res;
            do {
                socketToReceiveVideoFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

                if (socketToReceiveVideoFD < 0) {
                    continue;
                }  /*ignore this returned Ip addr*/

                if (connect(socketToReceiveVideoFD, res->ai_addr, res->ai_addrlen) == 0) {
                    printf("Connection Ok!\n"); /* success*/
                    break;
                } else {
                    perror("Connecting to stream socket");
                }

            } while ((res = res->ai_next) != nullptr);

            freeaddrinfo(ressave);

            printf("Connection to FFMPEG succeded!!\n");

            //--------------SERVER PART-----------------//


            if (transportType == "tcp") {
                struct sockaddr_in server_address;

                //open socket
                server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (server_socket_fd < 0) {
                    perror("ERROR opening socket");
                    raise(SIGINT);
                    return 1;
                }

                int rc = fcntl(server_socket_fd, F_SETFL, O_NONBLOCK);
                if (rc < 0) {
                    perror("fcntl() failed");
                    raise(SIGINT);
                    return 1;
                }
                // set all values in server_address to 0
                bzero((char*) &server_address, sizeof(server_address));

                server_address.sin_family = AF_INET;
                server_address.sin_port = htons((uint16_t) portForClients);
                server_address.sin_addr.s_addr = INADDR_ANY;


                int bind_result = bind(server_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address));
                if (bind_result < 0) {
                    perror("ERROR on binding");
                    raise(SIGINT);
                    return 1;
                }

                rc = listen(server_socket_fd, 32);
                if (rc < 0) {
                    perror("listen() failed");
                    raise(SIGINT);
                    return 1;
                }


                printf("Ready to send video to clients! On address |%s| and port |%d|\n", hostname.c_str(),
                       portForClients);

                int counter = 0;

                while (true) {

                    int status_of_child;
                    pid_t result = waitpid(regularFFmpegPID, &status_of_child, WNOHANG);
                    if (result == 0) {
                        // Child still alive
                    } else {
                        break;
                    }

                    int newClientFD = accept(server_socket_fd, nullptr, nullptr);
                    if (newClientFD > 0) {
                        clientsSocketList.push_back(newClientFD);
                        printf("ADDED Client with SOCKET ---> %d !!!\n", newClientFD);
                    }

                    int numberOfWrittenElements = (int) read(socketToReceiveVideoFD, ffmpegBuffer, BUFFERSIZE);

//                counter++;

                    if (numberOfWrittenElements < 0) {
                        perror("ERROR reading from socket");
                        raise(SIGINT);
                        return 1;
                    } else if (numberOfWrittenElements == 0) {
                        printf("Stream is over??..\n");
                    } else {
//                    printf("%d. Bytes Read-> %d\n", counter, numberOfWrittenElements);
                    }

                    clientsSocketList.remove_if([ffmpegBuffer](int clientSocket) {

                        int bytesWritten = (int) write(clientSocket, ffmpegBuffer, BUFFERSIZE);
                        if (bytesWritten < 0) {
                            printf("REMOVED Client with SOCKET ---> %d !!!\n", clientSocket);
                            return true;
                        }

//                    printf("%%Client |%d| ---> received %d bytes %%\n", clientSocket, bytesWritten);
                        return false;
                    });

                }
            } else if (transportType == "udp") {

                struct sockaddr_in addr;
                int addrlen, sock, cnt;
                struct ip_mreq mreq;
                char message[BUFFERSIZE];

                bzero(message, sizeof(message));

                /* set up socket */
                sock = socket(AF_INET, SOCK_DGRAM, 0);
                if (sock < 0) {
                    perror("socket");
                    raise(SIGINT);
                    return 1;
                }
                bzero((char*) &addr, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = htonl(INADDR_ANY);
                addr.sin_port = htons(portForClients);
                addrlen = sizeof(addr);

                /* send */
                addr.sin_addr.s_addr = inet_addr("127.0.0.1");
                while (true) {
                    read(socketToReceiveVideoFD, message, BUFFERSIZE);
                    cnt = (int) sendto(sock, message, sizeof(message), 0, (struct sockaddr *) &addr, (socklen_t) addrlen);
                    if (cnt < 0) {
                        perror("sendto");
                        raise(SIGINT);
                        return 1;
                    }
                }

            } else {
                printf("you fucked up");
            }


        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    Server::closeStream();

    Server::killFFMpeg();

    printf("Closing the server socket\n");
    close(server_socket_fd);

    printf("Trying to exit now...\n");

    std::cout << "It's over" << std::endl;
    return status;

}

void commandLineParsing(int argc, char* argv[]) {

    try {
        TCLAP::CmdLine cmd("Streaming Server", ' ', "1.0", true);

        std::vector<std::string> allowedEnconders;
        allowedEnconders.emplace_back("libx264");
        allowedEnconders.emplace_back("libx265");
        TCLAP::ValuesConstraint<std::string> allowedEnc(allowedEnconders);

        std::vector<std::string> allowedTransportTypes;
        allowedTransportTypes.emplace_back("tcp");
        allowedTransportTypes.emplace_back("udp");
        TCLAP::ValuesConstraint<std::string> allowedTT(allowedTransportTypes);

        TCLAP::ValueArg<std::string> hostNameArg("", "host", "FFmpeg hostname", false, "localhost", "address");
        TCLAP::ValueArg<std::string> movieNameArg("n", "name", "Movie name", true, "", "name string");

        TCLAP::ValueArg<int> ffmpegPortArg("", "ff_port", "Port where FFMPEG is running", true, 0, "port number");
        TCLAP::ValueArg<int> clientsPortArg("", "my_port", "Port that will listen to clients", true, 0, "port number");

        TCLAP::SwitchArg hlsSwitchArg("", "hls", "Produce HLS stream", false);
        TCLAP::SwitchArg dashSwitchArg("", "dash", "Produce DASH stream", false);

        TCLAP::ValueArg<std::string> videoSizeArg("v", "videosize", "WIDTHxHEIGHT", true, "", "WIDTHxHEIGHT");
        TCLAP::ValueArg<std::string> bitRateArg("b", "bitrate", "bitrate", true, "", "bitrate in a string");
        TCLAP::ValueArg<std::string> encoderArg("e", "enconder", "Enconder", true, "", &allowedEnc);
        TCLAP::ValueArg<std::string> filenameArg("f", "filename", "Movie file name", true, "", "path string");
        TCLAP::ValueArg<std::string> transportTypeArg("t", "transport_type", "Transport Type", false, "tcp",
                                                      &allowedTT);

        TCLAP::MultiArg<std::string> keywordsArgs("k", "keyword", "keywords", false, "keyword");

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

        cmd.parse(argc, argv);

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

    } catch (TCLAP::ArgException& e) {  // catch any exceptions
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    }

}

int main(int argc, char* argv[]) {

    Server app;
    app.main(argc, argv);
}
