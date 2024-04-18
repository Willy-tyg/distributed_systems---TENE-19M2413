#backend et la base de données MySQL
FROM node:latest as backend

WORKDIR /app_4107/backend/

# Copier les fichiers dans le répertoire de travail
COPY /app_4107/backend/package*.json ./
COPY backend/ ./

#Dépendances pour le backend
RUN npm install

# Copier le fichier de base de données
COPY database.js .

# Port pour le backend
EXPOSE 8080

# Créer les tables dans la base de données et lancer le serveur backend
CMD ["sh", "-c", "npm install && node database.js && npm start"]

#______________________________________________________________

#Frontend

FROM node:latest as frontend

WORKDIR /app_4107/frontend

# Copier les fichiers du frontend dans le répertoire de travail
COPY /app_4107/frontend/package*.json ./
COPY frontend/ ./

#Dépendances pour le frontend
RUN npm install

#Port 3000 pour le frontend
EXPOSE 3000

# Exécuter le serveur frontend
CMD ["npm", "start"]
