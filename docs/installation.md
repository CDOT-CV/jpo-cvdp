# Installation and Setup

The following instructions represent the "hard" way to install. A docker image can be built to make
this easier: [Using the Docker Container](#using-the-docker-container).

*The directions that follow were developed for a clean installation of Ubuntu 18.04.* 

## 0. Create a base directory from which to install all the necessary components to run & test the PPM.

```bash
$ export BASE_PPM_DIR=~/some/dir/you/want/to/put/this/stuff
```

## 1. Install [Git](https://git-scm.com/)

```bash
$ sudo apt install git
```

## 2. Install [CMake](https://cmake.org) which is used to build the PPM.

```bash
$ sudo apt install cmake
```

## 3. Install [`librdkafka`](https://github.com/edenhill/librdkafka) which the PPM depends on.

```bash
$ sudo apt-get install -y libsasl2-dev libsasl2-modules libssl-dev librdkafka-dev
```

## 4. Download the Privacy Protection Module (PPM)

```bash
$ cd $BASE_PPM_DIR
$ git clone https://github.com/usdot-jpo-ode/jpo-cvdp.git
$ cd jpo-cvdp
```

## 5. If necessary, switch to the `develop` branch

```bash
$ git checkout develop
```

## 6. Build the PPM

```bash
$ mkdir build && cd build
$ cmake ..
$ make
```

## Additional information

- The PPM uses [RapidJSON](https://github.com/miloyip/rapidjson), but it is a header-only library included in the repository.
- The PPM uses [spdlog](https://github.com/gabime/spdlog) for logging; it is a header-only library and the headers are included in the repository.
- The PPM uses [Catch](https://github.com/philsquared/Catch) for unit testing, but it is a header-only library included in the repository.

# Additional Installation Instructions To Make Testing Easier
## 0. If you haven't already, create a base directory from which to install all the necessary components to run & test the PPM.

```bash
$ export BASE_PPM_DIR=~/some/dir/you/want/to/put/this/stuff
```

## 1. Install [Docker](https://www.docker.com)

- When following the website instructions, setup the Docker repos and follow the Linux post-install instructions.
- The CE version seems to work fine.
- [Docker installation instructions](https://docs.docker.com/engine/installation/linux/ubuntu/#install-using-the-repository)

### ORNL Specific Instructions
- *ORNL specific, but may apply to others with organizational security*
    - Correct for internal Google DNS blocking
#### 1.  As root (`$ sudo su`), create a `daemon.json` file in the `/etc/docker` directory that contains the following information:
```
{
    "debug": true,
    "default-runtime": "runc",
    "dns": ["160.91.126.23","160.91.126.28”],
    "icc": true,
    "insecure-registries": [],
    "ip": "0.0.0.0",
    "log-driver": "json-file",
    "log-level": "info",
    "max-concurrent-downloads": 3,
    "max-concurrent-uploads": 5,
    "oom-score-adjust": -500
}
```
- NOTE: The DNS IP addresses are ORNL specific.

#### 2. Restart the docker daemon to consume the new configuration file.

```bash
$ service docker stop
$ service docker start
```

#### 3. Check the configuration using the command below to confirm the updates above are taken if needed:

```bash
$ docker info
```

## 2. Install Docker Compose
- Comprehensive instructions can be found on this [website](https://www.digitalocean.com/community/tutorials/how-to-install-docker-compose-on-ubuntu-16-04)
- Follow steps 1 and 2.

## 3. Install [`kafka-docker`](https://github.com/wurstmeister/kafka-docker) so kafka and zookeeper can run in a separate container.

- Get your host IP address. The address is usually listed under an ethernet adapter, e.g., `en<number>`.

```bash
$ ifconfig
$ export DOCKER_HOST_IP=<HOST IP>
```
- Get the kafka and zookeeper images.

```bash
$ cd $BASE_PPM_DIR
$ git clone https://github.com/wurstmeister/kafka-docker.git
$ cd kafka-docker
$ vim docker-compose.yml	                        // Set kafka: ports: to 9092:9092
```
- The `docker-compose.yml` file may need to be changed; the ports for kafka should be 9092:9092.
- Startup the kafka and zookeeper containers and make sure they are running.

```bash
$ docker-compose up --no-recreate -d
$ docker-compose ps
```
- **When you want to stop kafka and zookeeper, execute the following commands.**

```bash
$ cd $BASE_PPM_DIR/kafka-docker
$ docker-compose down
```

## 4. Download and install the Kafka **binary**.

-  The Kafka binary provides a producer and consumer tool that can act as surrogates for the ODE (among other items).
-  [Kafka Binary](https://kafka.apache.org/downloads)
-  [Kafka Quickstart](https://kafka.apache.org/quickstart) is a very useful reference.
-  Move and unpack the Kafka code as follows:

```bash
$ cd $BASE_PPM_DIR
$ wget http://apache.claz.org/kafka/0.10.2.1/kafka_2.12-0.10.2.1.tgz   // mirror and kafka version may change; check website.
$ tar -xzf kafka_2.12-0.10.2.1.tgz			               // the kafka version may be different.
$ mv kafka_2.12-0.10.2.1 kafka
```

# Integrating with the ODE

## Using the Docker Container

This will run the PPM module in separate container. First set the required environmental variables. You need to tell the PPM container where the Kafka Docker container is running with the `DOCKER_HOST_IP` variable. Also tell the PPM container where to find the [map file](configuration.md#map-file) and [PPM Configuration file](configuration.md) by setting the `DOCKER_SHARED_VOLUME`:

```bash
$ export DOCKER_HOST_IP=your.docker.host.ip
$ export DOCKER_SHARED_VOLUME=/your/shared/directory
```

Note that the map file and configuration file must be located in the `DOCKER_SHARED_VOLUME` root directory and named
`config.properties` and `road_file.csv` respectively. 

Add the following service to the end of the `docker-compose.yml` file in the `jpo-ode` installation directory.

```bash
  ppm:
    build: /path/to/jpo-cvdp/repo
    environment:
      DOCKER_HOST_IP: ${DOCKER_HOST_IP}
    volumes:
      - ${DOCKER_SHARED_VOLUME}:/ppm_data
```

Start the ODE containers as normal. Note that the topics for raw BSMs must be created ahead of time.


# CDOT Integration with K8s

## Overview
The Colorado Department of Transportation (CDOT) is deploying the various ODE services within a Kubernetes (K8s) environment. Details of this deployment can be found in the main ODE repository [documentation pages](https://github.com/usdot-jpo-ode/jpo-ode/docs). In general, each submodule image is built as a Docker image and then pushed to the CDOT registry. The images are pulled into containers running within the K8s environment, and additional containers are spun up as load requires.

## CDOT PPM Module Build
A couple additional files have been added to this project to facilitate the CDOT integration. These files are:
- cdot-scripts/build_cdot.sh
- docker-test/ppm_no_map.sh

### Shell Scripts
Two additional scripts have been added to facilitate the CDOT integration. The first, [`ppm_no_map.sh`](../docker-test/ppm_no_map.sh), is modeled after the existing [`ppm.sh`](../docker-test/ppm.sh) script and performs a similar function. This script is used to start the PPM module, but leaves out the hard-coded mapfile name in favor of the properties file configuration. The second script, [`build_cdot.sh`](../cdot-scripts/build_cdot.sh), is used to build the CDOT PPM Docker image, tag the image with a user provided tag, and push that image to a remote repository. This is a simple automation script used to help reduce complexity in the CDOT pipeline.