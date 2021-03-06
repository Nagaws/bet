#include "bet-cli.h"

struct enc_share *g_shares=NULL;

bits256 v_hash[CARDS777_MAXCARDS][CARDS777_MAXCARDS];
bits256 g_hash[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];

int32_t sharesflag[CARDS777_MAXCARDS][CARDS777_MAXPLAYERS];
bits256 playershares[CARDS777_MAXCARDS][CARDS777_MAXPLAYERS];
bits256 deckid;
struct deck_player_info player_info;
int32_t no_of_shares=0;

char* bet_strip(char *s) 
{
	    char *t = NULL;
	    int32_t l=0;
		t=calloc(strlen(s),sizeof(char));
		for(int i=0;i<strlen(s);i++)
		{
			if(((i+1)<strlen(s))&&(strncmp(s+i,"\\n",2)==0))
			{
				t[l++]=0x0A;
				i+=1;
			}
			else if(((i+1)<strlen(s))&&(strncmp(s+i,"\\t",2)==0))
			{
				t[l++]=0x20;
				i+=1;
			}
			else if(((i+1)<strlen(s))&&(strncmp(s+i,"\"",2)==0))
			{
				t[l++]=0x22;
			}
			else	
				t[l++]=s[i];
		}
		t[l]='\0';
		return t;
}

int main(int argc, char **argv)
{
	struct pair256 *cards=NULL,key;
	cJSON *cardsInfo=NULL;
	char t[1024];
	int32_t n,l=0;
	if(argc>=2)
	{
		if(strcmp(argv[1],"create-player")==0)
		{
			bet_player_create();
		}
		else if(strcmp(argv[1],"create-deck")==0)
		{
			if(argc!=3)
				goto end;
			n=atoi(argv[2]);
			cards=calloc(n,sizeof(struct pair256));
			bet_player_deck_create(n,cards);
		}
		else if(strcmp(argv[1],"blind-deck")==0)
		{
			bet_blind_deck(argv[2],argv[3]);
		}
		else if(strcmp(argv[1],"join-req")==0)
		{
			bet_player_join_req(argv[2],argv[3],argv[4]);
		}
		else if(strcmp(argv[1],"player-init")==0)
		{
			bet_player_init(atoi(argv[2]),argv[3],argv[4],argv[5]);
		}
		else if(strcmp(argv[1],"dcv-init")==0)
		{
			bet_dcv_init(atoi(argv[2]),atoi(argv[3]),argv[4]);
	
		}
		else if(strcmp(argv[1],"bvv-init")==0)
		{
			bet_bvv_init(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),argv[5]);
	
		}
		else if(strcmp(argv[1],"dcv-bvv-init")==0)
		{
			bet_dcv_bvv_init(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),argv[5],argv[6]);
	
		}
		else
		{
			printf("\nCommand Not Found");
		}
	}
	end:
		//printf("\nInvalid Arguments");
	return 0;
}


bits256 bet_curve25519_rand256(int32_t privkeyflag,int8_t index)
{
    bits256 randval;
    OS_randombytes(randval.bytes,sizeof(randval));
    if ( privkeyflag != 0 )
        randval.bytes[0] &= 0xf8, randval.bytes[31] &= 0x7f, randval.bytes[31] |= 0x40;
    randval.bytes[30] = index;
    return(randval);
}

struct pair256 bet_player_create()
{
	cJSON *playerInfo=NULL;
	struct pair256 key;

    key.priv=curve25519_keypair(&key.prod);
	playerInfo=cJSON_CreateObject();

	cJSON_AddStringToObject(playerInfo,"command","create-player");
    jaddbits256(playerInfo,"PubKey",key.prod);
	printf("\nPlayer PubKey: %s",cJSON_Print(cJSON_CreateString(cJSON_Print(playerInfo))));

	jaddbits256(playerInfo,"PrivKey",key.priv); 
	printf("\n%s",cJSON_Print(playerInfo));
	cJSON_Delete(playerInfo);

	return(key);
}

void bet_player_deck_create(int n,struct pair256 *cards)
{
	cJSON *deckInfo=NULL,*cardsInfo=NULL,*temp=NULL;
	int32_t i; 
	struct pair256 tmp;
	char *rendered=NULL;	

	deckInfo=cJSON_CreateObject();
	cJSON_AddStringToObject(deckInfo,"command","create-deck");
	cJSON_AddNumberToObject(deckInfo,"Number Of Cards",n);
	cJSON_AddItemToObject(deckInfo,"CardsInfo",cardsInfo=cJSON_CreateArray());
	
    for (i=0; i<n; i++) 
	{
		temp=cJSON_CreateObject();
        tmp.priv = bet_curve25519_rand256(1,i);
        tmp.prod = curve25519(tmp.priv,curve25519_basepoint9());
        cards[i] = tmp;

		cJSON_AddNumberToObject(temp,"Card Number",i);	
		jaddbits256(temp,"PrivKey",cards[i].priv);
		jaddbits256(temp,"PubKey",cards[i].prod);
		cJSON_AddItemToArray(cardsInfo,temp);
    }
    printf("\n%s",cJSON_Print(deckInfo));
	printf("\n%s",cJSON_Print(cJSON_CreateString(cJSON_Print(deckInfo))));
	cJSON_Delete(deckInfo);
	
    
}
void bet_blind_deck(char *deckStr,char *pubKeyStr)
{
	bits256 key,pubKey,privKey;
	cJSON *deckInfo=NULL,*cardsInfo=NULL,*card,*keyInfo=NULL,*blindDeckInfo=NULL;
    int32_t i,n; 
	char str[65];
	struct pair256 *blindCards=NULL;

	deckInfo=cJSON_CreateObject();
	deckInfo=cJSON_Parse(bet_strip(deckStr));
	if(deckInfo)
	{
		n=jint(deckInfo,"Number Of Cards");
		blindCards=calloc(n,sizeof(struct pair256));
		cardsInfo=cJSON_GetObjectItem(deckInfo,"CardsInfo");
		for(i=0;i<n;i++)
		{
			card=cJSON_GetArrayItem(cardsInfo,i);
			blindCards[i].priv=jbits256(card,"PrivKey");
		}
	}
	keyInfo=cJSON_CreateObject();
	keyInfo=cJSON_Parse(bet_strip(pubKeyStr));
	if(keyInfo)
	{
		key=jbits256(keyInfo,"PubKey");
	}
	
    for (i=0; i<n; i++)
    {
		blindCards[i].prod=curve25519(blindCards[i].priv,key);
	}
	if(cardsInfo)
		cJSON_Delete(cardsInfo);
	blindDeckInfo=cJSON_CreateObject();
	cJSON_AddStringToObject(blindDeckInfo,"command","blind-deck");
	cJSON_AddItemToObject(blindDeckInfo,"BlindDeck",cardsInfo=cJSON_CreateObject());
	for(i=0;i<n;i++)
	{
		card=cJSON_CreateObject();
		cJSON_AddNumberToObject(card,"Card Number",i);
		jaddbits256(card,"PrivKey",blindCards[i].priv);
		jaddbits256(card,"BlindPrivKey",blindCards[i].prod);
		cJSON_AddItemToArray(cardsInfo,card);
	}

	printf("\nBlinded Deck:\n%s",cJSON_Print(blindDeckInfo));
    printf("\n%s",cJSON_Print(cJSON_CreateString(cJSON_Print(blindDeckInfo))));

}

void bet_player_join_req(char *pubKeyStr,char *srcAddr,char *destAddr)
{
	int32_t pushSock,subSock,recvlen;
	char *recvBuf=NULL,*rendered=NULL;
	bits256 pubKey;
	cJSON *keyInfo=NULL,*joinInfo=NULL;
	
	keyInfo=cJSON_CreateObject();
	keyInfo=cJSON_Parse(bet_strip(pubKeyStr));
	if(keyInfo)
	{
		pubKey=jbits256(keyInfo,"PubKey");
	}

	 joinInfo=cJSON_CreateObject();
    cJSON_AddStringToObject(joinInfo,"method","join_req");
    jaddbits256(joinInfo,"pubkey",pubKey);    
    rendered=cJSON_Print(joinInfo);
   
	
	pushSock=BET_nanosock(0,destAddr,NN_PUSH);
	subSock=BET_nanosock(0,srcAddr,NN_SUB);
	nn_send(pushSock,rendered,strlen(rendered),0);
	while(1)
	{
		if ( (recvlen= nn_recv(subSock,&recvBuf,NN_MSG,0)) > 0 )
		{
			printf("\nResponse Received:%s",recvBuf);
			break;
		}
	}
}

int32_t bet_player_init(int32_t peerID,char *deckStr,char *pubKeyStr,char *destAddr)
{
	bits256 key,pubKey,privKey;
	cJSON *deckInfo=NULL,*cardsInfo=NULL,*keyInfo=NULL,*initInfo=NULL,*card;
    int32_t pushSock,i,n,bytes,retval=1;; 
	char str[65],*rendered=NULL;
	struct pair256 *cards=NULL;

	deckInfo=cJSON_CreateObject();
	deckInfo=cJSON_Parse(bet_strip(deckStr));
	if(deckInfo)
	{
		n=jint(deckInfo,"Number Of Cards");
		cards=calloc(n,sizeof(struct pair256));
		cardsInfo=cJSON_GetObjectItem(deckInfo,"CardsInfo");
		for(i=0;i<n;i++)
		{
			card=cJSON_GetArrayItem(cardsInfo,i);
			cards[i].priv=jbits256(card,"PrivKey");
			cards[i].prod=jbits256(card,"PubKey");
		}
	}
	keyInfo=cJSON_CreateObject();
	keyInfo=cJSON_Parse(bet_strip(pubKeyStr));
	if(keyInfo)
	{
		key=jbits256(keyInfo,"PubKey");
	}
	
   if(cardsInfo)
		cJSON_Delete(cardsInfo);
	initInfo=cJSON_CreateObject();
	cJSON_AddStringToObject(initInfo,"method","init_p");
	cJSON_AddNumberToObject(initInfo,"peerid",peerID);
	jaddbits256(initInfo,"pubkey",key);
	cJSON_AddItemToObject(initInfo,"cardinfo",cardsInfo=cJSON_CreateArray());
	for(i=0;i<n;i++)
	{
		cJSON_AddItemToArray(cardsInfo,cJSON_CreateString(bits256_str(str,cards[i].prod)));
	}
	rendered=cJSON_Print(initInfo);
	pushSock=BET_nanosock(0,destAddr,NN_PUSH);
	bytes=nn_send(pushSock,rendered,strlen(rendered),0);
	printf("\nInit Deck Info:\n%s",cJSON_Print(initInfo));
        printf("\n%s",cJSON_Print(cJSON_CreateString(cJSON_Print(initInfo))));	
     	printf("\nBytes Sent:%d",bytes);
	if(bytes<0)
        retval=-1;
    
	return retval;
}

void bet_dcv_init(int32_t n, int32_t r, char *dcvStr)
{
	
	cJSON *cardsProdInfo,*cjsong_hash,*dcvInfo=NULL;

	//struct deck_player_info playerDeckInfo;
	//bits256 g_hash[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];
	
	dcvInfo=cJSON_CreateObject();
	dcvInfo=cJSON_Parse(bet_strip(dcvStr));
	if(dcvInfo)
	{
		player_info.deckid=jbits256(dcvInfo,"deckid");
		cardsProdInfo=cJSON_GetObjectItem(dcvInfo,"cardprods");	
		
		for(int i=0;i<n;i++)
		{
			for(int j=0;j<r;j++)
			{
				player_info.cardprods[i][j]=jbits256i(cardsProdInfo,i*r+j);
			}
		}

		
		cjsong_hash=cJSON_GetObjectItem(dcvInfo,"g_hash");
		
		for(int i=0;i<n;i++)
		{
			for(int j=0;j<r;j++)
			{
				g_hash[i][j]=jbits256i(cjsong_hash,i*r+j);
			}
		}


	}

	printf("\n%s",cJSON_Print(dcvInfo));
	
}

struct enc_share bet_get_enc_share(cJSON *obj)
{
    struct enc_share hash; char *str;
	char hexstr[177];
    memset(hash.bytes,0,sizeof(hash));
   if ( obj != 0 )
    {
        if ( is_cJSON_String(obj) != 0 && (str= obj->valuestring) != 0 && strlen(str) == 176 ){
			
			decode_hex(hash.bytes,sizeof(hash),str);

        }
    }   

    return(hash);
}

void bet_bvv_init(int32_t peerID,int32_t n, int32_t r,char *bvvStr)

{
		//struct enc_share *g_shares=NULL;
		cJSON *bvvBlindCardsInfo,*shamirShardsInfo,*bvvInfo=NULL;
		bits256 temp,playerprivs[CARDS777_MAXCARDS],bvvPubKey;
		//struct deck_player_info player_info;
		//bits256 v_hash[CARDS777_MAXCARDS][CARDS777_MAXCARDS];
		
		
		bvvInfo=cJSON_Parse(bet_strip(bvvStr));
		if(bvvInfo)
		{
			bvvPubKey=jbits256(bvvInfo,"bvvpubkey");
			g_shares=(struct enc_share*)malloc(CARDS777_MAXPLAYERS*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS*sizeof(struct enc_share));
			bvvBlindCardsInfo=cJSON_GetObjectItem(bvvInfo,"bvvblindcards");
			
			for(int i=0;i<n;i++)
			{
				for(int j=0;j<r;j++)
				{
					player_info.bvvblindcards[i][j]=jbits256i(bvvBlindCardsInfo,i*r+j);
				}
			}

			shamirShardsInfo=cJSON_GetObjectItem(bvvInfo,"shamirshards");
			int k=0;
			for(int playerid=0;playerid<n;playerid++)
			{
				for (int i=0; i<r; i++)
		        {
		            for (int j=0; j<n; j++) 
					{
						g_shares[k]=bet_get_enc_share(cJSON_GetArrayItem(shamirShardsInfo,k));
						k++;
		            }
		        }
			}

			for(int i=0;i<r;i++)
			{
				for(int j=0;j<r;j++)
				{
					temp=xoverz_donna(curve25519(player_info.player_key.priv,curve25519(playerprivs[i],player_info.cardprods[peerID][j])));
					vcalc_sha256(0,v_hash[i][j].bytes,temp.bytes,sizeof(temp));
				}
			}
		}
		printf("\n%s",cJSON_Print(bvvInfo));

}


void bet_dcv_bvv_init(int32_t peerID,int32_t n, int32_t r,char *dcvStr, char *bvvStr)
{

	cJSON *cardsProdInfo,*cjsong_hash,*dcvInfo=NULL;
	struct deck_player_info dcvBlindDeckInfo;
	//bits256 g_hash[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];

	//struct enc_share *g_shares=NULL;
	cJSON *bvvBlindDeckInfo,*shamirShardsInfo,*bvvInfo=NULL;
	bits256 temp,playerprivs[CARDS777_MAXCARDS],bvvPubKey;
	//struct deck_player_info player_info;
	//bits256 v_hash[CARDS777_MAXCARDS][CARDS777_MAXCARDS];

		
	dcvInfo=cJSON_CreateObject();
	dcvInfo=cJSON_Parse(bet_strip(dcvStr));
	if(dcvInfo)
	{
		player_info.deckid=jbits256(dcvInfo,"deckid");
		cjsong_hash=cJSON_GetObjectItem(dcvInfo,"g_hash");
		
		for(int i=0;i<n;i++)
		{
			for(int j=0;j<r;j++)
			{
				g_hash[i][j]=jbits256i(cjsong_hash,i*r+j);
			}
		}


	}


	printf("\n%s\n",cJSON_Print(dcvInfo));
		
	
	
	bvvInfo=cJSON_Parse(bet_strip(bvvStr));
	if(bvvInfo)
	{
		bvvPubKey=jbits256(bvvInfo,"bvvpubkey");
		g_shares=(struct enc_share*)malloc(CARDS777_MAXPLAYERS*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS*sizeof(struct enc_share));
		bvvBlindDeckInfo=cJSON_GetObjectItem(bvvInfo,"bvvblindcards");
		
		for(int i=0;i<n;i++)
		{
			for(int j=0;j<r;j++)
			{
				player_info.bvvblindcards[i][j]=jbits256i(bvvBlindDeckInfo,i*r+j);
			}
		}
	
		shamirShardsInfo=cJSON_GetObjectItem(bvvInfo,"shamirshards");
		int k=0;
		for(int playerid=0;playerid<n;playerid++)
		{
			for (int i=0; i<r; i++)
			{
				for (int j=0; j<n; j++) 
				{
					g_shares[k]=bet_get_enc_share(cJSON_GetArrayItem(shamirShardsInfo,k));
					k++;
				}
			}
		}
	
		for(int i=0;i<r;i++)
		{
			for(int j=0;j<r;j++)
			{
				temp=xoverz_donna(curve25519(player_info.player_key.priv,curve25519(playerprivs[i],player_info.cardprods[peerID][j])));
				vcalc_sha256(0,v_hash[i][j].bytes,temp.bytes,sizeof(temp));
			}
		}
	}
	printf("\n%s\n",cJSON_Print(bvvInfo));
}


int32_t bet_get_share(int32_t n,int32_t r,int32_t peerID,cJSON *turnInfo)
{
	struct enc_share temp;
	int32_t cardID,retval,playerID,recvlen;
	uint8_t decipher[sizeof(bits256) + 1024],*ptr;
	bits256 share;
	char str[65];
	
	playerID=jint(turnInfo,"playerid");
	cardID=jint(turnInfo,"cardid");
	
	
	temp=g_shares[peerID*n*r + (cardID*n + playerID)];
	recvlen = sizeof(temp);

	if ( (ptr= BET_decrypt(decipher,sizeof(decipher),player_info.bvvpubkey,player_info.player_key.priv,temp.bytes,&recvlen)) == 0 )
	{
		retval=-1;
		printf("decrypt error ");
	}
	else
	{
		memcpy(share.bytes,ptr,recvlen);
		playershares[cardID][peerID]=share;
		sharesflag[cardID][peerID]=1;
		
		printf("\n%s:%d:share:%s",__FUNCTION__,__LINE__,bits256_str(str,share));
		
	}
	return retval;
}
void bet_player_turn(int32_t n,int32_t r,int32_t peerID,char *turnStr)
{
		cJSON *turnInfo=NULL;
		int32_t cardID,playerID;
		
		turnInfo=cJSON_Parse(bet_strip(turnStr));
		playerID=jint(turnInfo,"playerid");
		cardID=jint(turnInfo,"cardid");
		
		if(playerID==peerID)
		{
			no_of_shares=1;
			bet_get_share(n,r,peerID,turnInfo);	
		}
		else
		{
			//BET_p2p_client_give_share(argjson,bet,vars);
		}
}


