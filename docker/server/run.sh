docker build -t eventql .
docker run --name eventql -v `pwd`:/var/evql -p 9175:9175  eventql
