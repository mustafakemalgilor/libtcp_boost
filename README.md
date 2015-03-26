# libtcp_boost
An easy to use &amp; light-weight TCP server wrapper for boost::asio network library.

# Usage

Step 1 : Include Server.hpp, Session.hpp & Session.cpp in your project.

Step 2 : Inherit Session class to your own custom session class.

Step 3 : You need to override the following method(s);
        
        - OnDataReceived(void *,size_t); 
			Invokes whenever data received from the remote endpoint. (void*) is actually (unsigned char *)
			and contains the received data. size_t is the received data's length.
        - OnConnected();
			Invokes when the session is successfully connected.
        - OnDisconnected();
			Invokes immediately the session is disconnected.
        
Step 4 : Create a Server variable wherever you want to use it.
        
        - Server srv(3333); /* Creates a server that listens the port 3333 */
          srv.Run();        /* Start the server */
          
Step 5 : You're done. Enjoy using it!

# Performance Reports

Test platform : Intel Core i7-4710HQ, 2.5 GHz Processor, 16 gigabytes of ram(1600mhz), 1 TB SSD(540R,520W)
Both client and server is running on the same machine.

- The memory stress test

 - Test terms : 5 minute(s) runtime, 2500 sessions with 50 milliseconds packet interval and 16k message chunk size on client side.

- Results
	- Initial Memory Usage(idle) : 1.1 megabyte(s)
	- Memory usage in runtime between 0:30 and 5:00 : 48.7 megabyte(s)
	- After clients disconnected: 42.14 mebabytes deallocated.
	- Memory allocation for each session : 16572 bytes.
	- Memory allocation for server class : 26470 bytes.

- The CPU usage test

- Test terms : 5 minute(s) runtime, 2500 sessions with with 50 milliseconds packet interval and 16k message chunk size on client side.

- Results
	- Initial CPU Utilization(idle) : %0.0
	- CPU utilization in runtime between 0:30 and 5:00 : %8.41
	- After clients disconnected : %0.0
        
