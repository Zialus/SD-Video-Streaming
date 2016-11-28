// Ice includes
#include <Ice/Ice.h>
#include <IceUtil/UUID.h>

// C
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// C++
#include <fstream>

// My stuff
#include "StreamServer.h"
#include "Auxiliary.h"

using namespace FCUP;

int
main(int argc, char* argv[])
{

	if (argc < 4)
	{
		fprintf(stderr,"ERROR, you need to provide two arguments: a file with the ffmpeg options, and a port to start ffmpeg on.");
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

		StreamServerEntry allMyInfo;

		StringSequence keywords = {"basketball","Cavs","indoor","sports"};
		allMyInfo.keywords = keywords;
		allMyInfo.name = IceUtil::generateUUID();

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

			// argv[2] contains txt file with ffmpeg options
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

			for (int i = 0; strings[i] != NULL; ++i)
			{
				printf("|%s|\n",strings[i]);
			}
			execvp(strings[0], strings);

		} else { // Parent will only start executing after child calls execvp because we are using vfork()

			int socket_to_receive_video_fd, port_number_to_receive_video;
			int number_of_written_elements;
			struct sockaddr_in ffmpeg_server_address;
			struct hostent *ffmpeg_server;

			char buffer[256];

			// argv[2] contains port number for the server
			port_number_to_receive_video = atoi(argv[2]);

			socket_to_receive_video_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (socket_to_receive_video_fd < 0)
			{
				perror("ERROR opening socket");
				exit(1);
			}

			// argv[1] contains name of the server
			ffmpeg_server = gethostbyname(argv[1]);
			if (ffmpeg_server == NULL)
			{
				fprintf(stderr,"ERROR, no such host");
				exit(0);
			}

			bzero( (char *) &ffmpeg_server_address, sizeof(ffmpeg_server_address) );
			ffmpeg_server_address.sin_family = AF_INET;
			bcopy( (char *) ffmpeg_server->h_addr, (char *) &ffmpeg_server_address.sin_addr.s_addr, ffmpeg_server->h_length );
			ffmpeg_server_address.sin_port = htons(port_number_to_receive_video);

			printf("|%u|\n", ffmpeg_server_address.sin_addr.s_addr);

			if ( connect(socket_to_receive_video_fd, (struct sockaddr *) &ffmpeg_server_address, sizeof(ffmpeg_server_address) ) < 0)
			{
				perror("ERROR connecting");
				exit(1);
			}

			number_of_written_elements = read(socket_to_receive_video_fd,buffer,255);
			if (number_of_written_elements < 0){
				perror("ERROR reading from socket");
				exit(1);
			}
			printf("%s",buffer);

			// //SERVER PART
			// int socket_fd, new_socket_fd, port_number, n;
			// socklen_t client_adress_size;
			// char buffer[256];
			// struct sockaddr_in server_address, client_address;
			
			// //open socket
			// socket_fd = socket(AF_INET, SOCK_STREAM, 0);
			// if (socket_fd < 0){
			// 	perror("ERROR opening socket");
			// 	exit(1);
			// }

			// // set all values in server_adress to 0
			// bzero((char *) &server_address, sizeof(server_address));
			// // argv[1] contains port number for the server
			// port_number = atoi(argv[1]);
			// server_address.sin_family = AF_INET;
			// server_address.sin_port = htons(port_number);
			// server_address.sin_addr.s_addr = INADDR_ANY;

			// int bind_result = bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));

			// if ( bind_result < 0 ){
			// 	perror("ERROR on binding");
			// 	exit(1);
			// }

			// listen(socket_fd,5);

			// client_adress_size = sizeof(client_address);
			// new_socket_fd = accept(socket_fd, (struct sockaddr *) &client_address, &client_adress_size);
			// if (new_socket_fd < 0){
			// 	perror("ERROR on accept");
			// 	exit(1);
			// }

			// bzero(buffer,256);
			// n = read(new_socket_fd,buffer,255);
			// if (n < 0){
			// 	perror("ERROR reading from socket");
			// 	exit(1);
			// }
			// printf("Here is the message: %s",buffer);

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
