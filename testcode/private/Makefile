default: cert.pem

cert.pem: dsaparams
	openssl req -newkey dsa:dsaparams -x509 -nodes -keyout key.pem -out cert.pem

dsaparams:
	openssl dsaparam -out dsaparams 1024
