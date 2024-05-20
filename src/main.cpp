
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"


// String captureAudio();
String transcribeSpeech(String audioData, const char* apiKey);

String readAndTranscribeVoiceFile(const char* apiKey, const char* filePath);

String getLanguageCode(const char* languageName);
String translateText(String text, const char* apiKey, const char* targetLanguage);
// void speakText(String text, const char* apiKey, const char* targetLanguage);


const char* ssid = "RedmiNuria";
const char* password = "Patata123";
const char* apiKey = "AIzaSyCz4Pb-7OIi3Gs6LGgJ-XHZ2Xy__hRAeZQ";


// Definir el pin del micrófono analógico
const int pinMicrophone = A0;

// Función para conectar a WiFi
void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Conectado a la red WiFi");
}

// Función para inicializar
/*
void setup() {
    Serial.begin(115200);
    Serial.println("Inicio");
    connectToWiFi();
    pinMode(pinMicrophone, INPUT);
}
*/


// setup() de ejemplo: 
void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    Serial.println("Conectado a WiFi");

    // Inicializar el sistema de archivos SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar el sistema de archivos SPIFFS");
        return;
    }

    // Transcribir un archivo de voz llamado "audio.wav" ubicado en la raíz de SPIFFS
    // String apiKey = "TU_API_KEY";
    //String transcription = readAndTranscribeVoiceFile(apiKey.c_str(), "/proba.opus");
    String transcription = readAndTranscribeVoiceFile(apiKey, "/proba.opus");

    Serial.println("Transcripción del archivo de voz:");
    Serial.println(transcription);

    // Obtener el código de idioma para "Spanish"
    String languageName = "Spanish";
    String languageCode = getLanguageCode(languageName.c_str());

    if (languageCode != "") {
        Serial.println("Código del idioma " + languageName + ": " + languageCode);

        // Traducir texto usando el código de idioma obtenido
        String textToTranslate = "Hello, how are you?";
        String translatedText = translateText(textToTranslate, apiKey, languageCode.c_str());
        Serial.println("Texto traducido: " + translatedText);
        //speakText(translatedText, apiKey, languageCode.c_str());
    }
    else {
        Serial.println("No se encontró el código para el idioma " + languageName);
    }
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

// Funcion de prueba para leer un fichero de voz
String readAndTranscribeVoiceFile(const char* apiKey, const char* filePath) {
    // Abrir el archivo de voz
    File file = SPIFFS.open(filePath, "r");
    if (!file) {
        Serial.println("Error al abrir el archivo");
        return "";
    }

    // Leer el contenido del archivo
    String audioData;
    while (file.available()) {
        audioData += (char)file.read();
    }

    // Cerrar el archivo
    file.close();

    // Llamar a la función transcribeSpeech con los datos de audio
    return transcribeSpeech(apiKey, audioData.c_str());
}



// Función para enviar audio a Google Cloud Speech-to-Text
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

        // Analizar la respuesta JSON para obtener la transcripción
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
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
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    // Liberar los recursos
    http.end();

    return transcribedText;
}


// Función para obtener el código de idioma a partir del nombre del idioma
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


/*PARA LA DETECCIÓN AUTOMARICA DEL IDIOMA
String translateText(String text, const char* apiKey, const char* targetLanguage) {
    HTTPClient http;

    String url = "https://translation.googleapis.com/language/translate/v2?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"q\":\"";
    jsonBody += text;
    jsonBody += "\",\"source\":\"en\",\"target\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\",\"format\":\"text\"}";

    int httpResponseCode = http.POST(jsonBody);

    String translatedText = "";

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();

        // Parsear la respuesta JSON para obtener la traducción
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);

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

void loop() {
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

