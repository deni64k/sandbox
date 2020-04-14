FROM fedora

RUN yum update -y && \
    yum upgrade -y && \
    yum groupinstall -y "Development Tools" "Development Libraries" && \
    yum install -y gcc-c++ cmake make perf man mg

VOLUME /src
