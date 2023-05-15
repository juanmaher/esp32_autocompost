// Wifi Credentials
#define SSID "SSID"
#define PASSWORD "PASSWORD"

// Read readme.md to properly configure api key and authentication

// create a new api key and add it here 
#define API_KEY "API_KEY"
// Copy your firebase real time database link here 
#define DATABASE_URL "https://autocompost-5e61a-default-rtdb.firebaseio.com/"  

#define USER_EMAIL "juanma@gmail.com"   // This gmail does not exist outside your database. it only exists in the firebase project as a user
#define USER_PASSWORD "123456789"      // Dont add your gmail credentials. Setup users authentication in your Firebase project first

typedef struct {
    // Campos y datos adicionales del evento
    int data;
    char message[50];
} firebase_event_data_t;
