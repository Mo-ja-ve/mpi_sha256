verify:coin_verifier.cc
	g++ -o verify  coin_verifier.cc -I/usr/include/openssl -L/usr/lib/openssl -lcrypto
miner:cs_miner.cc
	g++ -o miner cs_miner.cc -I/usr/include/openssl -L/usr/lib/openssl -lcrypto -lpthread
