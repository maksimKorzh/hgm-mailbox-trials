#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#pragma warning(disable : 4996)
#define INLINE inline
#ifndef SPOIL
#define SPOIL
#endif

#ifndef SPOIL2
#define SPOIL2 0
#endif

#define STATS(X) X

#define PATH 0//ply==0 || path[0]==0x1324 && (ply==1 || path[1]==0x3122 && (ply==2 || path[2]==0x2451 && (ply==3 || path[3]==0x0 && (ply==4 || path[4]==0x0706 && (ply==5 || path[5]==0x0 && (ply==6 || path[6]==0x1450 && (ply==7 || path[7]==0x6051 && (ply==8 || path[8]==0x4465 && (ply==9 || path[9]==0x7050 && (ply==10 || path[10]==0x6577 && (ply==11 || path[11]==0x6677 && (ply==12 || path[12]==0x4354 && (ply==13 || path[13]==0x2716 && (ply==14 || path[14]==0x2522 && (ply==15 || path[15]==0x6454 && (ply==16 || path[16]==0x2262 && (ply==17 || path[17]==0x5010 && (ply>=18))))))))))))))))))

#define WHITE 16
#define BLACK 32
#define COLOR (WHITE|BLACK) // also used for edge guards

#define INF 8000

#define CAPTURED 254
#define MARGIN   30         // for futility pruning / lazy eval
#define STMKEY   123456789

#define KIWIPETE "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
#define FIDE     "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define PST(PIECE, SQR) pstData[offs[PIECE] + SQR]
#define KEY(PIECE, SQR) zobrist[offs[PIECE] + SQR]

typedef long long unsigned int uint64_t;

int nodeCnt, patCnt, genCnt, evalCnt, qsCnt, leafCnt, captCnt[2], discoCnt, makeCnt, mapCnt, illCnt, futCnt, pinCnt, moveCnt; // for statistics

int pstData[7*128];
long long int zobrist[7*128];


unsigned char rawBoard[20*16+2];  // 0x88 board with 2-wide rim of edge guards
#define board (rawBoard + 2*17)

int stm; // side to move (WHITE or BLACK)

int location[48], offs[48], mvv[48]; // piece list

unsigned char code[] = { // capture directions per piece
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,  0,
   1,  1,  1,  1,  1,  1,  1,  1,  0,  0,0203,0203,0204,0204,0207,  7,
 020,020,020,020,020,020,020,020,  0,  0,0230,0230,0240,0240,0270,070,
};

unsigned char capt[] = { 044, 022, 044, 011, 044, 011, 044, 022 }; // capture flags per direction

int vec[8] = { 16, 17, 1, -15, -16, -17, -1, 15 };

int firstDir[32] = { // offset of move list in steps[] table, per piece
  1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 31, 31, 36, 36, 27, 18, // white pieces
  5, 5, 5, 5, 5, 5, 5, 5, 9, 9, 31, 31, 36, 36, 27, 18, // black pieces
};

int firstCapt[32] = { // offset of move list in steps[] table, per piece (captures)
  2, 2, 2, 2, 2, 2, 2, 2, 9, 9, 31, 31, 36, 36, 27, 18, // white pieces
  6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 31, 31, 36, 36, 27, 18, // black pieces
};

signed char steps[] = {
  0,
  16, 15, 17, 0,                         //  1 wP
  -16, -15, -17, 0,                      //  5 bP
  31, -31, 33, -33, 18, -18, 14, -14, 0, //  9 N
  16, -16, 1, -1, 15, -15, 17, -17, 0,   // 18 K
  16, -16, 1, -1, 15, -15, 17, -17, 0,   // 27 Q   31 B
  16, -16, 1, -1, 0,                     // 36 R
   1, 5, 3, 7, 8, 4, 2, 6, 0,            // Q & B slides in terms of direction number
   1, 5, 3, 7, 0,                        // R slides in terms of direction number
};

int Nvec[] = {
  31, -31, 33, -33, 18, -18, 14, -14, 0, //  9 N
};

int Kvec[] = {
  16, -16, 1, -1, 15, -15, 17, -17, 0,   // 18 K
};

int cList[] = {
        0x28,       0x28,   0x28,   0x28,   0x28,   0x28,       0x28,       0x28,
  0x87654321, 0x87654321, 0x6248, 0x6428, 0x7351, 0x7351, 0x62487351, 0x62487351,
        0x64,       0x64,   0x64,   0x64,   0x64,   0x64,       0x64,       0x64,
  0x87654321, 0x87654321, 0x6248, 0x6428, 0x7351, 0x7351, 0x62487351, 0x62487351,
};

int leaperType[] = { // indexed by piece-16, 11 = K-like, 12 = N-like, 0 = slider
  11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 0, 0, 0, 0, 0, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 0, 0, 0, 0, 0, 11,
};

signed char sleaps[] = {
   0,
   1, 8, 2, 0,
   5, 4, 6, 0,
   1, 2, 3, 4, 5, 6, 7, 8, 0,
   1, 5, 3, 7, 8, 4, 2, 6, 0,
   1, 5, 3, 7, 8, 4, 2, 6, 0,            // Q & B slides in terms of direction number
   1, 5, 3, 7, 0,                        // R slides in terms of direction number
};

unsigned char slideOffs[] = { // offset of board-size table in firstSlide[], per piece
  128, 128, 128, 128, 128, 128, 128, 128, 8, 8, 128, 128, 8, 8, 0, 0,
  136, 136, 136, 136, 136, 136, 136, 136, 8, 8, 128, 128, 8, 8, 0, 0,
};

unsigned char firstSlide[] = {
  // Queen                             // Rook
  45, 21, 21, 21, 21, 21, 21, 41,      69, 58, 58, 58, 58, 58, 58, 66,
  27,  0,  0,  0,  0,  0,  0, 15,      62, 49, 49, 49, 49, 49, 49, 54,
  27,  0,  0,  0,  0,  0,  0, 15,      62, 49, 49, 49, 49, 49, 49, 54,
  27,  0,  0,  0,  0,  0,  0, 15,      62, 49, 49, 49, 49, 49, 49, 54,
  27,  0,  0,  0,  0,  0,  0, 15,      62, 49, 49, 49, 49, 49, 49, 54,
  27,  0,  0,  0,  0,  0,  0, 15,      62, 49, 49, 49, 49, 49, 49, 54,
  27,  0,  0,  0,  0,  0,  0, 15,      62, 49, 49, 49, 49, 49, 49, 54,
  33,  9,  9,  9,  9,  9,  9, 37,      63, 50, 50, 50, 50, 50, 50, 55,
  // Bishop                             // unused
  84, 83, 83, 83, 83, 83, 83, 91,       0,  0,  0,  0,  0,  0,  0,  0,
  86, 72, 72, 72, 72, 72, 72, 80,       0,  0,  0,  0,  0,  0,  0,  0,
  86, 72, 72, 72, 72, 72, 72, 80,       0,  0,  0,  0,  0,  0,  0,  0,
  86, 72, 72, 72, 72, 72, 72, 80,       0,  0,  0,  0,  0,  0,  0,  0,
  86, 72, 72, 72, 72, 72, 72, 80,       0,  0,  0,  0,  0,  0,  0,  0,
  86, 72, 72, 72, 72, 72, 72, 80,       0,  0,  0,  0,  0,  0,  0,  0,
  86, 72, 72, 72, 72, 72, 72, 80,       0,  0,  0,  0,  0,  0,  0,  0,
  89, 77, 77, 77, 77, 77, 77, 81,       0,  0,  0,  0,  0,  0,  0,  0,
};

signed char slides[] = { // 0-terminated sets of directions sliders can move in
   1, 5, 3, 7, 8, 4, 2, 6, 0,   // 0   Q
   5, 3, 7, 4, 6, 0,            // 9
   1, 5, 7, 8, 6, 0,            // 15
   1, 3, 7, 8, 2, 0,            // 21
   1, 5, 3, 4, 2, 0,            // 27
   5, 3, 4, 0,                  // 33
   5, 7, 6, 0,                  // 37
   1, 7, 8, 0,                  // 41
   1, 3, 2, 0,                  // 45
   1, 5, 3, 7, 0,               // 49, 50  R
   1, 5, 7, 0,                  // 54, 55
   1, 3, 7, 0,                  // 58
   1, 5, 3, 0,                  // 62, 63
   1, 7, 0,                     // 66
   1, 3, 0,                     // 69
   8, 4, 2, 6, 0,               // 72  B
   4, 6, 0,                     // 77
   8, 6, 0,                     // 80, 81
   8, 2, 0,                     // 83, 84
   4, 2, 0,                     // 86
   4, 0,                        // 89
   8, 0,                        // 91
};

char rawAlign[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
#define hippo (rawAlign + 7*17)

char codeBoard[256];

unsigned int moveStack[10000];
int msp; // move stack pointer

int path[100], ply;
int pv[10000];
int *pvPtr = pv;
int followPV;

void PrintBoard()
{
//  int i; for(i=0; i<128; i=i+9&~8) printf("%02x%s", board[i], (i&15) == 7 ? "\n" : " ");
  int i; for(i=-34; i<258; i++) printf("%02x%s", board[i], (i&15) == 15 ? "\n" : " ");
}

// Neighbor table

typedef union {
  long long int all;
  unsigned char d[8];
} Neighbor;

Neighbor neighbor[256];

void PrintNeighbor()
{ // for debugging only
  int r, f, i;
  for(i=0; i<8; i++) {
    for(r=8; r>=-1; r--) {
      for(f=-1; f<9; f++) {
        int s = 16*r + f & 255;
        printf("%4d", neighbor[s].d[i]);
      }
      printf("\n");
    }
    printf("\n");
  }
}

void Evacuate(int sqr)
{
  int d;
  for(d=0; d<4; d++) {
    int up = neighbor[sqr].d[d];
    int dn = neighbor[sqr].d[d+4];
    neighbor[up].d[d+4] = dn;
    neighbor[dn].d[d] = up;
  }
}

void Reoccupy(int sqr)
{
  int d;
  for(d=0; d<4; d++) {
    int up = neighbor[sqr].d[d];
    int dn = neighbor[sqr].d[d+4];
    neighbor[up].d[d+4] = neighbor[dn].d[d] = sqr;
  }
}

void Occupy(int sqr)
{
  int d;
  for(d=0; d<4; d++) {
    int dn, up = sqr, v = vec[d];
    while(!board[up+=v]) {}
    up &= 255; // Yegh!
    dn = neighbor[up].d[d+4];
    neighbor[up].d[d+4] = sqr;
    neighbor[dn].d[d] = sqr;
    neighbor[sqr].d[d+4] = dn;
    neighbor[sqr].d[d] = up;
  }
}

void InitNeighbor()
{ // initilizes a neighbor table for an empty board. (Opposit rim squares see each other.)
  int r, f, i;
  for(i=0; i<256; i++) neighbor[i].all = 0;
  for(r=0; r<8; r++) neighbor[16*r-1&255].d[2] = 16*r + 8, neighbor[16*r+8].d[6] = 16*r - 1;
  for(f=0; f<8; f++) neighbor[f-16 & 255].d[0] = 16*8 + f, neighbor[16*8+f].d[4] = f - 16;
  for(f=0; f<8; f++) neighbor[f-15 & 255].d[7] = 16*f +15, neighbor[16*f+15].d[3] = f - 15;
  for(f=0 ;f<8; f++) neighbor[f-17 & 255].d[1] = 136-16*f, neighbor[136-16*f].d[5] = f - 17;
  for(f=0; f<8; f++) neighbor[f+127].d[3] = 16*f - 8,  neighbor[16*f-8 & 255].d[7] = f + 127;
  for(f=0; f<8; f++) neighbor[f+129].d[5] = 95 - 16*f, neighbor[95-16*f & 255].d[1] = f + 129;
  for(i=0; i<8; i++) neighbor[12].d[i] = Nvec[i]+40, neighbor[11].d[i] = vec[i]+40;
}

// structure that contains most local variables needed in search routines
// so these can be easily shared amongst those by passing a pointer to it
typedef struct {
  long long int hashKey, oldKey;     // keys
  Neighbor savedNeighbor;            // 8 neighbors
  int *pv;                           // pointer to start of PV
  int *oldMap;                       // attack map
  int pstEval, oldEval, curEval;     // scores
  int alpha, beta, parentAlpha;      // scores
  int from, to;                      // squares
  int piece, victim;                 // pieces
  int depth;                         // depth
  int searched;                      // counter
  int first;                         // move index
  int pvMove;                        // move
  int targets, newThreats;           // victim sets
  int victimBit;                     // victim sets
  int oldPresence;                   // presence mask
  int moverSet, victimSet, inCheck;  // attackers sets
  int moverMask, victimMask;         // attackers sets with single attacker
} UndoInfo;

// Attack map

#define SZ 64

unsigned int attackersStack[3200]; // for allowing copy-make of attack maps
unsigned int *attackers = attackersStack + 256;
int colorDependents[32];  // to store some color-dependent variables
#define presence  (colorDependents - 16) // attackers set of not-captured pieces of the given color
#define vPresence (colorDependents - 15) // victim set of not-captured pieces of the given color

#define BSF(X) asm(" bsf %1, %0\n" : "=r" (bit) : "r" (X)) // assembler for doing bit = NrOfTrailingZeros(X);


void PrintRow(int data, int start, char end)
{
  int j; for(j=start; j<32; j+=2) printf("%c", data & 1<<j ? '@' : j > 19 && j < 30 ? ':' : '.');
  printf("%c", end);
}

void PrintMaps() {
  int i, j;
  printf("                            %08x         %08x\n", presence[WHITE], presence[BLACK]);
  printf("              presence: "); PrintRow(presence[WHITE], 0, ' '); PrintRow(presence[BLACK], 1, '\n');
  printf("                        PPPPPPPPNNBBRRQK ppppppppnnbbrrqk\n");
  for(i=16; i<48; i++) {
    printf("%2d. %08x    %c@%c%d    ", i, attackers[i],
                             "PPPPPPPPNNBBRRQKppppppppnnbbrrqk"[i-16], (location[i]&7)+'a', (location[i]>>4)+1);
    PrintRow(attackers[i], 0, ' '); PrintRow(attackers[i], 1, '\n');
  }
}

int noprint;

int victim2bit[] = { // mapping for victim sets (extract in MVV order)
  1<<31, 1<<30, 1<<29, 1<<28, 1<<27, 1<<26, 1<<25, 1<<24, 1<<23, 1<<22, 1<<21, 1<<20, 1<<19, 1<<18, 1<<17, 1<<16,
  1<<15, 1<<14, 1<<13, 1<<12, 1<<11, 1<<10, 1<<9,  1<<8,  1<<7,  1<<6,  1<<5,  1<<4,  1<<3,  1<<2,  1<<1,  1,
};

int attacker2bit[] = { // extract in LVA order (white and black interleaved)
     1, 1<<2, 1<<4, 1<<6, 1<<8, 1<<10, 1<<12, 1<<14, 1<<16, 1<<18, 1<<20, 1<<22, 1<<24, 1<<26, 1<<28, 1<<30,
  1<<1, 1<<3, 1<<5, 1<<7, 1<<9, 1<<11, 1<<13, 1<<15, 1<<17, 1<<19, 1<<21, 1<<23, 1<<25, 1<<27, 1<<29, 1<<31,
};

unsigned char bit2attacker[] = {
  16, 32, 17, 33, 18, 34, 19, 35, 20, 36, 21, 37, 22, 38, 23, 39,
  24, 40, 25, 41, 26, 42, 27, 43, 28, 44, 29, 45, 30, 46, 31, 47,
};

void MovePiece2(UndoInfo *u)
{ // generate moves for the piece from the square, and add or remove them from the attack map
  int bit = (attacker2bit-16)[u->piece];  // addsub is 1 or -1, and determines we set or clear
  int dir = (firstCapt-16)[u->piece];
  board[u->from] = 0;
  board[u->to] = 0;
  Evacuate(u->from);
  if(code[u->piece] & 0200) { // slider
    int d = steps[dir+14];                        // direction nr (+1)
    do {
      int sqr = neighbor[u->to].d[d-1];     // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[sqr];
      attackers[victim] += bit;           // set/clear the bit for this attacker
      sqr = neighbor[u->from].d[d-1];
      victim = board[sqr];
      attackers[victim] -= bit;
    } while((d = steps[++dir+14]));
  } else { // leaper
    int v = steps[dir];
    do {
      int sqr = u->to + v;                 // target square
      int victim = board[sqr];             // occupant of that square
      attackers[victim] += bit;            // set/clear the bit for this attacker
      sqr = u->from + v;
      victim = board[sqr];
      attackers[victim] -= bit;
    } while((v = steps[++dir]));
  }
  board[u->to] = u->piece;
}

void NewAddMoves(UndoInfo *u, int addsub)
{ // generate moves for the piece from the square, and add or remove them from the attack map
  int bit = (attacker2bit-16)[u->piece]*addsub;  // addsub is 1 or -1, and determines we set or clear
  if(code[u->piece] & 0200) { // slider
    int dir = (firstCapt-16)[u->piece];
    int d = steps[dir+14];                    // direction nr (+1)
    do {
      int sqr = neighbor[u->from].d[d-1];   // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[sqr];
      attackers[victim] += bit;           // set/clear the bit for this attacker
    } while((d = steps[++dir+14]));
  } else { // leaper
    int dir = firstCapt[u->piece-16];
    int v = steps[dir];
    do {
      int sqr = u->from + v;               // target square
      int victim = board[sqr];             // occupant of that square
      attackers[victim] += bit;            // set/clear the bit for this attacker
    } while((v = steps[++dir]));
  }
}

int AddMoves(int piece, int sqr, int addsub)
{ // generate moves for the piece from the square, and add or remove them from the attack map
  int bit = (attacker2bit-16)[piece]*addsub;  // addsub is 1 or -1, and determines we set or clear
  if(code[piece] & 0200) { // piece is slider
    int d, dir = firstSlide[(slideOffs-16)[piece] + sqr];
    while((d = slides[dir++])) {            // direction nr (+1)
      int to = neighbor[sqr].d[d-1];        // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[to];
      attackers[victim] += bit;             // set/clear the bit for this attacker
    }
  } else {
    int v, dir = (firstCapt-16)[piece];
    while((v = steps[dir++])) {
      int to = sqr + v;                    // target square
      int victim = board[to];              // occupant of that square
        attackers[victim] += bit;          // set/clear the bit for this attacker
    }
  }
}

void MapFromScratch(int *attackMap)
{
  int i, first = msp;
  memset((void*)(attackMap+16), 0, 128);
noprint++;
  for(i=WHITE; i<COLOR; i++) {
    int sqr = location[i];
    if(sqr != CAPTURED) AddMoves(i, sqr, 1);
  }
  msp = first;
  STATS(mapCnt++;)
noprint = 0;
}

char rawDirection[] = { // direction numbers per board step
  6, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 4, 0,
  0, 6, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 4, 0, 0,
  0, 0, 6, 0, 0, 0, 0, 5, 0, 0, 0, 0, 4, 0, 0, 0,
  0, 0, 0, 6, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 0,
  0, 0, 0, 0, 6, 0, 0, 5, 0, 0, 4, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 6, 0, 5, 0, 4, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 6, 5, 4, 0, 0, 0, 0, 0, 0, 0,
  7, 7, 7, 7, 7, 7, 7, 0, 3, 3, 3, 3, 3, 3, 3, 0,
  0, 0, 0, 0, 0, 0, 8, 1, 2, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 8, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 8, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0,
  0, 0, 0, 8, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0,
  0, 0, 8, 0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0,
  0, 8, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0,
  8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0,
};
#define direction (rawDirection + 7*17)

#define SLIDERS 0x3FF00000 // mask to extract slider attackers

int Discover(int incoming, int sqr)
{ // discovery of the slider moves over the mentioned square
  int threats = 0;                      // victim set for collecting the pin anchors
  incoming &= SLIDERS;                  // slider attackers of evacuated square
//printf("incoming=%08x @ %02x\n", incoming, sqr);
  while(incoming) {                     // loop over them
    int bit; BSF(incoming);             // extract next
    int pinner = bit2attacker[bit];     // get its piece number
    int src = location[pinner];         // and location
    int d = direction[sqr - src];       // the direction of the move towards us
    int dest = neighbor[sqr].d[d-1];    // where the slide now hits downstream
    int target = board[dest];           // and what it hits there
    int mask = 1 << bit;                // the bit corresponding to this slider
    if(target != COLOR) {               // ignore moves that go off board
      attackers[target] += mask;        // and add the slider to the attackers set of the target
      threats |= victim2bit[target-16]; // record newly-attacked targets
      STATS(discoCnt++;)
    }
    incoming -= mask;                   // clear the bit for this slider
  }
  return threats;
}

void NewDiscover(int incoming, int sqr)
{ // discovery of the slider moves over the mentioned square
  incoming &= SLIDERS;                  // slider attackers of evacuated square
//printf("incoming=%08x @ %02x\n", incoming, sqr);
  while(incoming) {                     // loop over them
    int bit; BSF(incoming);             // extract next
    int pinner = bit2attacker[bit];     // get its piece number
    int src = location[pinner];         // and location
    int d = direction[sqr - src];       // the direction of the move towards us
    int dest = neighbor[sqr].d[d-1];    // where the slide now hits downstream
    int target = board[dest];           // and what it hits there
    int mask = 1 << bit;                // the bit corresponding to this slider
    if(target != COLOR) {               // ignore moves that go off board
      attackers[target] += mask;        // and add the slider to the attackers set of the target
    }
    incoming -= mask;                   // clear the bit for this slider
  }
}

int INLINE PinDetect(int incoming, int sqr)
{ // discovery of the slider moves over the mentioned square
  int threats = 0;                      // victim set for collecting the pin anchors
  incoming &= SLIDERS;                  // slider attackers of evacuated square
//printf("incoming=%08x @ %02x\n", incoming, sqr);
  while(incoming) {                     // loop over them
    int bit; BSF(incoming);             // extract next
    int pinner = bit2attacker[bit];     // get its piece number
    int src = location[pinner];         // and location
    int d = direction[sqr - src];       // the direction of the move towards us
    int dest = neighbor[sqr].d[d-1];    // where the slide now hits downstream
    int target = board[dest];           // and what it hits there
    int mask = 1 << bit;                // the bit corresponding to this slider
    if(target != COLOR) {               // ignore moves that go off board
      threats |= (victim2bit-16)[target]; // record newly-attacked targets
      STATS(discoCnt++;)
    }
    incoming -= mask;                   // clear the bit for this slider
  }
  STATS(makeCnt++;)
  return threats;
}

void PreUpdate(UndoInfo *u)
{
  int sliders;
  u->moverSet  = attackers[u->piece];
  u->victimSet = attackers[u->victim];
  u->victimMask = attacker2bit[u->victim-16];
  u->moverMask  = attacker2bit[u->piece-16];
  u->oldPresence = presence[stm];                                 // for unmake
//printf("mset=%08x vset=%08x mmask=%08x vmask=%08x\n", u->moverSet, u->victimSet, u->moverMask, u->victimMask);
  presence[stm] -= u->victimMask;                                 // and neither do they attack anything
  NewDiscover(u->moverSet & presence[stm], u->from);              // discover opponent slider moves
  attackers[u->piece] = attackers[u->victim] - u->moverMask;      // inherit attackers, but remove self-attack
  attackers[u->victim] = u->moverMask;                            // this last attack will be removed with other old moves of mover
  if(attackers[u->piece] & presence[stm])                         // we can be recaptured
    u->newThreats |= victim2bit[u->piece-16];                     // so add mover to the possible retaliation victims
if(PATH) printf("       att=%08x pres=%08x bit=%08x nt=%08x\n", attackers[u->piece], presence[stm], victim2bit[u->piece-16], u->newThreats);
//printf("mover=%08x victim=%08x presence=%08x\n", attackers[u->piece], attackers[u->victim], presence[stm]);
  STATS(makeCnt++;)
}

void NewPreUpdate(UndoInfo *u)
{
  int sliders;
  u->moverMask  = (attacker2bit-16)[u->piece];
//printf("mset=%08x vset=%08x mmask=%08x vmask=%08x\n", u->moverSet, u->victimSet, u->moverMask, u->victimMask);
  presence[stm] -= u->victimMask;                                 // and neither do they attack anything
//  NewDiscover(u->moverSet & presence[stm], u->from);              // discover opponent slider moves
  attackers[u->piece] = attackers[u->victim] - u->moverMask;      // inherit attackers, but remove self-attack
  attackers[u->victim] = 0;//u->moverMask;                            // this last attack will be removed with other old moves of mover
if(PATH) printf("       att=%08x pres=%08x bit=%08x nt=%08x\n", attackers[u->piece], presence[stm], victim2bit[u->piece-16], u->newThreats);
//printf("mover=%08x victim=%08x presence=%08x\n", attackers[u->piece], attackers[u->victim], presence[stm]);
}

void INLINE Predict(UndoInfo *u)
{ // non-destructive preview of the trouble our moves invites
  int sliders, newPresence;
  u->moverSet  = attackers[u->piece];
  u->victimSet = attackers[u->victim];
  u->victimMask = (attacker2bit-16)[u->victim];
  u->oldPresence = presence[stm];                                 // for unmake
//printf("mset=%08x vset=%08x mmask=%08x vmask=%08x\n", u->moverSet, u->victimSet, u->moverMask, u->victimMask);
  newPresence = u->oldPresence - u->victimMask;                   // and neither do they attack anything
  u->newThreats = PinDetect(u->moverSet & newPresence, u->from);  // discover opponent slider moves
  if(u->victimSet & newPresence || u->newThreats & u->victimBit)  // we can be recaptured
    u->newThreats |= (victim2bit-16)[u->piece];                   // so add mover to the possible retaliation victims
if(PATH) printf("       att=%08x pres=%08x bit=%08x nt=%08x\n", attackers[u->piece], presence[stm], victim2bit[u->piece-16], u->newThreats);
//printf("mover=%08x victim=%08x presence=%08x\n", attackers[u->piece], attackers[u->victim], presence[stm]);
}

char *Move2text(int move)
{
  static char buf[10];
  sprintf(buf, "%c%d%c%d", (move >> 8 & 7) + 'a', (move >> 12 & 7) + 1, (move &7) + 'a', (move >> 4 & 7) + 1);
  return buf;
}

void Dump(char *msg) {
  int i;
  PrintMaps(attackers);
  for(i=0; i<ply; i++) printf(" %s", Move2text(path[i]));
  printf(" ply = %d %s\n", ply, msg); exit(0);
}

void VerifyMap(char *msg)
{
  int i, bad=0;

  attackers += SZ;
  MapFromScratch(attackers);
  attackers -= SZ;

  for(i=WHITE; i<COLOR; i++) if((attackers[i] ^ attackers[i+SZ]) & presence[COLOR ^ i & COLOR]) {
    bad++; printf("different attackers set at %d: %08x vs %08x\n", i, attackers[i], attackers[i+SZ]);
  }
  if(bad) PrintBoard(), Dump(msg), exit(0);
}


#define PVAL  90
#define NVAL 325
#define BVAL 350
#define RVAL 500
#define QVAL 950
#define KVAL INF


int pType[] = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6 };
int pieceVal[] = { 0, PVAL, NVAL, BVAL, RVAL, QVAL, KVAL };

int buf[10000];
void S() { register int i; int a[10000]; for(i=0; i<10000; i++) buf[i] = a[i]; }
void R() { register int i; int a[10000]; for(i=0; i<10000; i++) a[i] = buf[i]; }

void Init()
{
  int i, r, f;
  char *p;
  // board with rim
  for(i=0; i<20*16+2; i++) rawBoard[i] = COLOR;
  for(i=0; i<16; i++) {
    offs[i+WHITE] = 128*pType[i];
    offs[i+BLACK] = 128*pType[i] + 8;
    mvv[i+WHITE] = mvv[i+BLACK] = 16*(pType[i] | 1) << 24;
  }
  for(r=0; r<8; r++) for(f=0; f<8; f++) {
    int s = 16*r+f;
    int d = 14 - (r-3.5)*(r-3.5) - (f-4)*(f-4);
    board[s] = 0;
    for(i=1; i<6; i++) {
      pstData[128*i+s] =
      pstData[128*i+(s^0x70)+8] = pieceVal[i] + d*(i < 4) + 9*(i == 1)*r;
    }
    pstData[128*6+s] = pstData[128*6+(s^0x70)+8] = -d;
  }
  p = (char*) (zobrist + 128);
  for(i=0; i<6*8*128; i++) *p++ = rand()*rand() >> 6;
  while(63 & (int)attackers) attackers++;
}

#define MOVE(F, T) (256*(F) + (T))

int GenNoncapts()
{
  int i, xstm = stm ^ COLOR;

  for(i=0; i<16; i++) {
    int piece = xstm + i;
    int from = location[piece];
    if(from != CAPTURED) {
      int step, dir = (firstDir-16)[piece];
      while((step = steps[dir++])) {
        int to = from;
        do {
          int victim = board[to += step];
          if(!victim) {                        // target square empty
            if(!(piece & 8)) {                 // is Pawn
              if(step & 7) break;              // diagonal
              if(from - 32 & 64 && !board[to + step])
                moveStack[msp++] = MOVE(from, to + step);
            }
            moveStack[msp++] = MOVE(from, to); // generate non-capture
          } else break;
        } while(dir >= 27);
      }
    }
  }
  return msp;
}

int Evaluate(int pstEval)
{
  return pstEval;
}


int InterceptAttacks(int sqr)
{
  int d, i, mask = 0;
  for(d=0; d<8; d++) {                            // look in 8 directions
    int src = neighbor[sqr].d[d];                 // nearest occupied square
    int piece = board[src];                       // its occupant
    if(piece + 16 & 32) {                         // reject empty and edge guard
      int c = code[piece];                        // how does it move?
      if(c & 0200) {                              // it is a slider
        if(c & capt[d]) {                         // hits us (from any distance)
          int dest = neighbor[sqr].d[d^4];        // opposit neighbor square
          int victim = board[dest];               // what was there
          if(victim != COLOR)                     // reject edge guards
            attackers[victim] -= (attacker2bit-16)[piece];
          mask |= attacker2bit[piece-16];         // record this attack
//printf("slider %d attacks at %02x\n", piece, sqr);
        }
      } else {                                    // it is a leaper
        if(c & capt[d] && src == sqr + vec[d]) {  // hits us (only when adjacent)
          mask |= (attacker2bit-16)[piece];       // record this attack
//printf("leaper %d attacks at %02x\n", piece, sqr);
        }
      }
    }
  }
  for(i=8; i<10; i++) { // Knights
    int from = location[WHITE+i];
    if(from != CAPTURED && hippo[sqr-from])
      mask |= (attacker2bit+WHITE-16)[i];
    from = location[BLACK+i];
    if(from != CAPTURED && hippo[sqr-from])
      mask |= (attacker2bit+BLACK-16)[i];
  }
  return mask;
}

void UnMake(UndoInfo *u)
{
  // restore board and piece list
ply--;
  board[u->from] = u->piece;
  board[u->to]   = u->victim;
  location[u->piece]  = u->from;
  location[u->victim] = u->to;
  if(u->victim) presence[stm] += (attacker2bit-16)[u->victim], vPresence[stm] += (victim2bit-16)[u->victim];
  if(!u->victim) Evacuate(u->to), neighbor[u->to] = u->savedNeighbor;
  Reoccupy(u->from);
  attackers -= SZ;
  codeBoard[u->from] = code[u->piece];
  codeBoard[u->to]   = code[u->victim];
}

int MakeMove(int move, UndoInfo *u)
{
  // decode the move
  u->to = move & 255;
  u->from = move >> 8 & 255;
  u->piece = board[u->from];
  u->victim = board[u->to];

  // update the incremental evaluation
  u->pstEval = -(u->oldEval + PST(u->piece, u->to) - PST(u->piece, u->from) + PST(u->victim, u->to));
//if(PATH) printf("     eval=%d beta=%d MARGIN=%d victim=%d to=%02x offs=%d val=%d\n", u->pstEval, u->beta, MARGIN, u->victim, u->to, offs[u->victim], PST(u->victim, u->to));
  if(u->depth <= 0 && u->pstEval > u->beta + MARGIN) return -INF-1; // futility (child will stand pat)

  // update hash key, and possibly abort on repetition
  u->hashKey =   u->oldKey  ^ KEY(u->piece, u->to) ^ KEY(u->piece, u->from) ^ KEY(u->victim, u->to);
  // if(REPEAT) return 0;

  memcpy((void*) (attackers + WHITE + SZ), (void*) (attackers + WHITE), 128);
  attackers += SZ;
  {
    u->moverSet  = attackers[u->piece];
    u->moverMask  = (attacker2bit-16)[u->piece];
    board[u->from] = 0;                                             // 'lift' the moved piece
    AddMoves(u->piece, u->from, -1);                                // and remove its moves
    Evacuate(u->from);
    u->newThreats = Discover(u->moverSet & presence[stm], u->from); // discover opponent slider moves
    Discover(u->moverSet & presence[COLOR-stm], u->from);           // and own own slider moves
    // the mover is now consistently gone
    if(!u->victim) u->savedNeighbor = neighbor[u->to], Occupy(u->to);
    attackers[u->piece] = InterceptAttacks(u->to);                  // build attackers set, and remove the blocked sliders from their old targets
    if(attackers[u->piece])                                         // we went to an attacked square
      u->newThreats |= (victim2bit-16)[u->piece];                   // so add mover to the possible retaliation victims
    AddMoves(u->piece, u->to, 1);
if(PATH) printf("       att=%08x pres=%08x bit=%08x nt=%08x\n", attackers[u->piece], presence[stm], victim2bit[u->piece-16], u->newThreats);
//printf("mover=%08x victim=%08x presence=%08x\n", attackers[u->piece], attackers[u->victim], presence[stm]);
  }

  // update board and piece list
  board[u->to]   = u->piece;
  location[u->piece]  = u->to;
  location[u->victim] = CAPTURED;
  if(u->victim) presence[stm] -= (attacker2bit-16)[u->victim], vPresence[stm] -= (victim2bit-16)[u->victim];

//  VerifyMap("in MakeMove()");

//  attackers += SZ;
//  MapFromScratch(attackers);
path[ply++] = move & 0xFFFF;
  if(attackers[COLOR+15 - stm] & presence[stm]) { UnMake(u); return -INF; } // move was illegal
  captCnt[!u->victim]++;
  return INF+1; // kludge to indicate success
}

// decoding table for the bits in merged attacker sets
// accessed as bit2piece[bit + OFFSET] in move extraction loop,
// where OFFSET is one of the macros defined above the two lines
// to which it apples
char bit2piece[] = {
//define bit2attacker (bit2piece - 16)
  16, 32, 17, 33, 18, 34, 19, 35, 20, 36, 21, 37, 22, 38, 23, 39,
  24, 40, 25, 41, 26, 42, 27, 43, 28, 44, 29, 45, 30, 46, 31, 47,
#define Q_VICTIM 16
  46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30,
  46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30,
};

char Qvictim[] = {
  46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30,
  46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30,
};

#define MERGE(A, OP, B) capts = (A & myPresence) OP (B & myPresence)

#define SUPER_LOOP \
  while(capts) {\
    int bit; BSF(capts);\
    int inc = incs[bit+base];\
    base += inc;\
    capts >>= inc;\
  }

#define EXTRACT_AND_PLAY(FRIEND, FOE, S) \
if((PATH) && ply==-1) PrintMaps(attackers), PrintBoard(), printf("%d. capts=%08x N=%08x B=%08x P=%08x A=%08x %08x\n", S, capts, capturesN, capturesB, capturesP, capturesA, myPresence);/**/\
  while(capts) {                          \
    int bit; BSF(capts);                  \
    undo.victim = FOE;                    \
    undo.piece = FRIEND;                  \
    if(SearchCapture(&undo)) goto cutoff; \
    capts &= capts - 1;                   \
  }                                       \

#define CAPT_CASES(ACOL, VCOL, OP) \
/* ACOL = WHITE or BLACK   (attacker color)                       */\
/* VCOL = BLACK or WHITE   (victim color)                         */\
/* OP   = "+ 2*" (for white victim) and ">>1 |" (black victim)    */\
    case VCOL+15:                                                   \
      if(attackers[VCOL+15] & myPresence) { undo.parentAlpha = INF; goto cutoff; }\
    case VCOL+14: /* Queen */                                       \
      if(u->depth <= 1 && undo.parentAlpha > u->pstEval + QVAL+MARGIN+10) goto cutoff;  \
      capts = attackers[VCOL+14] & myPresence;                      \
      EXTRACT_AND_PLAY(bit2attacker[bit], Qvictim[bit], 0)             \
      if(u->depth <= 1 && undo.parentAlpha > u->pstEval + RVAL+MARGIN+10) goto cutoff;  \
    case VCOL+13: /* Rooks */                                       \
    case VCOL+12:                                                   \
      capts = MERGE(attackers[VCOL+12], OP, attackers[VCOL+13]);    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+12+(bit&1), 1)              \
      if(u->depth <= 1 && undo.parentAlpha > u->pstEval + BVAL+MARGIN+10) goto cutoff;  \
    case VCOL+11: /* Bishops */                                     \
    case VCOL+10:                                                   \
    case VCOL+9:  /* Knights */                                     \
    case VCOL+8:                                                    \
if((PATH) && ply==-1) printf("%08x %08x %08x\n", capts, attackers[VCOL+8], attackers[VCOL+9]);\
      capts = MERGE(attackers[VCOL+10], OP, attackers[VCOL+11]);    \
      capturesB = capts; capts &= 0xFFFF; /* 8 PxN            */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+10+(bit&1), 2)              \
      capts = MERGE(attackers[VCOL+8], OP, attackers[VCOL+9]);      \
      capturesN = capts; capts &= 0xFFFF; /* 8 PxN            */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+8+(bit&1), 3)               \
      capts = capturesB & 0xFF0000;       /* 4 NxB, 4 BxB     */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+10+(bit&1), 4)              \
      capts = capturesN & 0xFF0000;       /* 4 NxN, 4 BxN     */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+8+(bit&1), 5)               \
      capts = capturesB & 0xFF000000;     /* 2 RxB, QxB, KxN  */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+10+(bit&1), 6)              \
      capts = capturesN & 0xFF000000;     /* 2 RxN, QxN, KxN  */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+8+(bit&1), 16)               \
      if(u->depth <= 1 && undo.parentAlpha > u->pstEval + PVAL+MARGIN+10) goto cutoff;  \
    case VCOL+7:  /* Pawns */                                       \
    case VCOL+6:                                                    \
    case VCOL+5:                                                    \
    case VCOL+4:                                                    \
    case VCOL+3:                                                    \
    case VCOL+2:                                                    \
    case VCOL+1:                                                    \
    case VCOL:                                                      \
      capts = MERGE(attackers[VCOL+6], OP, attackers[VCOL+7]);      \
      capturesA = capts; capts &= 0xFFFF; /* 8 PxP            */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+6+(bit&1), 7)               \
      capts = MERGE(attackers[VCOL+4], OP, attackers[VCOL+5]);      \
      capturesB = capts; capts &= 0xFFFF; /* 8 PxP            */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+4+(bit&1), 8)               \
      capts = MERGE(attackers[VCOL+2], OP, attackers[VCOL+3]);      \
      capturesP = capts; capts &= 0xFFFF; /* 8 PxP            */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+2+(bit&1), 9)               \
      capts = MERGE(attackers[VCOL],   OP, attackers[VCOL+1]);      \
      capturesN = capts; capts &= 0xFFFF; /* 8 PxP            */    \
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL + (bit&1), 10)               \
      capts = capturesA & 0xFFFF0000; /* 4NxP 4BxP 4RxP 2QxP 2KxP */\
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+6+(bit&1), 12)               \
      capts = capturesB & 0xFFFF0000; /* 4NxP 4BxP 4RxP 2QxP 2KxP */\
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+4+(bit&1), 13)               \
      capts = capturesP & 0xFFFF0000; /* 4NxP 4BxP 4RxP 2QxP 2KxP */\
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL+2+(bit&1), 14)               \
      capts = capturesN & 0xFFFF0000; /* 4NxP 4BxP 4RxP 2QxP 2KxP */\
      EXTRACT_AND_PLAY(ACOL+(bit>>1), VCOL + (bit&1), 15)               \


int SearchCapture(UndoInfo *u);
int SearchNonCapts(UndoInfo *undo, UndoInfo *u);

int CaptsLoop(UndoInfo *u)
{
}

int Search(UndoInfo *u) // pass all parameters in a struct
{
  UndoInfo undo;
  int *p;
  int curMove, noncapts = 10000;
  undo.pv = pvPtr;
  undo.first = msp;
  nodeCnt++;
  *pvPtr++ = 0; // empty PV
  undo.parentAlpha = u->alpha;
  undo.inCheck = attackers[stm+15] & presence[COLOR-stm];
  undo.oldMap  = attackers;

if(PATH) printf("%2d    Search(%d,%d,%d) eval=%d nb=%016llx check=%08x\n       %d %ld %ld\n", ply, u->alpha, u->beta, u->depth, u->pstEval, neighbor[0].all, undo.inCheck, msp, pvPtr-pv, attackers-attackersStack),PrintBoard(),PrintMaps(attackers), fflush(stdout);
  // QS / stand pat
  if(u->depth <= 0) {
    int *p;
    qsCnt++;
    if(u->pstEval > undo.parentAlpha - MARGIN) { // don't bother with full eval if hopeless
      undo.curEval = Evaluate(u->pstEval); evalCnt++;
      if(undo.curEval > undo.parentAlpha) {
        if(undo.curEval >= u->beta) { patCnt++; return u->beta; }
        undo.parentAlpha = undo.curEval;
      }
    }
  } else if(!undo.inCheck && u->pstEval > u->beta - MARGIN) { // null-move pruning
    int score;
    undo.hashKey =  u->hashKey ^ STMKEY;
    undo.pstEval = -u->pstEval;
    undo.alpha   = -u->beta;
    undo.beta    = 1 - u->beta;
    undo.depth   = (u->depth > 3 ? u->depth - 3 : 0);
    undo.from    = -1;
if(PATH)printf("null move\n"), fflush(stdout);
    undo.targets = vPresence[stm];
    stm ^= COLOR; path[ply++]=0;
//    attackers += SZ;
//    MapFromScratch(attackers);
    score = -Search(&undo);
//    attackers -= SZ;
    stm ^= COLOR; ply--;
if(PATH) printf("null done score = %d\n", score), fflush(stdout);
    if(score >= u->beta) return u->beta;
  }

  // set child & make-move parameters that are always the same
  undo.oldKey  =  u->hashKey ^ STMKEY;
  undo.oldEval =  u->pstEval;
  undo.depth   =  u->depth - 1;
  undo.alpha   = -u->beta;
  undo.searched = 0;
  stm ^= COLOR;

  // generate moves
  if(followPV >= 0) {              // first branch
    undo.pvMove = pv[followPV++];  // get the move
    if(undo.pvMove) {
      undo.beta = -undo.parentAlpha;
      undo.searched++;
      MakeMove(undo.pvMove, &undo);
      undo.parentAlpha = -Search(&undo);
      UnMake(&undo);
    } else followPV = -1;
  } else undo.pvMove = 0;

//  undo.captured = 0;               // keep records of what captures we find ...
//  undo.untried  = 0xFFFFFFFF;      // ... or rule out

  if(u->targets) { // capture section
    unsigned int capts, capturesN, capturesB, capturesP, capturesA;
    int myPresence = presence[COLOR-stm];
    int bit; BSF(u->targets);
    int mvv = COLOR-1 - bit;
if(PATH) printf("       targets=%08x bit=%d MVV=%d\n", u->targets, bit, mvv);
//int i;
//for(i=stm+14; i>mvv; i--) if(attackers[i] & myPresence) printf("at %d a=%08x p=%08x mvv=%d\n",i,attackers[i],myPresence,mvv),Dump("MVV"),exit(0);
//mvv = stm + 14;
    switch(mvv) {
      CAPT_CASES(WHITE, BLACK, +2*  ) // white captures black
      break;
      CAPT_CASES(BLACK, WHITE, >>1| ) // black captures white
      case 0: ;
    }
  }

  if(u->depth <= 0 || u->depth <= 1 && undo.parentAlpha > u->pstEval + MARGIN + 10) goto cutoff;

  SearchNonCapts(&undo, u);

 cutoff:
if(PATH) printf("%2d    done\n", ply),fflush(stdout);
  stm ^= COLOR;
 abort:
//  u->threats = undo.captured;
//  u->report  = undo.untried;
  leafCnt += !undo.searched;
  msp = undo.first;    // pop move list
  pvPtr = undo.pv;     // pop PV (but remains above stack top)
  return undo.parentAlpha;
}

int SearchNonCapts(UndoInfo *undo, UndoInfo *u)
{
  int curMove;

  GenNoncapts(); genCnt++;
  undo->targets = vPresence[COLOR - stm];

  // move loop
  for(curMove = undo->first; curMove < msp; curMove++) {
    int score;
    unsigned int move;

    move = moveStack[curMove];
//if(PATH) {int i; for(i=undo.first; i<msp; i++) printf("      %d. %s\n", i, Move2text(moveStack[i])); }

    if(move == undo->pvMove && !followPV) continue; // duplicat moves

    // recursion
    undo->beta = -undo->parentAlpha;
if(PATH) printf("%2d:%d %3d. %08x %s               %016llx\n", ply, u->depth, curMove, move, Move2text(move), neighbor[0].all), fflush(stdout);
    score = MakeMove(move, undo); // rejected moves get their score here
//if(PATH) printf("      %s: score=%d\n", Move2text(move), score);
    if(score < -INF) break;        // move is futile, and so will be all others
    if(score > INF) { // move successfully made
      undo->searched++;
ply--;
if(PATH) printf("%2d:%d %3d. %08x %s               %016llx\n", ply, u->depth, curMove, move, Move2text(move), neighbor[0].all), fflush(stdout);
ply++;
      score = -Search(undo);
      UnMake(undo);
    }
if(PATH) printf("%2d:%d %3d. %08x %s %5d %5d n %016llx %d\n", ply, u->depth, curMove, move, Move2text(move), score, undo->parentAlpha, neighbor[0].all, nodeCnt), fflush(stdout);

    // minimaxing
    if(score > undo->parentAlpha) {
      int *p;
      if(score >= u->beta) {
        undo->parentAlpha = u->beta;
        break;
      }
      undo->parentAlpha = score;
      p = pvPtr; pvPtr = undo->pv; // pop old PV
      *pvPtr++ = move;             // push new PV, starting with this move
      while((*pvPtr++ = *p++)) {}  // and append child PV
    }
  }
}

int INLINE GetMVV(UndoInfo *u)
{
  {
#define att64(X) *(uint64_t*) (attackers + X)
    int xstm = COLOR - stm;
    int oppoPresence = presence[stm] - u->victimMask;
    int gap = u->alpha - MARGIN - 10 - u->pstEval;     // futility gap child will have to bridge with a capture
    int targets;
    uint64_t presence64 = oppoPresence * 0x100000001ull;
    int nonFutiles = 0xFFFFFFFF;
    attackers[u->piece] = 0;
    nonFutiles = (gap > PVAL ? 0x00FF00FF : nonFutiles);
    nonFutiles = (gap > BVAL ? 0x000F000F : nonFutiles);
    nonFutiles = (gap > RVAL ? 0x00030003 : nonFutiles);
    nonFutiles = (gap > QVAL ? 0x00000000 : nonFutiles);
    targets  = (attackers[14+xstm] & oppoPresence ? 0x00020002 : 0);
    targets |= (att64(12+xstm) & presence64 ? 0x000C000C : 0);
    targets |= ((att64(8+xstm)|att64(10+xstm)) & presence64 ? 0x00F000F0 : 0);
    targets |= ((att64(xstm)|att64(2+xstm)|att64(4+xstm)|att64(6+xstm)) & presence64 ? 0xFF00FF00 : 0);
    attackers[u->piece] = u->moverSet;
if(PATH) printf("new=%08x target=%08x pres=%08x\n",u->newThreats,targets,vPresence[xstm]);
    targets |= u->newThreats;
    targets &= nonFutiles;
    u->targets = targets &= vPresence[xstm];
    return (targets | gap <= 0);
  }
}

int Finish(UndoInfo *u);

int SearchCapture(UndoInfo *u)
{ // make / search / unmake and score minimaxing all in one
  STATS(moveCnt++;)
  u->victimBit = (victim2bit-16)[u->victim];
//  u->captured |= victimBit;                   // remember this victim is attacked
//  u->untried  &= (untriedMask-16)[u->victim]; // we must have tried all captures on higher value groups, remove these

  u->victimMask = (attacker2bit-16)[u->victim];             // save for unmake
if(PATH) printf("%2d      %s check=%08x mask=%08x\n", ply, Move2text(256*location[u->piece] + location[u->victim]), u->inCheck, u->victimMask);
  if(u->inCheck && u->inCheck & ~u->victimMask              // we were in check, and did not capture checker,
                && u->piece != COLOR + 15 - stm) { STATS(illCnt++;) return 0; } // nor moved the King => move is illegal

  // decode move
  u->to = location[u->victim];

  // detect futility at earliest opportunity
  u->pstEval = u->oldEval + PST(u->victim, u->to);
//if(PATH) printf("      search, lazyEval=%d, alpha=%d\n",u->pstEval, u->parentAlpha),fflush(stdout);
  if(u->depth <= 0 && u->pstEval < u->parentAlpha - MARGIN) { STATS(futCnt++;) return 1; } // futility (child will stand pat)\}

  // finish decoding move and updating evaluation
  u->from = location[u->piece];
  u->pstEval = -(u->pstEval + PST(u->piece, u->to) - PST(u->piece, u->from));

  // update hash key, and possibly abort on repetition
  u->hashKey =   u->oldKey  ^ KEY(u->piece, u->to) ^ KEY(u->piece, u->from) ^ KEY(u->victim, u->to);

  Predict(u);

  if(u->newThreats & 1 << stm - WHITE) { STATS(pinCnt++;) return 0; } // illegal move, abort

  if(u->depth <= 1 SPOIL) { // overkill pruning: test if child will be futile
    if(!GetMVV(u)) {
      u-> parentAlpha = -u->alpha;                     // fail-high score (-u->alpha is parent's beta)
/*      u->searched++;*/ nodeCnt++; qsCnt += (u->depth <= 0);
      return 1;                                        // and abort move ('overkill pruning')
    }
  } else u->targets = vPresence[COLOR - stm]; // at higher depth we let child try all present pieces

  return Finish(u);
}

int Finish(UndoInfo *u)
{
  // committed to move
  int score, move = MOVE(u->from, u->to);
  u->searched++;
  {

if(PATH) { printf("%2d:%d %3d. %08x %s (%d)\n", ply, u->depth+1, 0, move, Move2text(move), stm), fflush(stdout);}
    // finish map update and apply move to board
  memcpy((void*) (attackers + WHITE + SZ), (void*) (attackers + WHITE), 128);
  attackers += SZ;
    NewPreUpdate(u);
//    AddMoves(u->piece, u->from, -1);

//    board[u->from] = 0;
//    board[u->to]   = u->piece;
    location[u->piece]  = u->to;
    location[u->victim] = CAPTURED;
    vPresence[stm] -= u->victimBit;
//    Evacuate(u->from);

    MovePiece2(u);

    NewDiscover(u->moverSet & (presence[WHITE]|presence[BLACK]), u->from);
//    AddMoves(u->piece, u->to, 1);

    path[ply++] = move; captCnt[0]++;

//    attackers += SZ;
//    presence[stm] -= (attacker2bit-16)[u->victim];
//    MapFromScratch(attackers);

//    VerifyMap("after make");

//ply--; if(PATH) { printf("%2d:%d %3d. %08x %s\n", ply, u->depth+1, 0, move, Move2text(move)), fflush(stdout);} ply++;
    u->beta = -u->parentAlpha;
    score = - Search(u);

   abort:
    // unmake move
    board[u->from] = u->piece;
    board[u->to]   = u->victim;
    location[u->piece]  = u->from;
    location[u->victim] = u->to;
    presence[stm] = u->oldPresence;
    vPresence[stm] += u->victimBit;
    Reoccupy(u->from);

//if(PATH) printf("%2d    Restore(%d,%d)\n", ply, u->first, u->piece),fflush(stdout);
//    FullMapRestore(u);
//    VerifyMap("after unmake");
    attackers -= SZ;


    ply--;
//if(PATH) printf("%2d    Reoccupy(%02x)\n", ply, u->from),fflush(stdout);
  }
if(PATH) { printf("%2d:%d %3d. %08x %s %5d %5d   %016llx %d\n", ply, u->depth+1, 0, move, Move2text(move), score, u->parentAlpha, neighbor[0].all, nodeCnt), fflush(stdout);}

  // minimax
  if(score > u->parentAlpha) {
    int *p;
    u->parentAlpha = score;
    if(score + u->alpha >= 0) { // u->alpha = -parentBeta!
      return 1;
    }
    p = pvPtr; pvPtr = u->pv;   // pop old PV
    *pvPtr++ = move;            // push new PV, starting with this move
    while((*pvPtr++ = *p++)) {} // and append child PV
  }
  return 0;
}

void SearchRoot(UndoInfo *u, int maxDepth)
{
  nodeCnt = patCnt = evalCnt = genCnt = qsCnt = leafCnt = captCnt[0] = captCnt[1] = discoCnt = makeCnt = mapCnt = futCnt = illCnt = pinCnt = moveCnt = 0;
  u->alpha = -INF;
  u->beta  = INF;
  u->from = -1;
  u->targets = vPresence[COLOR-stm];
  followPV = -1;   // nothing to follow at d=1
  for(u->depth=1; u->depth<=maxDepth; u->depth++) { // iterative deepening
    int i, score;
    score = Search(u);
    printf("%2d %4d %9d %d", u->depth, score, nodeCnt, 0);
    for(i=0; pv[i]; i++) printf(" %s", Move2text(pv[i]));
    printf("\n"), fflush(stdout);
    followPV = 0;  // follow this PV on next search
  }
}

int Setup(UndoInfo *u, char *fen)
{
  static char pieces[] = "PPPPPPPPNNBBRRQK";
  int r, f, i;
  for(i=WHITE; i<COLOR; i++) location[i] = CAPTURED;
  for(r=0; r<8; r++) for(f=0; f<8; f++) board[16*r+f] = codeBoard[16*r+f] = 0;
  InitNeighbor();
  u->pstEval = 0; u->hashKey = 0;
  presence[WHITE] = presence[BLACK] = 0;
  vPresence[WHITE] = vPresence[BLACK] = 0;

  r = 7; f = 0;
  while(*fen) {
    if(*fen == '/') r--, f = 0;
    else if(*fen > '0' && *fen <= '9') f += *fen - '0'; // empties
    else if(*fen >= 'A') {                              // piece
      int color = (*fen >= 'a' ? BLACK : WHITE);
      for(i=0; i<16; i++) {
        int sqr = 16*r + f;
        if(location[color+i] == CAPTURED && !(*fen - pieces[i] & 31)) {
          location[color+i] = sqr;
          board[sqr] = color + i;
          codeBoard[sqr] = code[color+i];
          u->pstEval += PST(color + i, sqr)*(color == WHITE ? 1 : -1);
          u->hashKey ^= KEY(color + i, sqr);
          presence[color] |= attacker2bit[color+i-16];
          vPresence[color] |= victim2bit[color+i-16];
          Occupy(sqr);
          break;
        }
      }
      f++;
    } else break;
    fen++;
  }
  while(*fen == ' ') fen++;
  if(*fen == 'b') u->pstEval *= -1;
  return (*fen == 'b' ? BLACK : WHITE);
}

UndoInfo root;

int main()
{
  time_t t;
  Init();
//int j, i = firstLeap[slideOffs[17] + 0x56]; while((j = leaps[i++])) printf(" %d. %d:%d\n", slideOffs[1], i, j);
//return;
  stm = Setup(&root, FIDE);
  stm = Setup(&root, KIWIPETE);
  MapFromScratch(attackers);
//  PrintMaps();
  t = clock();
  SearchRoot(&root, 10);
  t = clock() - t;
  printf("t = %6.3f sec\n", t * (1./CLOCKS_PER_SEC));
  printf("%10d nodes (%3.1f Mnps)\n%10d QS (%3.1f%%)\n", nodeCnt, nodeCnt*1e-6*CLOCKS_PER_SEC/t, qsCnt, qsCnt*100./nodeCnt);
  printf("%10d evals (%3.1f%%)\n%10d stand-pats (%3.1f%%)\n", evalCnt, evalCnt*100./qsCnt, patCnt, patCnt*100./qsCnt);
  printf("%10d leaves (%3.1f%%)\n", leafCnt, leafCnt*100./qsCnt);
  printf("%10d move gens\n", genCnt);
  printf("%10d map gens\n", mapCnt);
  printf("%10d moves\n", moveCnt);
  printf("%10d non-evasions\n", illCnt);
  printf("%10d futiles\n", futCnt);
  printf("%10d illegals\n", pinCnt);
  printf("%4.2f pins/move\n", discoCnt/(double)makeCnt);
  printf("captures: %3.1f%%\n", captCnt[0]*100./(captCnt[0] + captCnt[1]));
  return 0;
}
