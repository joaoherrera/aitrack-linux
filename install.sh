set -x

# Install dependencies
sudo apt install -y qtbase5-dev \
    qtbase5-dev-tools \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    libqt5x11extras5-dev \
    libopencv-dev \
    libspdlog-dev \
    libfmt-dev \
    libomp-12-dev \
    libqt5x11extras5 \
    libspdlog1 \
    libomp5-12 \
    libxsettings-dev \
    libxsettings-client-dev

# Download onnxruntime
curl -L https://github.com/microsoft/onnxruntime/releases/download/v1.4.0/onnxruntime-linux-x64-1.4.0.tgz --output onnxruntime-linux-x64-1.4.0.tgz

# Extract the archive
gunzip onnxruntime-linux-x64-1.4.0.tgz && tar --extract -f onnxruntime-linux-x64-1.4.0.tar
rm onnxruntime-linux-x64-1.4.0.tar

# Generate the Makefile:
qmake -makefile

# Build the project:
make

# Create the directory where the models will be searched:
sudo mkdir /usr/share/aitrack && sudo mkdir /usr/share/aitrack/models

# Copy all the model files into the newly created directory:
sudo cp models/* /usr/share/aitrack/models/

# Copy the shared object file libonnxruntime.so.1.4.0 into the /usr/lib/ directory:
sudo cp onnxruntime-linux-x64-1.4.0/lib/libonnxruntime.so.1.4.0 /usr/lib/