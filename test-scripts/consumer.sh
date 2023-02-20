#!/bin/bash

# Produce the test data.
docker run --name ppm_kafka_consumer -it --rm jpo-cvdp_ppm:latest /cvdi-stream/docker-test/consumer_dbg.sh
