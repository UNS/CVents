# CVents
Don`t forget to change the sample parameters to the necessary ones. at the file app.js:

#define WIFI_NETWORK "`your wifi name`"  
#define WIFI_PASSWORD "`password`"  
#define HOST_ADDR "`hostname.or.address`"  
#define HOST_PORT `8080` // port number  

and look at the end of the file sketch_feb23a.ino:

server.listen(`8080`, '`some_ip`');

and, if you don't use ngrok 

applicationId: '`skill_id`'
