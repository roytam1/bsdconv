/*
 * Copyright (c) 2009 Kuan-Chung Chiu <buganini@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "../../src/bsdconv.h"
#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

struct my_s{
	int status;
	int *tbl;
	int b;
	union {
		unsigned char c[4];
		uint32_t i;
	} buf;
};

void *cbcreate(void){
	return malloc(sizeof(struct my_s));
}

void cbinit(struct bsdconv_codec_t *cdc, struct my_s *r){
	r->status=0;
}

void cbclear(void *p){
	free(p);
}

#define CONTINUE() do{	\
	this_phase->state.status=CONTINUE;	\
	return;	\
}while(0);

#define DEADEND() do{	\
	this_phase->state.status=DEADEND;	\
	t->status=0;	\
	return;	\
}while(0);

#define APPEND(n) do{	\
	this_phase->data_tail->next=malloc(sizeof(struct data_s));	\
	this_phase->data_tail=this_phase->data_tail->next;	\
	this_phase->data_tail->next=NULL;	\
	this_phase->data_tail->len=n;	\
	p=this_phase->data_tail->data=malloc(n);	\
	p[0]=0x01;	\
}while(0);

int dec[256]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int hex[256]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

void callback(struct bsdconv_instance *ins){
	unsigned char ob[8];
	int i,j=0;
	struct bsdconv_phase *this_phase=&ins->phase[0];
	struct my_s *t=this_phase->codec[this_phase->index].priv;
	unsigned char d=*ins->from_data, *p;
	if(d==';' && t->status){
		//put data
		t->buf.i=htonl(t->buf.i);
		for(i=0;i<4;i++){
			if(t->buf.c[i] || j)
				ob[j++]=t->buf.c[i];
		}
		this_phase->data_tail->next=malloc(sizeof(struct data_s));
		this_phase->data_tail=this_phase->data_tail->next;
		this_phase->data_tail->next=NULL;
		this_phase->data_tail->len=j+1;
		p=this_phase->data_tail->data=malloc(j+1);
		p[0]=0x01;
		memcpy(&p[1], ob, j);
		this_phase->state.status=NEXTPHASE;
		t->status=0;
		return;
	}
	if(t->status){
		++t->status;
		if(t->tbl[d]==-1) DEADEND();
		t->buf.i*=t->b;
		t->buf.i+=t->tbl[d];
	}else{
		if(d=='x'){
			t->status=1000;
			t->tbl=hex;
			t->b=16;
			t->buf.i=0;
			CONTINUE();
		}
		t->b=10;
		t->tbl=dec;
		if(t->tbl[d]==-1) DEADEND();
		t->buf.i=t->tbl[d];
		t->status=1;
	}
	CONTINUE();
}
