# libtcp_boost
An easy to use &amp; light-weight TCP server wrapper for boost::asio network library.

# Usage

Step 1 : Include Server.hpp, Session.hpp & Session.cpp in your project.

Step 2 : Inherit Session class to your own custom session class.

Step 3 : You need to override the following method(s);
        
        - OnDataReceived(void *,size_t);
        - OnConnected();
        - OnDisconnected();
        
Step 4 : Create a Server variable wherever you want to use it.
        
        - Server srv(3333); /* Creates a server that listens the port 3333 */
          srv.Run();        /* Start the server */
          
Step 5 : You're done. Enjoy using it!
        
