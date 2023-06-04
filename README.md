# Multithread HTTP File Downloader
Linux command line tool to download file from HTTP Server using C/C++.

## Feature
Download multiple file at one time.

Track download status in real-time.

Option to limit number of concurrent downloads.

Option to start new downloads at runtime.

Option to pause downloads.

Option to continue download from paused downloads.

## Instructions

In the directory where the file needs to be hosted, run the server.py file with python3.

To download a file, run the executable file with the command: Download <link1> <link2> ... <linkN> --output <directory> --limit <number>

To stop downloading a file, enter p<index>

To resume downloading, enter c<index>

To download a new file, enter n<file path>
  
## Issues
When entering input, the display of the values that have not been fully entered will be erased each time it is updated. This does not affect the operation of the program, just make sure to enter the correct command, and the program will work properly.
