/* Remarks */
/*
		- HEADER :
			Constant -beginning of packet- for protocol.
		- TAIL   :
			Constant -end of packet- value for protocol.
		- SIZE_TYPE : 
			The type that will be used for protocol's size indicator.
			Available values :
			2 for int16, 4 for int32, 8 for int64
		- RECV_BUFFER_SIZE :
			The amount will be reserved for receive ring buffer.
		- MAX_AVAILABLE_BUFFER_AMOUNT :
			The maximum amount of data that is stored in internal socket buffer.
			If any session exceeds this amount, it will automatically disconnected.
		- MAX_INDIVIDUAL_PACKET_SIZE :
			The maximum amount of data that can be stored in a single packet.
			If any session sends data that exceeds this value will automatically disconnected.
		- MAX_PACKET_FRAGMENT_COUNT :
			The packet fragment threshold. If any session exceeds defined value,
			it'll be automatically disconnected.
		- USE_TCP_NODELAY :
			Define this to disable nagle's algorithm.
		- USE_KEEP_ALIVE :
			Define this to enable keep alive.
		- INTERNAL_SEND_BUFFER_SIZE :
			Defines the buffer size of socket for sending operation. (internal buffer, not ring.)
		- INTERNAL_RECV_BUFFER_SIZE :
			Defines the buffer size of socket for receiving operation. (internal buffer, not ring.)
		- AUTOKICK_INACTIVE_SESSIONS :
			If defined, enables kicking mechanism for inactive sessions according to last response time.
			The no response period can be set by changing MAX_NO_RESPONSE_PERIOD definition.
		- MAX_NO_RESPONSE_PERIOD :
			Defines the maximum time period without receiving any data from session.
			If this time exceeds, the session will be disconnected automatically.
		- AUTOKICK_ON_EXCESSIVE_TRANSMISSION :
			If defined, enables kicking mechanism for sessions who are sending too much data(flooding).
			The data limit can be set by changing the MAX_TRANSMISSION_AMOUNT value.
		- MAX_TRANSMISSION_AMOUNT :
			Defines the maximum data amount for data waiting to be read in the network buffer.
			The values that are too low is not recommended.
		- AUTOKICK_ON_EXCESSIVE_TRANSMISSION :
			If defined, enables kicking mechanism for sessions who are sending too much packet in a second.
			The limit can be set by changing the MAX_RECV_COUNT_PER_SEC value.
		- MAX_RECV_COUNT_PER_SEC :
			Defines the maximum packet amount for incoming packet(s) per second.
		- MIN_RECV_TRANSFER_AMOUNT :
			Defines the minimum amount of data needed to be received before the receiver callback invoked.
			It is a good strategy to set the value to header + size length.
			Default is 7.
		- USE_SHARED_OBJECT_POOL :
			If defined, the session(s) will fetch receive buffer from pool instead of allocating new one.
			The pool size can be changed by altering RECEIVE_POOL_AMOUNT value.
		- RECEIVE_POOL_AMOUNT :
			The size of the receive pool.
*/


#define HEADER						"\xAA\x55"		/* AA55 */
#define TAIL						"\x55\xAA"		/* 55AA*/

#define SIZE_TYPE 4

#define RECV_BUFFER_SIZE			16384	/* bytes */
#define MAX_TRANSMISSION_AMOUNT	    16384	/* bytes */
#define MAX_INDIVIDUAL_PACKET_SIZE  16384	/* bytes */
#define MIN_RECV_TRANSFER_AMOUNT    7		/* bytes */
#define RECEIVE_POOL_AMOUNT         32767   /* bytes */

#define MAX_PACKET_FRAGMENT_COUNT	10
#define MAX_RECV_COUNT_PER_SEC      8


#define USE_TCP_NODELAY
#define USE_KEEP_ALIVE
#define USE_SHARED_OBJECT_POOL
#define AUTOKICK_INACTIVE_SESSIONS
#define AUTOKICK_ON_EXCESSIVE_TRANSMISSION
//#define AUTOKICK_ON_EXCESSIVE_RECV

#define INTERNAL_SEND_BUFFER_SIZE 16384
#define INTERNAL_RECV_BUFFER_SIZE 16384

#define MAX_NO_RESPONSE_PERIOD    (60*30) * 1000 /* 30 minute(s) as milliseconds. */