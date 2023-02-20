build_linux_SHA256SUM = sha256sum
build_linux_DOWNLOAD = curl --location --fail --insecure --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o
