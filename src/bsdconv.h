enum bsdconv_status{
	CONTINUE,
	DEADEND,
	MATCH,	
};

struct data_s{
	int data;
	size_t len;
}

struct state_s{
	char status;
	int data;
	int sub[257];
};
