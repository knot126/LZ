echo "==> picolz <=="
picolz ./sample.txt ./sample.txt.lz
echo "==> lz4 <=="
lz4 --best ./sample.txt ./sample.txt.lz4
echo "==> gzip <=="
gzip -vc ./sample.txt > ./sample.txt.gz
