// Ice includes
#include <Ice/Ice.h>
#include <IceUtil/UUID.h>

// C
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

// C++
#include <fstream>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <IceUtil/CtrlCHandler.h>

// My stuff
#include "StreamServer.h"
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;

std::string serverName;
PortalCommunicationPrx portal;
std::list<int> clientsSocketList;

class Server : public Ice::Application {
public:
    Server();
    virtual int run(int, char*[]);
    static void destroyComm();
    static void closeStream();
};

void my_handler(int s){
    printf("Caught signal %d\n",s);
    Server::closeStream();
    Server::destroyComm();
    printf("Exiting now\n");
    exit(0);
}

void Server::closeStream(){
    printf("Gonna close the stream\n");
    portal->closeStream(serverName);
    printf("Stream closed\n");
}


Server::Server() : Ice::Application(Ice::NoSignalHandling){
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGSTOP, &sigIntHandler, NULL);
    sigaction(SIGKILL, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
};

void Server::destroyComm() {
    if (communicator()){
        communicator()->destroy();
    }
}

int Server::run(int argc, char* argv[]) {

    if (argc < 9)
    {
        fprintf(stderr,"ERROR, you need to provide 3 arguments: a file with the ffmpeg options, a port to start ffmpeg on and a file with commands");
        exit(1);
    }

    int status = 0;

    try {

        Ice::ObjectPrx base = communicator()->stringToProxy("Portal:default -p 9999");
        portal = PortalCommunicationPrx::checkedCast(base);
        if (!portal){
            throw "Invalid proxy";
        }



        char* hostname = argv[1];
        int port = atoi(argv[2]);
        int portForClients = atoi(argv[3]);
        std::stringstream ss;
        ss << "tcp://" << hostname << ":" << port << "?listen=1";
        const std::string& tmp = ss.str();
        const char* whereToListen = tmp.c_str();
        char* videosize= argv[4];
        char* bitrate = argv[5];
        char* encoder = argv[6];
        char* filename = argv[7];
        char* transportType = argv[8];
        std::cout << whereToListen << " !! " << filename << std::endl;

        StreamServerEntry allMyInfo;

        char* keywordsWithCommas = argv[9];

        std::string str(keywordsWithCommas);

        std::vector<std::string> keywordVector = split(keywordsWithCommas, ',');

        StringSequence keywords;

        for (std::string keyword: keywordVector) {
            keywords.push_back(keyword);
        }

        allMyInfo.keywords = keywords;
        serverName = IceUtil::generateUUID();
        allMyInfo.name = serverName;
        allMyInfo.videoSize = videosize;
        allMyInfo.bitrate = bitrate;
        allMyInfo.endpoint.ip = hostname;
        allMyInfo.endpoint.port = argv[3];
        allMyInfo.endpoint.transport = transportType;


        printf("PRINT ANTES\n");
        portal->registerStreamServer(allMyInfo);
        printf("PRINT DEPOIS\n");

        pid_t pid = vfork();
        if ( pid < 0 ) {
            perror("fork failed");
            return 1;
        }

        if ( pid == 0 ) { // Child process

            execlp("ffmpeg","ffmpeg","-re","-i",filename,"-loglevel","warning",
                   "-analyzeduration","500k","-probesize","500k","-r","30","-s",videosize,"-c:v",encoder,"-preset","ultrafast","-pix_fmt",
                   "yuv420p","-tune","zerolatency","-preset","ultrafast","-b:v", bitrate,"-g","30","-c:a","flac","-profile:a","aac_he","-b:a",
                   "32k","-f","mpegts",whereToListen,NULL);

        } else { // Parent will only start executing after child calls execvp because we are using vfork()

            int n;
            int socketToReceiveVideoFD;
            int numberOfWrittenElements;
            struct addrinfo hints, *res, *ressave;

            char *portToReceiveVideo;
            char *ffmpegServer;
            char ffmpegBuffer[64];

            // argv[1] contains name of the server
            ffmpegServer = argv[1];
            // argv[2] contains port number for the server
            portToReceiveVideo = argv[2];

            printf("Video vai ser recebido na porta %s e no adress %s\n", portToReceiveVideo, argv[1]);

            bzero( (char *) &hints, sizeof(addrinfo) );

            bzero(&hints, sizeof(struct addrinfo));
            hints.ai_family=AF_UNSPEC;
            hints.ai_socktype=SOCK_STREAM;
            hints.ai_protocol=IPPROTO_TCP;

            if((n=getaddrinfo(ffmpegServer, portToReceiveVideo, &hints, &res))!=0) {
                printf("getaddrinfo error for %s, %s; %s", ffmpegServer, portToReceiveVideo, gai_strerror(n));
            }

            ressave=res;
            sleep(1); //waiting for ffserver
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
                std::exit(1);
            }

            int rc = fcntl(server_socket_fd, F_SETFL, O_NONBLOCK);
            if(rc<0){
                perror("fcntl() failed");
                close(server_socket_fd);
                exit(-1);
            }
            // set all values in server_adress to 0
            bzero((char *) &server_address, sizeof(server_address));
            // argv[1] contains port number for the server
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons((uint16_t) portForClients);
            server_address.sin_addr.s_addr = INADDR_ANY;


            int bind_result = bind(server_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
            if ( bind_result < 0 ) {
                perror("ERROR on binding");
                close(server_socket_fd);
                exit(1);
            }

            rc = listen(server_socket_fd, 32);
            if (rc < 0) {
                perror("listen() failed");
                close(server_socket_fd);
                exit(1);
            }


            printf("Ready to send to clients!\n");

            while (true) {

                int new_socket_fd = accept(server_socket_fd, NULL, NULL);
                if (new_socket_fd > 0){
                    clientsSocketList.push_back(new_socket_fd);
                }

                numberOfWrittenElements = (int) read(socketToReceiveVideoFD, ffmpegBuffer, 63);

                if (numberOfWrittenElements < 0){
                    perror("ERROR reading from socket");
                    exit(1);
                }   else if (numberOfWrittenElements == 0){
                    printf("Stream is over..\n");
                    //break;
                }
                else{
                    printf("Number -> %d\n", numberOfWrittenElements);
                }

                clientsSocketList.remove_if([ffmpegBuffer](int clientSocket)  {

                    auto bytesWritten = write(clientSocket, ffmpegBuffer, 63);
                    if (bytesWritten < 0) {
                        printf("SOCKET DENIED ---> %d !!!\n", clientSocket);
                        return true;
                    }

                    printf("SOCKET -> %d | %ld !!!\n", clientSocket, bytesWritten);
                    return false;
                });

            }
            closeStream();
        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    destroyComm();
    return status;

}



int main(int argc, char* argv[])
{
    Server app;
    app.main(argc,argv, NULL);
}
