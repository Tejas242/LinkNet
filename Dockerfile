FROM ubuntu:22.04

# Install required dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libsodium-dev \
    libssl-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libgtest-dev \
    pkg-config \
    git \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Setup working directory
WORKDIR /app

# Copy source code
COPY . .

# Clean any existing build and build the application
RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Expose port
EXPOSE 8080

# Set entry point for running the application
ENTRYPOINT ["/app/build/bin/linknet"]

# Default command line arguments
CMD ["--port=8080"]
