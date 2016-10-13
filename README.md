# Simple Web Crawler
An assignment for my Computer Networks Practice class.

### Requirements
* Generate text file containing list of servers (URL + response time)
* Construct proper HTTP messages and send them using the basic socket class
* Control the request rate by introducing some time delay
* Stop the crawler once a certain number of servers has been discovered
* Visit each URL only once
* Handle all exceptions

### Build
```sh
g++ crawler.cpp -o crawler
```

### Usage
```sh
./crawler
```
Note: You will have to modify the source code if you wish to change the initial seed URLs used by the crawler.

### Results
See [servers.txt](servers.txt) for a list of 100 servers discovered.

### Possible improvements
* Introduce the time delay specific to each host, instead of a blanket delay between each request
* Implement multi-threading
* Implement support for HTTPS
* Allow users to specify number of servers required and seed file as arguments
