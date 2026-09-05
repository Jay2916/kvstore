#!/bin/bash
cd "$(dirname "$0")/.."
echo "docker buildx build -f docker/Dockerfile.server --platform linux/amd64,linux/arm64 -t jay2916/kvserver:latest --push ."

docker buildx build -f docker/Dockerfile.server --platform linux/amd64,linux/arm64 -t jay2916/kvserver:latest --push .
