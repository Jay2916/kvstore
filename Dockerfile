FROM ubuntu:22.04

# Install compiler
RUN apt-get update && \
    apt-get install -y g++ && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

# Compile the server
RUN g++ server.cpp hashtable.cpp -o kvserver

EXPOSE 4444

# Run the server
CMD ["./kvserver"]