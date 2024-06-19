#ifndef PAGINA_WEB_HPP
#define PAGINA_WEB_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

void PaginaWeb(String &idioma1, String &idioma2);

/*
const char* ssid = "RedmiNuria";
const char* password = "Patata123";
*/
AsyncWebServer server(80);


String idioma1 = "";
String idioma2 = "";
String texto1 = "Texto de ejemplo 1";
String texto2 = "Texto de ejemplo 2";

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IDIOMA-BRIDGE</title>
    <style>
        body {
            background-color: #f0f0f0; /* Color de fondo principal */
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        #container {
            background-color: #ffffff; /* Color de fondo del contenedor */
            border-radius: 15px; /* Bordes redondeados del contenedor */
            box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1); /* Sombra suave */
            padding: 20px;
            width: 80%;
            max-width: 600px; /* Ancho máximo del contenedor */
            text-align: center;
        }
        h1 {
            color: #2e7d32; /* Color del título */
            font-size: 2.5em; /* Tamaño del título */
            margin-bottom: 20px;
        }
        .button-container {
            display: flex;
            justify-content: space-around;
            flex-wrap: wrap; /* Ajusta los botones en varias líneas si es necesario */
            margin-top: 20px;
        }
        .button {
            padding: 15px 30px; /* Tamaño de padding para hacer los botones más grandes */
            margin: 10px; /* Espacio entre botones */
            border-radius: 8px; /* Bordes redondeados de los botones */
            border: none;
            background-color: #4caf50; /* Color de fondo de los botones */
            color: white; /* Color del texto */
            font-size: 18px; /* Tamaño del texto */
            cursor: pointer;
            transition: background-color 0.3s ease; /* Transición suave del color de fondo */
        }
        .button:hover {
            background-color: #45a049; /* Color de fondo al pasar el mouse */
        }
    </style>
</head>
<body>
    <div id="container">
        <h1>IDIOMA-BRIDGE</h1>
        <div class="button-container">
            <button class="button" onclick="startRecording()">Hablar</button>
            <button class="button" onclick="speechToText()">Pasar a Texto</button>
            <button class="button" onclick="translateText()">Traducir</button>
            <button class="button" onclick="textToSpeech()">Pasar a Voz</button>
            <button class="button" onclick="playAudio()">Reproducir Audio</button>
        </div>
    </div>

    <script>
        function startRecording() {
            fetch('/grabar')
                .then(response => {
                    if (response.ok) {
                        console.log("Grabación completada");
                    } else {
                        console.error("Error al iniciar la grabación");
                    }
                });
        }

        function speechToText() {
            fetch('/transcribir')
                .then(response => response.text())
                .then(data => {
                    console.log("Transcripción completada: " + data);
                    // Aquí puedes manipular la transcripción como lo necesites
                });
        }

        function translateText() {
            fetch('/traducir')
                .then(response => response.text())
                .then(data => {
                    console.log("Traducción y síntesis completadas: " + data);
                    // Aquí puedes manipular la traducción como lo necesites
                });
        }

        function textToSpeech() {
            fetch('/textToSpeech')
                .then(response => {
                    if (response.ok) {
                        console.log("Texto convertido a voz");
                    } else {
                        console.error("Error al convertir texto a voz");
                    }
                });
        }

        function playAudio() {
            const audio = new Audio('/audio/output.mp3');
            audio.play();
        }
    </script>
</body>
</html>


)rawliteral";






/*
void loop() {
    // No hace falta nada aquí
}
*/

#endif // PAGINA_WEB_HPP
