# distributed_systems
Bienvenue dans ce projet d'applications client-serveur!

Ce projet implémente un serveur TCP simple qui écoute les connexions entrantes et permettra aux clients d'envoyer et de recevoir des fichiers.
Getting Started

Pour commencer, suivez ces étapes :

    Clonez le projet 
    deplacez vous dans le dossier serveur et construisez l'image en utilisant le Makefile fourni
    Exécutez le serveur en utilisant l'exécutable "serveur".

    faites de meme pour le client:     
    deplacez vous dans le dossier client et construisez l'image en utilisant le Makefile fourni
    Exécutez le client en utilisant l'exécutable "client".

Utilisation du serveur

Le serveur écoute les connexions entrantes sur le port 10000 par défaut et sur l'adresse ip 127.0.0.1. Pour changer le port, modifiez le champ server_addr.sin_port dans le fichier server.cpp et changer le egalement dan le fichier main.cpp du client. pour changer l'adresse modifier le champ serverAddress.sin_addr.s_addr sur le client(main.cpp) et le serveur(serveur.cpp)

Une fois que le client est connecté au serveur, il peut envoyer des messages en utilisant la commande send.




N'hésitez pas à nous contacter si vous avez des questions ou des commentaires sur le projet.
