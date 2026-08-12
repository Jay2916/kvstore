#!/bin/zsh
echo "docker buildx build -f Dockerfile.server --platform linux/amd64,linux/arm64 -t jay2916/kvserver:latest --push ."

docker buildx build -f Dockerfile.server --platform linux/amd64,linux/arm64 -t jay2916/kvserver:latest --push .