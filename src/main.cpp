#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include <SD.h>
#include <Audio.h>
#include <SPI.h>
#include <Base64.h>


// FUNCIONS
// uint8_t* readAudioFromSD(const char* filename, size_t* audioSize);
bool obtenerAudioDesdeSD(char* buffer, size_t bufferSize);
String readAudioFileAndConvertToBase64(const char* filename);
// String captureAudio();
String transcribeSpeech(String audioData, const char* apiKey);
// String getLanguageCode(const char* languageName);
// String translateText(String text, const char* apiKey, const char* targetLanguage);
// void speakText(String text, const char* apiKey, const char* targetLanguage);


// VARIABLES / CONTRASENYES
const char* ssid = "RedmiNuria";
const char* password = "Patata123";
const char* apiKey = "AIzaSyCz4Pb-7OIi3Gs6LGgJ-XHZ2Xy__hRAeZQ";
const char* filename = "/audio.WAV";

// Función para conectar a WiFi
void connectToWiFi() {
    // Configurar DNS manualmente
    // IPAddress dns1(8, 8, 8, 8); // DNS de Google
    // IPAddress dns2(8, 8, 4, 4); // DNS de Google

    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2);
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Conectado a la red WiFi");
}

// setup() de ejemplo: 
void setup() {
    // Inicializar la comunicación serie
    Serial.begin(115200);
    // Comienza la conexión WiFi
    Serial.println();
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(ssid);

    // Inicializa la conexión WiFi
    WiFi.begin(ssid, password);

    // Espera hasta que la conexión se realice
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Una vez conectado, imprime la dirección IP asignada
    Serial.println("");
    Serial.println("WiFi conectado.");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
    
    // Esperar a que se inicialice Serial
    while (!Serial) {}

    const size_t bufferSize = 1024; // Define un tamaño de buffer adecuado
    char audioBuffer[bufferSize];

    if (obtenerAudioDesdeSD(audioBuffer, bufferSize)) {
        Serial.println("Archivo de audio leído correctamente");
        String audioBase64 = readAudioFileAndConvertToBase64(filename);
        // Convertir el audio a texto
        String transcribedText = transcribeSpeech(audioBase64, apiKey);
        // Imprimir el texto transcrito
        Serial.println("Texto transcrito:");
        Serial.println(transcribedText);
            

        // Aquí puedes procesar el buffer de audio
    } else {
        Serial.println("Error al leer el archivo de audio");
    }
    /*
    // Traducir el texto
    String TL = getLanguageCode("English"); // Código de idioma de destino (ejemplo: "es" para español)
    // Convertir TL a un array de caracteres (char[])
    char TLCharArray[TL.length() + 1];
    TL.toCharArray(TLCharArray, TL.length() + 1);

    // Traducir el texto
    String translatedText = translateText(transcribedText, apiKey, TLCharArray);
    // Imprimir
    */
}

const int chipSelect = 39; // Cambia esto según el pin que uses

bool obtenerAudioDesdeSD(char* buffer, size_t bufferSize) {
  // Inicializar la tarjeta SD
  SPI.begin(36, 37, 35); //  void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  if (!SD.begin(chipSelect)) {
    Serial.println("Error al montar la tarjeta SD");
    return false;
  }

  // Abrir el archivo de audio
  File archivo = SD.open("/audio.WAV", FILE_READ);
  if (!archivo) {
    Serial.println("Error al abrir el archivo de audio");
    return false;
  }

  // Leer el contenido del archivo
  size_t bytesRead = 0;
  while (archivo.available() && bytesRead < bufferSize) {
    buffer[bytesRead++] = archivo.read();
  }

  // Cerrar el archivo
  archivo.close();

  return true;
}

String readAudioFileAndConvertToBase64(const char* filename) {
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Error al abrir el archivo para leer");
    return "";
  }

  size_t size = file.size();
  Serial.println(size);
  uint8_t* buffer = (uint8_t*)malloc(size);
  file.read(buffer, size);
  file.close();

  String base64Audio = base64::encode(buffer, size);
  free(buffer);
  return base64Audio;
}


// Función para capturar audio 
/*
String captureAudio() {
    const int bufferSize = 1024;
    String audioData = "";

    for (int i = 0; i < bufferSize; i++) {
        // Leer valor del pin analógico
        int audioValue = analogRead(pinMicrophone);
        // Convertir el valor a un rango de 0 a 255
        byte audioByte = map(audioValue, 0, 1023, 0, 255);
        // Convertir el byte a una cadena hexadecimal
        String hexValue = String(audioByte, HEX);
        // Añadir el valor hexadecimal a los datos de audio
        audioData += hexValue;
        // Añadir un separador
        audioData += " ";
    }

    return audioData;
}
*/


// Función para enviar audio a Google Cloud Speech-to-Text

// Función para transcribir el audio utilizando Google Cloud Speech-to-Text
String transcribeSpeech(String audioData, const char* apiKey) {
    HTTPClient http;

    // URL de la API de Google Cloud Speech-to-Text
    String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
    url += apiKey;

    // Configurar la solicitud HTTP
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Crear el cuerpo de la solicitud JSON
    String jsonBody = "{\"config\": {\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,\"languageCode\":\"es-ES\"},\"audio\": {\"content\":\"";
    jsonBody += audioData;
    jsonBody += "\"}}";

    // Enviar la solicitud POST con el cuerpo JSON
    int httpResponseCode = http.POST(jsonBody);

    String transcribedText = "";

    // Si la solicitud fue exitosa, obtener la transcripción
    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();
        /*
        Serial.println("Respuesta de la API:");
        Serial.println(response);
        */
        // Analizar la respuesta JSON para obtener la transcripción
        DynamicJsonDocument doc(4096); // Aumentado el tamaño del documento
        DeserializationError error = deserializeJson(doc, response);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
        } else {
            if (doc.containsKey("results")) {
                JsonObject result = doc["results"][0];
                if (result.containsKey("alternatives")) {
                    JsonObject alternative = result["alternatives"][0];
                    if (alternative.containsKey("transcript")) {
                        transcribedText = alternative["transcript"].as<String>();
                    }
                }
            } else {
                Serial.println("No se encontró el campo 'results' en la respuesta JSON");
            }
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    // Liberar los recursos
    http.end();

    return transcribedText;
}


// Función para obtener el código de idioma a partir del nombre del idioma
/*
String getLanguageCode(const char* languageName) {
    HTTPClient http;
    String url = "https://translation.googleapis.com/language/translate/v2/languages?key=";
    url += apiKey;
    url += "&target=en";  // Para obtener los nombres de idiomas en inglés

    http.begin(url);
    int httpResponseCode = http.GET();


    String languageCode = "";
    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();
        StaticJsonDocument<4096> doc;
        deserializeJson(doc, response);
        //DynamicJsonDocument doc(4096);
        //deserializeJson(doc, response);

        JsonArray languages = doc["data"]["languages"];
        for (JsonObject language : languages) {
            //if (String(language["name"]).equalsIgnoreCase(languageName))
            if (String(language["name"].as<const char*>()).equalsIgnoreCase(languageName)) {
                languageCode = language["language"].as<String>();
                break;
            }
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return languageCode;
}
*/
// TRADUCTOR
/*
String translateText(String text, const char* apiKey, const char* targetLanguage) {
    HTTPClient http;

    String url = "https://translation.googleapis.com/language/translate/v2?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"q\":\"";
    jsonBody += text;
    jsonBody += "\",\"target\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\",\"format\":\"text\"}";  // No especificar "source" para detección automática

    int httpResponseCode = http.POST(jsonBody);

    String translatedText = "";

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();

        // Parsear la respuesta JSON para obtener la traducción
                StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, response);
        //DynamicJsonDocument doc(1024);
        //DeserializationError error = deserializeJson(doc, response);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
        } else {
            translatedText = doc["data"]["translations"][0]["translatedText"].as<String>();
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    return translatedText;
}
*/

// Función para sintetizar texto a voz
/*
void speakText(String text, const char* apiKey, const char* targetLanguage) {
    HTTPClient http;

    String url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"input\":{\"text\":\"";
    jsonBody += text;
    jsonBody += "\"},\"voice\":{\"languageCode\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\"},\"audioConfig\":{\"audioEncoding\":\"MP3\"}}";

    int httpResponseCode = http.POST(jsonBody);

    if (httpResponseCode == HTTP_CODE_OK) {
        // Leer el audio generado
        String audio = http.getString();

        // Aquí puedes manejar el audio según tus necesidades
        // Por ejemplo, podrías guardarlo en un archivo, reproducirlo, etc.
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}
*/

/*
void speakText(String text, const char* apiKey, const char* targetLanguage) {
    HTTPClient http;
    String url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"input\":{\"text\":\"";
    jsonBody += text;
    jsonBody += "\"},\"voice\":{\"languageCode\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\"},\"audioConfig\":{\"audioEncoding\":\"MP3\"}}";

    int httpResponseCode = http.POST(jsonBody);

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();
        // Extraer y retornar el contenido de audio
        int audioStart = response.indexOf("\"audioContent\": \"") + 17;
        int audioEnd = response.indexOf("\"", audioStart);
        String audioContent = response.substring(audioStart, audioEnd);
        return audioContent;
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
        return "";
    }

    http.end();
}
*/

void loop() {
    /*
        if (aac->isRunning()) {
        if (!aac->loop()) {
            aac->stop();
        }
    } else {
        // Audio terminado
    }
    */
    /*
    // Capturar audio
    String audioData = captureAudio();

    // Transcribir discurso
    String transcribedText = transcribeSpeech(audioData, apiKey);

    // Imprimir la transcripción en el Monitor Serie
    Serial.println("Transcripción:");
    Serial.println(transcribedText);

    String translatedText = translateText(transcribedText, apiKey, "es"); // Traducir al español

    // Imprimir la traducción en el Monitor Serie
    Serial.println("Traducción:");
    Serial.println(translatedText);

    // Sintetizar el texto traducido a voz
    speakText(translatedText, apiKey, "es-ES"); // Sintetizar en español de España


    delay(5000); // Esperar un tiempo antes de la próxima transcripción
    */

}

