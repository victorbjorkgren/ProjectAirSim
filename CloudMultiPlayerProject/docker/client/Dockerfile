# Base image: ROS2 Humble (which includes Python 3)
FROM ros:humble-ros-base-jammy

# Install system dependencies
# netcat for checking if server is up
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3-pip \
    python3-dev \
    netcat \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the client python code
# We copy the entire client/python directory to preserve structure
# This includes 'projectairsim' package and 'example_user_scripts'
COPY client/python /app/client/python

# Install the python client
# The setup.py is located in client/python/projectairsim
RUN pip3 install --no-cache-dir /app/client/python/projectairsim

# Install any additional requirements if they exist
RUN if [ -f "/app/client/python/projectairsim/requirements.txt" ]; then pip3 install -r /app/client/python/projectairsim/requirements.txt; fi

# Install common python libs for scripts (numpy is usually in projectairsim deps but good to ensure)
RUN pip3 install --no-cache-dir numpy msgpack-rpc-python opencv-python-headless

# Set environment variables
ENV PYTHONUNBUFFERED=1

# Default command (can be overridden in docker-compose)
# Keeps container running
CMD ["tail", "-f", "/dev/null"]

