 #include <iostream>
#include <string>
#include <curl/curl.h>
#include <sys/stat.h>

int main() {
    CURL *curl;
    CURLcode res;

    std::string ftp_url = "ftp://willy:willy222@127.0.0.1:21/";
    std::string local_file = "log.txt";
    std::string remote_file = ".";

    ftp_url += remote_file + "/" + local_file;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, ftp_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, "willy");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "willy222");
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

        FILE *file = fopen(local_file.c_str(), "rb");
        if(file) {
            struct stat file_stat;
            stat(local_file.c_str(), &file_stat);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_stat.st_size);

            curl_easy_setopt(curl, CURLOPT_READDATA, file);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, fread);

            res = curl_easy_perform(curl);
            if(res!= CURLE_OK) {
                std::cerr << "Erreur lors de l'envoi du fichier : " << curl_easy_strerror(res) << std::endl;
            } else {
                std::cout << "Fichier envoyé avec succès!" << std::endl;
            }

            fclose(file);
        } else {
            std::cerr << "Erreur lors de l'ouverture du fichier local." << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}