/* Remarks */
/*
		KEEP_ALIVE_INTERVAL :
			This value determines the gap between two session status check.
			The 'keep alive' service checks for broken connections.
			Value unit is seconds.
		STAT_UPDATE_INTERVAL :
			The interval for server's internal stat update (such as recv/send rate)
			Default is 1.	
			Value unit is seconds.
		MAXIMUM_PENDING_CONNECTIONS : 
			The maximum pending queue amount for acceptor.
			If this value is exceeded, the incoming connection attempt(s) will be refused.
		MAXIMUM_ALLOWED_CONNECTION :
			The maximum session amount.
		USE_MAXIMUM_CONNECTION_LIMIT_PER_IP :
			If defined, every IP will have a maximum limit to establish connection.
			The limit is alterable by changing MAXIMUM_CONNECTION_ALLOWED_PER_IP value.
		MAXIMUM_CONNECTION_ALLOWED_PER_IP :
			Maximum connection limit for per IP.
		REPORT_MEMORY_USAGE :
			Shows the memory used by process on top of title bar.

*/


#define KEEP_ALIVE_INTERVAL  10 
#define STAT_UPDATE_INTERVAL 1	
#define MAXIMUM_PENDING_CONNECTIONS 100
#define MAXIMUM_ALLOWED_CONNECTION 500
#define REPORT_MEMORY_USAGE

#if defined USE_MAXIMUM_CONNECTION_LIMIT_PER_IP
	#define MAXIMUM_CONNECTION_ALLOWED_PER_IP 10
#endif