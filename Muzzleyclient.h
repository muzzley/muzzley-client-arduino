#ifndef MUZZLEYCLIENT_H_
#define MUZZLEYCLIENT_H_

#include <SPI.h>
#include <Ethernet.h>
#include <WSClient.h>

class Muzzleyclient {

public:
	void begin(Client &client);
	void disconnect(Client &client);
	void power(Client &client, char* server);
	void statemachine(Client &client);
	void connect(Client &client);
    typedef void Action(Muzzleyclient &socket, char* socketString);
    void addAction(Action *socketAction);
  	int getParticipantID(char* rxdata);
  	int getCID(char* rxdata, int version);
  	void parseObj(char* jsonString);
  	int findpos(char* text, int index);

  typedef struct {
      int object;
      int namecounter;
      char name[2][10];
      char value[2][10];
  }  ActionObj;
  ActionObj actionobj[2];


private:
	Client *socket_client;
	bool firstconnect;
	bool ready;
	bool reconnect;
	int cid;
	String token;
	String activity;
	WSClient websocket;
	char* getStringTableItem(int index, int send);
	void changeServer(char* rxdata);
	char* rpl(int strvar, int index, char* stradd);
	int getStrigTableItemSize(int index);
	struct ActionPack {
    	Action *socketAction;
    } socket_actions[CALLBACK_FUNCTIONS];

    int socket_actions_population;
    void executeActions(char* socketString);

};




#endif