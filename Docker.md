# Dockerizing your GS2Emu instances

## Requirements

* docker ([Docker Desktop for Mac](https://hub.docker.com/editions/community/docker-ce-desktop-mac), [Docker Desktop for Windows](https://hub.docker.com/editions/community/docker-ce-desktop-windows), [Docker for Linux](https://docs.docker.com/engine/install/#server))
* docker-compose (comes with docker-desktop, might be an extra install on Linux)

### On Windows

**When given the choice, choose to run Docker in WSL2**


## Glossary

The following you will need to replace in the next steps:
* `<your-server-directory-name>` example: `my-server`
* `<server-name>` example: `My awesome server`
* `<account-name>` this is just your GR account name
* `<lan-ip>` this is the IP your computer has on your local network. Example: `192.168.100.5`
* `<external-ip-or-hostname>` this is the IP provided by your internet provider. If you have a domain set up pointing to your external ip, you can put that here instead. Example `8.8.8.8` or `mydomainname.com` 


## Preparations

### 1. Create a main directory to confine your configuration.

### 2. Create a file in your main directory called `server.env`, then put this info in it:
```ini
USE_ENV=true
SERVER=<your-server-directory-name>
PORT=14900
LOCALIP=<lan-ip>
SERVERIP=<external-ip-or-hostname>
INTERFACE=0.0.0.0
SERVERNAME=<server-name>
STAFFACCOUNT=<account-name>
```  

### 3. Create a file in your main directory called `docker-compose.yml` and put following in it: 
```yml
version: "3"
 services:
   gserver:
     image: "xtjoeytx/gserver-v2:feature-npc-server"
     restart: always
     env_file: server.env
     volumes:
       - ./server:/gserver/servers/<your-server-directory-name>
     ports:
       - 14900:14900
```

### 4. Create a directory in called `server` in your main directory.

### 5. Your main directory should now have the following files
* **file**      `docker-compose.yml`
* **file**      `server.env`
* **directory** `server`

## Running 

Open up a terminal/command prompt and go to the path of your main directory, then write this in the terminal and press enter: `docker-compose up -d` 
This will run the gserver instance and will auto-restart the server on crash.
And since the `server` directory is mapped into the docker container, your files will be persistent.
