#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex>
#include <map>
#include <queue>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

#define NUM_SERVERS 100

void fetch(string host, string path, map<string, double>* servers, map<string, int>* links, queue< pair<string, string> >* queue) {
	const int port = 80;
	const int MAX_BUFFER_SIZE = 4096;

	// Create socket
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot open socket");
		return;
	}

	// Get host
	struct hostent *server;
	server = gethostbyname(host.c_str());
	if (server == NULL) {
		perror("No such host");
		return;
	}

	// Fill in the details
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

	// Connect to server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
		perror("Cannot connect to server");
		return;
	}
	//printf("Connected to %s at port %d\n\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

	// Record that the link has been visited
	cout << "Visiting " << host << " " << path << "\n";
	links->insert(pair<string, int>(host + path, 0));

	// Start timer
	clock_t start = clock();
	double duration;

	// Send request
	string request = "GET " + path + " HTTP/1.1\r\n";
	request.append("Host: " + host + "\r\n");
	request.append("Accept: text/html\r\n");
	request.append("Connection: close\r\n");
	request.append("\r\n");
	if ((send(sockfd, request.c_str(), request.size(), 0)) < 0) {
		perror("Error writing to socket");
		return;
	}
	cout << "\nRequest:\n" << request;

	// Receive response
	int bytes_received;
	char buffer[MAX_BUFFER_SIZE];
	string response = "";
	bool header_parsed = false;
	bool text_html = true;
	do {
		memset(buffer, 0, MAX_BUFFER_SIZE);
		bytes_received = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
		if (bytes_received < 0) {
			perror("Error reading from socket");
			return;
		}
		response.append(buffer);

		// Parse response header
		if (!header_parsed) {
			header_parsed = true;
			//cout << response << "\n\n";

			// Stop timer and record the server response time (of the most recent visit)
			duration = (clock() - start) / (double) CLOCKS_PER_SEC;
			servers->insert(pair<string, double>(host, duration));

			// Check the content type
			string content_type = "";
			string s = response;
			smatch match;
			regex rgx("Content-Type: .*");
			while (regex_search(s, match, rgx)) {
				content_type = match[0];
				s = match.suffix().str();

				// Check only the first occurrence
				break;
			}

			// Stop receiving the rest of the response if content type is not text/html
			if (content_type.substr(0,23).compare("Content-Type: text/html") != 0) {
				cout << "Content-type is not text/html\n";
				text_html = false;
				break;
			}
		}
	} while (bytes_received != 0);

	// Parse response for URLs if content type is text/html
	if (text_html) {
		int num_links = 0;
		string s = response;
		smatch match;
		regex rgx("<a\\s+(?:[^>]*?\\s+)?href=\"([^\"]*)\"");
		while (regex_search(s, match, rgx)) {
			string url = match[1];
			s = match.suffix().str();

			// Filter the URLs to get new host and path (only HTTP is supported)
			string new_host = "";
			string new_path = "";
			if (url.substr(0,1) == "/") {
				new_host = host;
				new_path = url;
			} else if (url.substr(0,7) == "http://") {
				int pos = url.substr(7).find_first_of('/');
				new_host = url.substr(7,pos);
				new_path = url.substr(7+pos);
			}

			// Add new link (new_host + new_path) to queue
			if (new_host != "") {
				queue->push(pair<string, string>(new_host, new_path));
				num_links++;
			}
		}
		cout << "Added " << num_links << " URL(s) to queue\n";
	}

	// Close socket
	close(sockfd);
}

int main() {
	map<string, double> servers;
	map<string, int> links;
	queue< pair<string, string> > queue;

	// Initial seed(s)
	queue.push(pair<string, string>("www.nus.edu.sg", "/"));
	queue.push(pair<string, string>("www.ntu.edu.sg", "/"));
	queue.push(pair<string, string>("www.smu.edu.sg", "/"));
	queue.push(pair<string, string>("www.sutd.edu.sg", "/"));

	// Fetch links in the queue until number of servers reaches the limit
	while (queue.size() > 0 && servers.size() < NUM_SERVERS) {
		cout << "====================\n\n";

		string host = queue.front().first;
		string path = queue.front().second;
		queue.pop();

		// Check if link has been fetched before
		map<string, int>::iterator it = links.find(host + path);
		if (it == links.end()) {
			fetch(host, path, &servers, &links, &queue);

			// Add delay after fetching
			sleep(1);
		} else {
			cout << "Already visited " << host << " " << path << "\n";
		}

		cout << "\n";
	}

	// Write to file
	ofstream write("servers.txt");
	map<string, double>::iterator it;
	for (it = servers.begin(); it != servers.end(); it++) {
		write << it->first << " " << it->second << "\n";
	}
	write.close();
	if (!write) {
		perror("Error writing to file");
		exit(1);
	}

	return 0;
}
