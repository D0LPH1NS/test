#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <FastLED.h>
#include <TMRpcm.h>

// Définir le nombre de LEDs et le pin de données
#define N 30
#define D 6
CRGB l[N]; // Tableau pour les LEDs

TMRpcm audio; // Création de l'objet audio pour TMRpcm
byte m[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Adresse MAC
IPAddress ip(192, 168, 0, 177); // Adresse IP

// Définir les pins des boutons
const int b[] = {A0, A1, A4, A5, 8, 9}; 
EthernetServer s(80); // Démarrer un serveur Ethernet sur le port 80
bool g = false; // Variable pour indiquer si le jeu est actif
unsigned long t; // Variable pour stocker le temps de début
uint8_t sc[6] = {0}; // Tableau pour les scores, utilisation de uint8_t pour économiser de la mémoire
uint8_t rt = 60; // Temps restant
uint8_t gm = 1; // Mode de jeu

void setup() {
    // Initialisation des composants
    Ethernet.begin(m, ip);
    s.begin();
    SD.begin(4);
    FastLED.addLeds<WS2812B, D, GRB>(l, N);
    audio.speakerPin = 9;

    // Configuration des pins des boutons en entrée avec pull-up interne
    for (uint8_t i = 0; i < 6; i++) pinMode(b[i], INPUT_PULLUP);
}

void loop() {
    // Vérifier si un client se connecte au serveur
    EthernetClient c = s.available();
    if (c) hC(c);

    // Si le jeu est actif, mettre à jour le jeu et vérifier les boutons
    if (g) {
        uG();
        cB();
    }
}

// Fonction pour gérer les requêtes des clients
void hC(EthernetClient c) {
    String r = c.readStringUntil('\r'); // Lire la requête du client

    // Gérer les différentes requêtes
    if (r.indexOf("GET /mode1") != -1) {
        sG(1);
        rC(c, "/game");
    } 
    else if (r.indexOf("GET /mode2") != -1) {
        sG(2);
        rC(c, "/equipe");
    }
    else if (r.indexOf("GET /scores") != -1) {
        sS(c);
    }
    else if (r.indexOf("GET /equipe") != -1) {
        sF(c, "Equipe.htm");
    }
    else {
        sF(c, g ? "Game.htm" : "Accueil.htm");
    }
    c.stop(); // Fermer la connexion avec le client
}

// Fonction pour rediriger le client vers une autre page
void rC(EthernetClient c, const char* l) {
    c.println("HTTP/1.1 303 See Other");
    c.print("Location: ");
    c.println(l);
    c.println();
}

// Fonction pour démarrer le jeu
void sG(int m) {
    g = true;
    t = millis();
    rt = 60;
    gm = m;
    memset(sc, 0, sizeof(sc)); // Réinitialiser les scores

    audio.play("music.mp3"); // Jouer de la musique
}

// Fonction pour mettre à jour le jeu
void uG() {
    unsigned long e = (millis() - t) / 1000;
    if (e >= 60) eG(); // Si le temps écoulé est supérieur ou égal à 60 secondes, terminer le jeu
    else rt = 60 - e; // Mettre à jour le temps restant
}

// Fonction pour vérifier les boutons
void cB() {
    for (uint8_t i = 0; i < 6; i++) {
        if (digitalRead(b[i]) == LOW) sc[i]++; // Incrémenter le score si le bouton est pressé
    }
}

// Fonction pour envoyer un fichier au client
void sF(EthernetClient c, const char* f) {
    File fl = SD.open(f);
    if (fl) {
        c.println("HTTP/1.1 200 OK");
        c.println("Content-Type: text/html");
        c.println();
        while (fl.available()) c.write(fl.read());
        fl.close();
    } else {
        c.println("HTTP/1.1 404 Not Found");
        c.println("<h1>404 - Fichier non trouvé</h1>");
    }
}

// Fonction pour envoyer les scores au client
void sS(EthernetClient c) {
    c.println("HTTP/1.1 200 OK");
    c.println("Content-Type: text/plain");
    c.println();
    
    c.print("TIME:");
    c.println(rt);
    
    int t1 = sc[0] + sc[1] + sc[2];
    int t2 = sc[3] + sc[4] + sc[5];

    for (uint8_t i = 0; i < 6; i++) {
        c.print("J");
        c.print(i + 1);
        c.print(": ");
        c.println(sc[i]);
    }

    if (gm == 2) { // Mode équipe
        c.print("T1:");
        c.println(t1);
        c.print("T2:");
        c.println(t2);
    }
}

// Fonction pour terminer le jeu
void eG() {
    g = false;
    audio.stopPlayback();
    FastLED.clear();
    FastLED.show();
}