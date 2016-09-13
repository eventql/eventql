FROM  ubuntu:latest

RUN   apt-get update      &&      \
      apt-get -y install  curl    \
                          git     \
                          cmake   \
                          make    \
                          automake\
                          autoconf\
                          python  \
                          g++     \
                          zlib1g-dev


RUN   curl http://dl.eventql.io/eventql/v0.3.1/eventql-0.3.1-linux_x86_64.tgz | tar xvz

RUN  mkdir -p /var/evql

RUN   groupadd -r eventql && useradd -r -g eventql eventql

EXPOSE 9175
USER  eventql
ENTRYPOINT ["/usr/local/bin/evqld",  "--datadir", "/var/evql"]
CMD ["--standalone"]
