#/bin/bash -e

protoc --encode=tsdb.RecordEnvelopeList src/tsdb/RecordEnvelope.proto < test/InsertRequest.txt | \
    curl --data-binary @- -X POST -i localhost:8000/tsdb/insert
