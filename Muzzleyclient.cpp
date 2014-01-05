#include <SPI.h>
#include "Muzzleyclient.h"
#include <Ethernet.h>
#include <WSClient.h>
#include <MemoryFree.h>


bool keepconnected = true;

// TODO: improve this array to be dynamic. This will fail is the number of chars is different on the new server
char newserver[] = "platform.geo.muzzley.com";


// Using consts to reduce memory usage by storing this statics into disk
const prog_char stringVar[] PROGMEM = "{0}";
const prog_char clientHandshakeLine1[] PROGMEM = "Handshake validated";
const prog_char clientHandshakeLine2[] PROGMEM = "sessionId";
const prog_char clientHandshakeLine3[] PROGMEM = "connectTo\":\""; // used as the prefix to find the newserver to connect to
const prog_char clientHandshakeLine4[] PROGMEM = "Activity moved";
const prog_char clientHandshakeLine5[] PROGMEM = "activityId";
const prog_char clientHandshakeLine6[] PROGMEM = "\"a\":\"participantJoined\"";
const prog_char clientHandshakeLine7[] PROGMEM = "{\"h\":{\"cid\":\"1\",\"t\":1},\"a\":\"loginApp\",\"d\":{\"token\":\"bb7aa55a4d56f1b5\"}}";
const prog_char clientHandshakeLine8[] PROGMEM = "{\"h\":{\"cid\":\"1\",\"t\":1},\"a\":\"create\",\"d\":{\"activityId\":\"7145d4\"}}";
const prog_char clientHandshakeLine9[] PROGMEM = "{\"h\":{\"cid\":\"1\",\"t\":1},\"a\":\"handshake\",\"d\":{\"protocolVersion\":\"1.1\",\"lib\":\"v01\",\"userAgent\":\"Ar\"}}";
const prog_char clientHandshakeLine10[] PROGMEM = ",\"d\":{";
const prog_char clientHandshakeLine11[] PROGMEM = "\"d\":{\"a\":\"ready\"}";
const prog_char clientHandshakeLine12[] PROGMEM = "\",\"t\":1,\"pid\":"; // prefix of the PID. Used for extraction
const prog_char clientHandshakeLine13[] PROGMEM = "},\"a\":\"signal\","; // sufix of the PID. Used for extraction
const prog_char clientHandshakeLine14[] PROGMEM = "{\"h\":{\"cid\":\"{0}\",\"t\":4},\"s\":true}"; // reply to participant join
const prog_char clientHandshakeLine15[] PROGMEM = "\"h\":{\"cid\":"; //prefix of the CID. Used for extraction
const prog_char clientHandshakeLine16[] PROGMEM = ",\"t\":"; // sufix of the CID. Used for extraction
const prog_char clientHandshakeLine17[] PROGMEM = "\"}}"; // used as the suffix to find the newserver to connect to
const prog_char clientHandshakeLine18[] PROGMEM = "{\"h\":{\"t\":2,\"cid\":\"";
const prog_char clientHandshakeLine19[] PROGMEM = "\",\"pid\":\"";
const prog_char clientHandshakeLine20[] PROGMEM = "\"},\"s\":true}";
const prog_char clientHandshakeLine21[] PROGMEM = "},\"a\":\"signal\",\"d\":";

PROGMEM const char *MuzzleyClientStringTable[] =
{
  stringVar,
  clientHandshakeLine1,
  clientHandshakeLine2,
  clientHandshakeLine3,
  clientHandshakeLine4,
  clientHandshakeLine5,
  clientHandshakeLine6,
  clientHandshakeLine7,
  clientHandshakeLine8,
  clientHandshakeLine9,
  clientHandshakeLine10,
  clientHandshakeLine11,
  clientHandshakeLine12,
  clientHandshakeLine13,
  clientHandshakeLine14,
  clientHandshakeLine15,
  clientHandshakeLine16,
  clientHandshakeLine17,
  clientHandshakeLine18,
  clientHandshakeLine19,
  clientHandshakeLine20,
  clientHandshakeLine21
};

boolean startRead = false;

// It will return for each object: Object d:XXX, Name: YYY, Value: ZZZ
// TODO: remove the String & convert it to a char array to optimize memory
void Muzzleyclient::parseObj(char* msgObj) {

  String jsonString(msgObj);
  // Parsing according to the json.org structures
  int c = 0; // Index counter while parsing
  int d = -1; // Dimension counter while parsing
  boolean registerValue = false;
  jsonString.trim();

  while( c < jsonString.length() ) {      // Loop until the end of the jSON string
    int n = 0; // used as aux for the int version of value conversion
    if( jsonString.charAt(c) == '{' ) {
      c++; // Increase index counter by one
      d++; // Increase dimension by one
      //Serial.print( "Object d:" );
      //Serial.println( d );
      //action[d].object = d;
      
      if( registerValue ) { registerValue = false; }
    }
    else if( jsonString.charAt(c) == '[' ) {
      c++; // Increase index counter by one
      d++; // Increase dimension by one
      //Serial.print( "Array d:" );
      //Serial.println( d );
      //action[d].object = d;
      
      if( registerValue ) { registerValue = false; }
    }
    else if( jsonString.charAt(c) == ':' ) {
      c++; // Increase index counter by one
      //Serial.println( "Read value" );
      registerValue = true;
    }
    else if( jsonString.charAt(c) == '"' ) {
      c++; // Increase index counter by one
      int stringEnd = jsonString.indexOf('"', c );
      String parsedString = jsonString.substring( c, stringEnd );
      c = stringEnd + 1;
      
      if( registerValue ) {
        registerValue = false;
        // Compose on the actions object the Value of the Key
        for (int i=0; i<parsedString.length(); i++){ 
          actionobj[d].value[actionobj[d].namecounter][i] = parsedString[i];
        }
      }
      else {
        // djsb
         //Serial.print( "Name: " );
         //Serial.println( parsedString );
        for (int i=0; i<parsedString.length(); i++){ 
          actionobj[d].name[actionobj[d].namecounter][i] = parsedString[i];
        }
          actionobj[d].namecounter++;
        //parsedString.toCharArray(actionobj[d].name, parsedString.length()); 
      }
    }
    else if( jsonString.charAt(c) > 47 && jsonString.charAt(c) < 58 ) {
      // djsb
      //Serial.print( "Number: " );
      //Serial.println( jsonString.charAt(c) );
      //action[d].value[n] = jsonString.charAt(c);
      n++;

      
      if( registerValue ) { registerValue = false; }
      c++;
    }
    else if( jsonString.indexOf( "true", c ) == c ) {
      //Serial.println( "Boolean TRUE" );
      c = c + 4;
      
      if( registerValue ) { registerValue = false; }
    }
    else if( jsonString.indexOf( "false", c ) == c ) {
      //Serial.println( "Boolean FALSE" );
      c = c + 5;
      
      if( registerValue ) { registerValue = false; }
    }
    else if( jsonString.indexOf( "null", c ) == c ) {
      //Serial.println( "NULL" );
      c = c + 4;
      
      if( registerValue ) { registerValue = false; }
    }
    else if( jsonString.charAt(c) == ',' ) {
      c++; // Increase index counter by one
      
      if( registerValue ) { registerValue = false; }
    }
    else if( jsonString.charAt(c) == '}' ) {
      c++; // Increase index counter by one
      d--; // Decrease dimension by one
    }
    else if( jsonString.charAt(c) == ']' ) {
      c++; // Increase index counter by one
      d--; // Decrease dimension by one
    }
    else {
      // Ignore whitespacing, tabs, line breaks, unsupported chars. Experimental function, may break correct parsing.
      c++; // Increase index counter by one
    }
  }
  // If reached this part, the json was successfully parsed
}


int Muzzleyclient::getParticipantID(char* rxdata){
  int startpos = findpos (rxdata, 12) + 1;
  int endpos = findpos (rxdata, 13) - 14;

  int i=endpos-startpos;
  char buffer[i];

  int xx=0;
  for (int x=startpos; x<endpos; x++){
    buffer[xx]=rxdata[x];
    xx++;
  }

  return atoi(buffer);
}

// extract the CID from a muzzley protocol message.
// rxdata is the message to extract from
// version is the message type. Actually the cid might be encoded behind a quote mark or not!
int Muzzleyclient::getCID(char* rxdata, int version){
  int startpos = 0;
  int endpos = 0;
  if (version == 0){
    startpos = findpos (rxdata, 15) + 1;
    endpos = findpos (rxdata, 16) - 4;
  }
  if (version == 1){
    startpos = findpos (rxdata, 15) + 2;
    endpos = findpos (rxdata, 16) - 5;
  }


  int i=endpos-startpos;
  char buffer[i];

  int xx=0;
  for (int x=startpos; x<endpos; x++){
    buffer[xx]=rxdata[x];
    xx++;
  }

  return atoi(buffer);
}


// rpl, replaces a keyword (strvar) in a specified string stores in progmem (index) by the new string (addstr)
// index = string to use as base
// strvar = the search string to replace
// addstr = the string to add
// finalStrSize = reference in order to "return" the result array length
char* Muzzleyclient::rpl(int index, int strvar, char* addstr) {

  int origstrsize = getStrigTableItemSize(index);
  char orig[origstrsize];
  for (int i=0; i<origstrsize; i++){
    orig[i] = getStringTableItem(index,0)[i];
  }


  int strvarsize = getStrigTableItemSize(strvar);

  char* newstr;
  newstr = addstr;
  int newstrsize = strlen(newstr);


  // calculate the final String to return to add 
  int finalStrSize = getStrigTableItemSize(index) - getStrigTableItemSize(strvar) + strlen(addstr);

  char finalStr[finalStrSize]; // where the final replaced string will be stored
  memset(finalStr,NULL,finalStrSize);

  int lastp = 0;

  int mark2 = findpos(orig,strvar); // the end of the placeholder string

  int mark1 = mark2 - strvarsize; // the first char of the placeholder string

  for (int i=0; i<=mark1; i++){ // compose the first part of the string
    finalStr[lastp] = orig[i];
    lastp++;
  }

  for (int i=0; i<newstrsize; i++){ // append the addstr to the string
    finalStr[lastp] = newstr[i];
    lastp++;
  }

  for (int i=mark2+1; i<origstrsize; i++){
    finalStr[lastp] = orig[i];
    lastp++;
  }

  finalStr[lastp] = NULL;

  // copying the existing string to a new one so it could be returned
  char returnStr[finalStrSize];
  memcpy(returnStr, finalStr, finalStrSize);
  returnStr[finalStrSize]=NULL;

  return (char*)returnStr;
}




int Muzzleyclient::getStrigTableItemSize(int index) {

  PGM_P prog_ptr = (prog_char*)pgm_read_word(&MuzzleyClientStringTable[index]); 
  int i=0;
  while (((char)pgm_read_byte(&prog_ptr[i])) != NULL){
    i++;
  }

  return i;
}

char* Muzzleyclient::getStringTableItem(int index, int send) {
  PGM_P prog_ptr = (prog_char*)pgm_read_word(&MuzzleyClientStringTable[index]); 
  int i=getStrigTableItemSize(index);
  char buffer[i+1];

  for (int x=0; x<i+1; x++){
    buffer[x]=(char)pgm_read_byte(&prog_ptr[x]);
  }

  if (send)
    websocket.sendData(buffer);
  return (char*)buffer;
}



// find a defined string stored on PROG_MEM with 'index'
// on a passed char array 'text'
int Muzzleyclient::findpos(char* text, int index){


  int pos_search = 0;
  int pos_text = 0;
  int len_search = getStrigTableItemSize(index); 
  int len_text = strlen(text);



  for (pos_text = 0; pos_text < len_text;++pos_text)
  {
    if(text[pos_text] == getStringTableItem(index,0)[pos_search])
    {

        //Serial.print (text[pos_text]); Serial.print (F(" = ")); Serial.println (getStringTableItem(index,0)[pos_search]);

      ++pos_search;
      if(pos_search == len_search)
      {
          // match
          //Serial.print(F("match: ")); Serial.println(pos_text);
        return pos_text;
      }
    }
    else
    {
     pos_search = 0;
   }
 }
    // no match
   //Serial.println(F("no match"));
 return -1;
}

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 3


void Muzzleyclient::begin(Client &client){
  socket_client = &client;
  firstconnect = true;
  reconnect = false;
  cid = 1;
  //String token = "bb7aa55a4d56f1b5";
  //String activity = "7145d4";
  ready = false;
}


void Muzzleyclient::disconnect(Client &client){

  socket_client = &client;
  while (socket_client->connected()){
    socket_client->stop();
  }
  
}

void Muzzleyclient::connect(Client &client){
  //while (!reconnect){
    power(client, newserver);
    if (reconnect){
      reconnect=false;
    }
  //}

}

void Muzzleyclient::addAction(Action *socketAction) {
  if (socket_actions_population <= SIZE(socket_actions)) {
    socket_actions[socket_actions_population++].socketAction = socketAction;
  }
}

void Muzzleyclient::executeActions(char* socketString) {
  for (int i = 0; i < socket_actions_population; ++i) {
    socket_actions[i].socketAction(*this, socketString);
  }
}

void Muzzleyclient::power(Client &client, char* server){

  reconnect=false;
  socket_client = &client;

  char* srv;
  srv = server;
  
  // if websocket connection already exists, disconnect
  if (socket_client->connected()){
    disconnect(client);
  }
  
  // Connect to the websocket server
  if (socket_client->connect(srv, 80)) {
    //Serial.println(F("Connected"));
  } else {
    //Serial.println(F("C f2."));
  }

  // Handshake with the server
  websocket.path = "/ws";
  websocket.host = srv;
  

  if (websocket.handshake(client)) {
    //updating state machine
    firstconnect=true;
    while (!reconnect){
      statemachine(client);
    }
  } else {
    disconnect(client);
  }

}

// parse the incomming new server and set instructions to reconnect
void Muzzleyclient::changeServer(char* rxdata){

  int endpos = findpos(rxdata, 17);
  int position = findpos(rxdata, 3);

  int z = 0;
  for (int i=position+1; i<endpos-2; i++){
    newserver[z] = rxdata[i];
    z++;
  }

}

void Muzzleyclient::statemachine(Client &client) {

  // Monitor what's happening in Memory inside the userspace.
  // The user memory should not exceed 600 bytes :(
  // Need to improve memory allocation
  //Serial.print(F("fmem: "));Serial.println(freeMemory());

  socket_client = &client;
  boolean msgok=false;

  while (socket_client->connected()) {

    // starting message after connecting
    if (firstconnect==true) {
      getStringTableItem(9,1);
      firstconnect = false;
    }


    char* crxdata;
    crxdata = websocket.getData();

    if (strlen(crxdata) > 0) {

      // DEBUG what's coming from the pipe ;-)
      /*
      Serial.print (F("RX: "));
      for (int i=0; i<strlen(crxdata); i++){
        Serial.print(crxdata[i]);
      }

      Serial.println(F(""));
      */

// *****************************************************
// * START STATE MACHINE FOR INCOMING DATA
// *****************************************************
            
      // RX: Handshake validated
      if ((findpos(crxdata, 1) != -1) && (firstconnect == false)){
        getStringTableItem(7,1);
        firstconnect=false;
        msgok=true;
        reconnect=false;
      }

    // RX: sessionId
      if (findpos(crxdata, 2) != -1){
        getStringTableItem(8,1);
        msgok=true;
        firstconnect=false;
      }

    // RX: Activity moved
      if (findpos(crxdata, 4) != -1){
        changeServer(crxdata);
        disconnect(client);
        msgok=true;
        reconnect=true;
      }

    // RX: activityID
      if (findpos(crxdata, 5) != -1){
        msgok=true;
        reconnect=false;
      }    


    // RX: on participant join
      if (findpos(crxdata, 6) != -1){
       reconnect=false;
        

        // get the CID from the rxdata message
        int CID = getCID(crxdata,0);
        int CID_digits = floor(log10(abs(CID))) + 1; // calculate CID digit number in order to create a dynamic array to hold the value
        char tmp[CID_digits];
        itoa(CID,tmp,10); // convert to char in order to append to existing char reply message

        char* var;
        var = rpl(14, 0, tmp);

        websocket.sendData(var);

        //executeActions(crxdata);
        msgok=true;
      }   


    // RX: signal ready
      if (findpos(crxdata, 11) != -1){
        ready=true;


        // get the Client ID
        int CID = getCID(crxdata, 1);
        int CID_digits = floor(log10(abs(CID))) + 1; // calculate CID digit number in order to create a dynamic array to hold the value
        char tmp[CID_digits+1];
        itoa(CID,tmp,10); // convert to char in order to append to existing char reply message

        // get the participant ID
        int pID = getParticipantID(crxdata);
        int pID_digits = floor(log10(abs(pID))) + 1; // calculate CID digit number in order to create a dynamic array to hold the value
        char tmppID[pID_digits+1];
        itoa(pID,tmppID,10); // convert to char in order to append to existing char reply message

         
        // replace the strings {0} and {1}
        int arrsize = getStrigTableItemSize(18) + CID_digits + getStrigTableItemSize(19) + pID_digits + getStrigTableItemSize(20);
        char tx[arrsize];
        memset(tx, NULL, arrsize); 

        int lastp = 0;

        // compose the reply message
        for (int i=0; i<getStrigTableItemSize(18); i++){
          tx[lastp] = getStringTableItem(18,0)[i];
          lastp++;
        }
        strcat (tx, tmp); // add the CID
        strcat (tx, getStringTableItem(19,0)); // compose message
        strcat (tx, tmppID); // add the Participant ID
        strcat (tx, getStringTableItem(20,0)); // compose message

        websocket.sendData(tx);
        //executeActions(tx);

        websocket.sendData("{\"h\":{\"cid\":7,\"t\":1},\"a\":\"signal\",\"d\":{\"a\":\"changeWidget\",\"d\":{\"widget\":\"webview\",\"params\":{\"uuid\":\"989d0ae8-c5c2-4a20-9b7c-a1e25ea540f8\"}}}}");

        msgok=true;
      }   


   // callback the userspace with a signal
    if ((findpos(crxdata, 21) != -1) && (findpos(crxdata, 11) == -1)){
      int startpos = findpos(crxdata, 21) + 1;
      int endpos = strlen(crxdata) - 1;
      char signal[endpos-startpos+1];

      int a = 0;
      for (int i=startpos; i<endpos; i++){
        signal[a] = crxdata[i];
        a++;
      }

      executeActions(signal);
      msgok=true;
    }
    

      if (!msgok){
        msgok=true;
      }   

      // clean the crxdata for next iteration
      memset(crxdata, NULL, sizeof(crxdata));  

      
    }









// *****************************************************
// * END STATE MACHINE
// *****************************************************
    
  } 


}


