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

// My stuff
#include "StreamServer.h"
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;


std::string serverName;
PortalCommunicationPrx portal;


void closeStream(){
    portal->closeStream(serverName);
}

void my_handler(int s){
    printf("Caught ctrl+c Bye! %d\n",s);
    closeStream();
    exit(0);
}

void sendVideoTo(int socket, char* ffmpeg_buffer) {
    int number_of_written_elements = (int) write(socket, ffmpeg_buffer, 63);
    if (number_of_written_elements < 0){
        perror("ERROR writing to socket");
        exit(1);
    }
}

int main(int argc, char* argv[])
{

    if (argc < 4)
    {
        fprintf(stderr,"ERROR, you need to provide 3 arguments: a file with the ffmpeg options, a port to start ffmpeg on and a file with commands");
        exit(1);
    }

    int status = 0;
    Ice::CommunicatorPtr ic;
    try {
        ic = Ice::initialize(argc, argv);
        Ice::ObjectPrx base = ic->stringToProxy("Portal:default -p 9999");
        PortalCommunicationPrx portal = PortalCommunicationPrx::checkedCast(base);
        if (!portal){
            throw "Invalid proxy";
        }

        struct sigaction sigIntHandler;
        sigIntHandler.sa_handler = my_handler;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;
        sigaction(SIGINT, &sigIntHandler, NULL);

        StreamServerEntry allMyInfo;

        StringSequence keywords = {"basketball","Cavs","indoor","sports"};
        allMyInfo.keywords = keywords;
        serverName = IceUtil::generateUUID();
        allMyInfo.name = serverName;

        portal->registerStreamServer(allMyInfo);

        pid_t pid = vfork();
        if ( pid < 0 ) {
            perror("fork failed");
            return 1;
        }

        if ( pid == 0 ) { // Child process

            char** strings = NULL;
            size_t count = 0;
            char string[1000];

            // argv[3] contains txt file with ffmpeg options
            std::ifstream file(argv[3]);

            if(file.is_open()){
                file >> string;
                while (!file.eof() ) {
                    AddString(&strings, &count, string);
                    file >> string;
                }
                file.close();
            } else{
                std::cout << "File could not be opened." << std::endl;
            }
            AddString(&strings, &count, NULL);

            /* char* argv[200] = {"ffmpeg","-i","/home/tiaghoul/Downloads/Popeye_forPresident_512kb.mp4","-loglevel","warning",
            "-analyzeduration","500k","-probesize","500k","-r","30","-s","640x360","-c:v","libx264","-preset","ultrafast","-pix_fmt",
            "yuv420p","-tune","zerolatency","-preset","ultrafast","-b:v","500k","-g","30","-c:a","flac","-profile:a","aac_he","-b:a",
            "32k","-f","mpegts","tcp://127.0.0.1:10000?listen=1",NULL};*/

            for (int i = 0; strings[i] != NULL; ++i) {
                printf("|%s|\n",strings[i]);
            }
            execvp(strings[0], strings);

        } else { // Parent will only start executing after child calls execvp because we are using vfork()

            sleep(1);

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

            do{
                socketToReceiveVideoFD=socket(res->ai_family, res->ai_socktype, res->ai_protocol);

                if(socketToReceiveVideoFD<0)
                    continue;  /*ignore this returned Ip addr*/

                if(connect(socketToReceiveVideoFD, res->ai_addr, res->ai_addrlen)==0) {
                    printf("connection ok!\n"); /* success*/
                    break;
                } else  {
                    perror("connecting stream socket");
                }

            } while ((res=res->ai_next)!= NULL);

            freeaddrinfo(ressave);

            printf("Connection to FFMPEG succeded!!");
            sleep(1);

            //--------------SERVER PART-----------------//

            socklen_t client_adress_size;
            struct sockaddr_in server_address, client_address;

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
            int port_number = 10066;
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(port_number);
            server_address.sin_addr.s_addr = INADDR_ANY;

            int yes=1;

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

            std::list<int> socketList;

            printf("Ready to send to clients!\n");

            while (true) {

                int new_socket_fd = accept(server_socket_fd, NULL, NULL);
                if (new_socket_fd > 0){
                    socketList.push_back(new_socket_fd);
                }

                numberOfWrittenElements = (int) read(socketToReceiveVideoFD, ffmpegBuffer, 63);
                if (numberOfWrittenElements < 0){
                    perror("ERROR reading from socket");
                    exit(1);
                } else{
                    printf("Number -> %d\n", numberOfWrittenElements);
                }

                for(int socket: socketList){
                    printf("SOCKET -> %d !!!\n", socket);
                    sendVideoTo(socket, ffmpegBuffer);
                }

            }

        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    if (ic){
        ic->destroy();
    }

    return status;
}
