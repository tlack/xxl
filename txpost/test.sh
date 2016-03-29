curl -vvvvv http://txpo.st/
dt=`date +%Y%m%d%H%M%S`
URL=`curl --verbose --data "hello world! $dt" http://txpo.st/t0`
echo $URL
curl -vvvvv $URL
curl --verbose --data "hax $dt" http://txpo.st/



