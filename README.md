# Multi_Processes_Image_Concatenation
## Objective
The objective is to request all horizontal strips of a picture from the server and concatenate them to reconstruct the original image. Given the randomness in strip delivery, utilizing a loop to repeatedly request random strips may result in receiving the same strip multiple times before obtaining all fifty distinct strips. As a consequence of this randomness, the time required to gather all necessary strips for image restoration will vary. 

When accessing the server, the server will randomly display 1 out of 50 images shown as such: \
![image](https://github.com/Max00358/PNG_Finding_and_Concatentation/assets/125518862/a73de071-2c4e-43c6-a5fa-6715001fb8db)

The code will take in the image in a loop and only end when all the images are received from the server and concatenated together into a full, recognizable image like such:

![all](https://github.com/Max00358/PNG_Finding_and_Concatentation/assets/125518862/5939b620-aaf6-48dd-be9d-fbbab4c17f0e)

## Producer-Consumer Problem with given context
In this lab, we have a different server running at port 2530 which sleeps for a fixed time
before it sends a specific image strip requested by the client. The deterministic
sleep time in the server is to simulate the time to produce the data. The image
format sent by the server is still the simple PNG format (see Figure 1.2(a)). The
PNG segment is still an 8-bit RGBA/color image (see Table 1.1 for details). The
web server still uses an HTTP response header that includes the sequence number
to tell you which strip it sends to you. The HTTP response header has the for-
mat of “X-Ece252-Fragment: M ” where M ∈ [0, 49]. To request a horizontal strip
with sequence number M of picture N , where N ∈ [1, 3], use the following URL:
http://machine:2530/image?img=N&part=M, where
machine is one of the following:

* ece252-1.uwaterloo.ca,
* ece252-2.uwaterloo.ca, and
* ece252-3.uwaterloo.ca 

For example, when you request data from http://ece252-1.uwaterloo.ca:2530/
image?img=1&part=2, you will receive a horizontal image strip with sequence num-
ber 2 of picture 1 . The received HTTP header will contain “X-Ece252-Fragment:
2“. The received data will be the image segment in PNG format. You may use the
browser to view a horizontal strip of the PNG image the server sends. Each strip has
the same dimensions of 400 × 6 pixels and is in PNG format.

Your objective is to request all horizontal strips of a picture from the server and
then concatenate these strips in the order of the image sequence number from top to
bottom to restore the original picture as quickly as possible for a given set of given
input arguments specified by the user command. You should name the concatenated
image as all.png and output it to the current working directory.

There are three types of work involved. The first is to download the image seg-
ments. The second is to process downloaded image data and copy the processed
data to a global data structure for generating the concatenated image. 
The third is to generate the concatenated all.png file once the global data structure that holds the concatenated image data is filled.
The producers will make requests to the lab web server and together they will
fetch all 50 distinct image segments. Each time an image segment arrives, it gets
placed into a fixed-size buffer of size B, shared with the consumer tasks. When
there are B image segments in the buffer, producers stop producing. When all 50
distinct image segments have been downloaded from the server, all producers will
terminate. That is the buffer can take maximum B items, where each item is an
image segment. The horizontal image strips sent out by the lab servers are all less
than 10, 000 bytes.

Each consumer reads image segments out of the buffer, one at a time, and then
sleeps for X milliseconds specified by the user in the command line4. Then the consumer will process the received data. The main work is to validate the received
image segment and then inflate the received IDAT data and copy the inflated data
into a proper place inside the memory.

Given that the buffer has a fixed size, B, and assuming that B < 50, it is possible
for the producers to have produced enough image segments that the buffer is filled
before any consumer has read any data. If this happens, the producer is blocked,
and must wait till there is at least one free spot in the buffer.
Similarly, it is possible for the consumers to read all of the data from the buffer,
and yet more data is expected from the producers. In such a case, the consumer is
blocked, and must wait for the producers to deposit one or more additional image
segments into the buffer.

Further, if any given producer or consumer is using the buffer, all other consumers and producers must wait, pending that usage being finished. That is, all
access to the buffer represents a critical section, and must be protected as such.
The program terminates when it finishes outputting the concatenated image seg-
ments in the order of the image segment sequence number to a file named all.png.
Note that there is a subtle but complex issue to solve. Multiple producers are
writing to the buffer, thus a mechanism needs to be established to determine whether
or not some producer has placed the last image segment into the buffer. Similarly,
multiple consumers are reading from the buffer, thus a mechanism needs to be es-
tablished to determine whether or not some consumer has read out the last image
segment from the buffer.

## Sample Test Run
Let B be the buffer size, P be the number of producers, C be the number of consumers, X be the number of milliseconds that a consumer sleeps before it starts to
process the image data, and N be the image number you want to get from the server.
The producer consumer system is called with the execution command syntax of:

* ./paster2 \<B\> \<P\> \<C\> \<X\> \<N\>

The following is an example execution of paster2 given (B, P, C, X, N ) = (2, 1, 3, 10, 1).
In this example, the bounded buffer size is 2. We have one producer to download
the image segments and three consumers to process the downloaded data. Each 
consumer sleeps 10 milliseconds before it starts to process the data. And the image
segments requested are from image 1 on lab servers.

* ./paster2 2 1 3 10 1
* paster2 execution time: 100.45 seconds

Note that due to concurrency, your output may not be exactly the same as the sam-
ple output above. Also depending on the implementation details and the platform
where the program runs, the sample system execution time is only for illustration
purpose. The exact paster2 execution time value your program produces will be
different than the one shown in the sample run.
