#include "client.h"


int main() {
    // Initialisation de la connexion au serveur
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket != -1) {
        writeLog("INFO: successfully created socket");
    } else {
        writeLog("ERROR: error when creating socket");
        perror("error when creating socket");
        return -1;
    }

    // Spécifier l'adresse du serveur et se connecter à celui-ci
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10000);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.32.174");

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("error when connect to a server");
        writeLog("ERROR: error when connect to a server");
        close(clientSocket); // Fermer le descripteur de fichier du socket
        return -1;
    } else {
        writeLog("INFO: success connect to a server");
    }

    vector<File> files;
    ThreadParams* params = new ThreadParams;

    bool declare_found = false;
    cout <<"*********************************************"<<endl;
    cout <<"***************Share File App****************"<<endl;
    cout <<"*********************************************"<<endl;


    bool isFileListLoaded = false;

    while  (true) {


        int option;
        int file_number;
        bool file_found = false;
        string file_ip;
        string file_info;

        string directoryPath = "data";
        cout <<"*********************************************"<<endl;
        cout << "Menu:" << endl;
        cout << "1. declarer mes fichiers" << endl;
        cout << "2. voir les fichiers disponible" << endl;
        cout << "3. Telecharger" << endl;
        cout << "4. Quitter" << endl;

        cout << endl;
        cout << "Choisissez une option : ";
        writeLog("INFO: le client doit choisir une option:");
        cin >> option;



        // Validation de la saisie
        while (cin.fail()) {
            cin.clear(); // Clear the error flag
            cin.ignore(INT_MAX, '\n'); // Ignore any remaining input
            cout << "Entrée invalide. Veuillez entrer un nombre entier entre 1 et 4;"<<endl;
            cout << "Choisissez une option : ";
            cin >> option;
        }

        switch (option) {
            case 1:
                writeLog("option choisie: 1-declarer mes fichiers");
                // Afficher les fichiers au niveau du serveur
                // Envoyer les données initiales au socket
                files = getFileInfo(directoryPath);
                sendData(clientSocket, files);
                cout << endl;
                pthread_t declare_file;
                pthread_t send_file;
                if (!declare_found) {

                    if (pthread_create(&declare_file, nullptr, UnderstandingDirectory, &clientSocket) != 0) {
                        cerr << "Erreur lors de la création du thread pour ecouter les modifications dans le dossier." << endl;
                        writeLog("ERROR: error when creating a thread to understandind 'data' directory");

                    }else if (pthread_create(&send_file, nullptr, sendFile, &clientSocket) != 0) {
                        cerr << "Erreur lors de la création du thread pour partager les fichiers" << endl;
                        writeLog("ERROR: error when creating a thread to share file");

                    }else{

                        declare_found = true;
                    }
                }
                pthread_detach(declare_file);
                pthread_detach(send_file);


                
                break;
            case 2:
                // Envoyer les données sur le socket
                writeLog("option choisie: 2-voir les fichiers disponible");
                cout<< "voici la liste des fichiers disponible!"<<endl;

                // Fonction pour gérer la demande, la recuperation et l'affichage de la liste
                
                getlistfile(clientSocket);
                isFileListLoaded = true;
            

                break;
            case 3:
                writeLog("option choisie = 3: Telecharger un fichier");
                cout << endl;
                
                if(!isFileListLoaded){
                    cout << "veillez recharger la liste des fichiers d'abord avec l'option 2" <<endl;
                    break;
                } 

                cout << "Entrez le numéro du fichier à télécharger : ";
                cin >> file_number;


                // Validation de la saisie
                while (cin.fail()) {
                    cin.clear(); // Clear the error flag
                    cin.ignore(INT_MAX, '\n'); // Ignore any remaining input
                    cout << "Entrée invalide. Veuillez entrer un nombre entier correspondant u numro du fichier;"<<endl;
                    cout << "Entrez le numéro du fichier à télécharger : ";
                    cin >> file_number;
                }

                // Parcourir la liste des fichiers pour trouver celui correspondant au numéro
                fileList_mutex.lock();
                for (const ListFile& file : fileList) {
                    if (file.file_number == file_number) {
                        file_found = true;
                        file_info = file.file_info;
                        file_ip = file.ipOwner;
                        break;
                    }
                }

                // Vérifier si le fichier a été trouvé et afficher les informations
                if (file_found) {
                    cout << "Informations du fichier " << file_number << " : " << file_info <<endl;
                    cout<<"Adresse ip du proprietaire : "<<file_ip<< endl;
                } else {
                    cout << "Fichier non trouvé! veillez recharger la liste des fichiers, bien vérifier le numéro et réessayer" << endl;
                    cout<<""<<endl;
                    break;
                }

                fileList_mutex.unlock();
                cout << endl;

                params->ipOwner = file_ip;
                params->file_info = file_info;


                download_file(params);

                break;
            case 4:
                writeLog("option choisie: 5-quitter!");
                writeLog("fin d'éxécution; à bientôt!");
                cout<<"a bientot!"<<endl;
                close(clientSocket);
                return 0;
                break;
            default:
                writeLog("option choisie: option non disponible");
                cout << "Option invalide. Veuillez choisir une option valide." << endl;
                cout << endl;
                break;
        }
    }

    // Nettoyage
    close(clientSocket);

    return 0;
}
