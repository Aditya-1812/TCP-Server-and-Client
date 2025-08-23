# Multi-Client TCP Server & Client (C++)

A simple multi-client TCP server and client implemented in C++ using POSIX sockets.  
The project demonstrates network programming, concurrency (multi-threading), and custom protocol design.

 Features
1.Multi-threaded TCP Server
  Handles multiple client connections concurrently using `std::thread`.  

2.Custom Length-Prefixed Protocol
  Each message is sent with a 4-byte header indicating its size (safe message framing).  

3.Command-based System
  The server supports the following commands:  
  - `ECHO <message>` → Replies with the same message.  
  - `TIME` → Replies with the current server time.  
  - `UPPER <message>` → Replies with the message in uppercase.  
  - `QUIT` → Gracefully closes the connection.  

4.Client Application
  A simple interactive client that connects to the server, sends requests, and prints server replies.
