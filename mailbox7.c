#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#pragma warning(disable : 4996)

#ifndef SPOIL
#define SPOIL
#endif

#define STATS(X) X

#define PATH 0//ply==0 || path[0]==0x4354 && (ply==1 || path[1]==0x6454 && (ply==2 || path[2]==0x1450 && (ply==3 || path[3]==0x5444 && (ply==4 || path[4]==0x1627 && (ply==5 || path[5]==0x4447 && (ply==6 || path[6]==0x2547 && (ply==7 || path[7]==0x6444 && (ply==8 || path[8]==0x2555 && (ply==9 || path[9]==0x2716 && (ply==10 || path[10]==0x5564 && (ply==11 || path[11]==0x7464 && (ply==12 || path[12]==0x2277 && (ply==13 || path[13]==0x1607 && (ply==14 || path[14]==0x0414 && (ply==15 || path[15]==0x7077 && (ply==16 || path[16]==0x0007 && (ply==17 || path[17]==0x6554 && (ply==18))))))))))))))))))

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

int nodeCnt, patCnt, genCnt, evalCnt, qsCnt, leafCnt, captCnt[2], discoCnt, makeCnt, mapCnt; // for statistics

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

unsigned char firstLeap[] = {
  // King                             // Knight
  45, 21, 21, 21, 21, 21, 21, 41,     147,126, 96, 96, 96, 96,122,144,
  27,  0,  0,  0,  0,  0,  0, 15,     130,165, 72, 72, 72, 72,160,118,
  27,  0,  0,  0,  0,  0,  0, 15,     101, 79, 49, 49, 49, 49, 65, 91,
  27,  0,  0,  0,  0,  0,  0, 15,     101, 79, 49, 49, 49, 49, 65, 91,
  27,  0,  0,  0,  0,  0,  0, 15,     101, 79, 49, 49, 49, 49, 65, 91,
  27,  0,  0,  0,  0,  0,  0, 15,     101, 79, 49, 49, 49, 49, 65, 91,
  27,  0,  0,  0,  0,  0,  0, 15,     134,150, 58, 58, 58, 58,155,114,
  33,  9,  9,  9,  9,  9,  9, 37,     138,106, 86, 86, 86, 86,110,141,
  // white Pawn                       // black Pawn
  171,170,170,170,170,170,170,173,      8,  8,  8,  8,  8,  8,  8,  8,
  171,170,170,170,170,170,170,173,    178,175,175,175,175,175,175,176,
  171,170,170,170,170,170,170,173,    178,175,175,175,175,175,175,176,
  171,170,170,170,170,170,170,173,    178,175,175,175,175,175,175,176,
  171,170,170,170,170,170,170,173,    178,175,175,175,175,175,175,176,
  171,170,170,170,170,170,170,173,    178,175,175,175,175,175,175,176,
  171,170,170,170,170,170,170,173,    178,175,175,175,175,175,175,176,
    8,  8,  8,  8,  8,  8,  8,  8,    178,175,175,175,175,175,175,176,
};

signed char leaps[] = { // 0-terminated sets of directions leapers can move in
  16, -16, 1, -1, 15, -15, 17, -17, 0,   // 0  King
 -16, 1, -1, -15, -17, 0,                // 9
  16, -16, -1, 15, -17, 0,               // 15
  16, 1, -1, 15, 17, 0,                  // 21
  16, -16, 1, -15, 17, 0,                // 27
  -16, 1, -15, 0,                        // 33
  -16, -1, -17, 0,                       // 37
  16, -1, 15, 0,                         // 41
  16, 1, 17, 0,                          // 45
  31, -31, 33, -33, 18, -18, 14, -14, 0, // 49 Knight
  -31, -33, 18, -18, 14, -14, 0,         // 58         inner track
  31, -31, 33, -33, -18, 14, 0,          // 65
  31, 33, 18, -18, 14, -14, 0,           // 72
  31, -31, 33, -33, 18, -14, 0,          // 79
  -31, -33, -18, -14, 0,                 // 86         
  31, -33, -18, 14, 0,                   // 91
  31, 33, 18, 14, 0,                     // 96
  -31, 33, 18, -14, 0,                   // 101
  -31, -33, -14, 0,                      // 106
  -31, -33, -18, 0,                      // 110
  -33, -18, 14, 0,                       // 114
  31, -18, 14, 0,                        // 118
  31, 33, 14, 0,                         // 122
  31, 33, 18, 0,                         // 126
  33, 18, -14, 0,                        // 130
  -31, 18, -14, 0,                       // 134
  -31, -14, 0,                           // 138        corners
  -33, -18, 0,                           // 141
  31, 14, 0,                             // 144
  33, 18, 0,                             // 147
  -31, -33, 18, -14, 0,                  // 150
  -31, -33, -18, 14, 0,                  // 155
  31, 33, -18, 14, 0,                    // 160
  31, 33, 18, -14, 0,                    // 165
  15, 17, 0,                             // 170, 171 white Pawn
  15, 0,                                 // 173
  -15, -17, 0,                           // 175, 176 black Pawn
  -15, 0,                                // 178
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
  int newThreats, undetectedThreats; // victim sets
  int targets, existingThreats;      // victim sets
  int oldPresence;                   // presence mask
  int moverSet, victimSet, inCheck;  // attackers sets
  int moverMask, victimMask;         // attackers sets with single attacker
} UndoInfo;

// Attack map

#define SZ 50

unsigned int attackersStack[3200]; // for allowing copy-make of attack maps
unsigned int *attackers = attackersStack + 1600;
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

void MovePiece(UndoInfo *u)
{ // generate moves for the piece from the square, and add or remove them from the attack map
  int bit = attacker2bit[u->piece-16];  // addsub is 1 or -1, and determines we set or clear
  if(code[u->piece] & 0200) { // slider
    int dir = firstCapt[u->piece-16];
    int d = steps[dir+14];                    // direction nr (+1)
    do {
      int sqr = neighbor[u->from].d[d-1];   // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[sqr];
      attackers[victim] -= bit;           // set/clear the bit for this attacker
    } while((d = steps[++dir+14]));
    board[u->from] = 0;
    board[u->to] = u->piece;
    Evacuate(u->from);
    dir = firstCapt[u->piece-16];
    d = steps[dir+14];                        // direction nr (+1)
    do {
      int sqr = neighbor[u->to].d[d-1];     // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[sqr];
      attackers[victim] += bit;           // set/clear the bit for this attacker
    } while((d = steps[++dir+14]));
  } else { // leaper
    int dir = firstCapt[u->piece-16];
    int v = steps[dir];
    do {
      int sqr = u->from + v;               // target square
      int victim = board[sqr];             // occupant of that square
      attackers[victim] -= bit;            // set/clear the bit for this attacker
    } while((v = steps[++dir]));
    board[u->from] = 0;
    board[u->to] = u->piece;
    Evacuate(u->from);
    dir = firstCapt[u->piece-16];
    v = steps[dir];
    do {
      int sqr = u->to + v;                 // target square
      int victim = board[sqr];             // occupant of that square
      attackers[victim] += bit;            // set/clear the bit for this attacker
    } while((v = steps[++dir]));
  }
}

void NewAddMoves(UndoInfo *u, int addsub)
{ // generate moves for the piece from the square, and add or remove them from the attack map
  int bit = attacker2bit[u->piece-16]*addsub;  // addsub is 1 or -1, and determines we set or clear
  if(code[u->piece] & 0200) { // slider
    int dir = firstCapt[u->piece-16];
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
  int bit = attacker2bit[piece-16]*addsub;  // addsub is 1 or -1, and determines we set or clear
  if(code[piece] & 0200) { // piece is slider
    int d, dir = firstSlide[slideOffs[piece-16] + sqr];
    while((d = slides[dir++])) {            // direction nr (+1)
      int to = neighbor[sqr].d[d-1];        // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[to];
//      if(victim != COLOR) {                 // ignore edge guards
        attackers[victim] += bit;           // set/clear the bit for this attacker
//if(!noprint) printf("Slider at %d @ %02x by %d @ %02x: kind=%d vKind=%d old=%08x @ %3d %d\n", victim, to, piece, sqr, kind, victimKind, oldAtt, ptr-attackers, msp),fflush(stdout);
//if((PATH) && ply == 4) printf("Slider: %d @ %02x by %d @ %02x a=%016llx dir=%2d d=%2d\n", victim, to, piece, sqr, neighbor[sqr].all, dir-1, d-1),fflush(stdout);
//      }
    }
  } else {
    int v, dir = firstCapt[piece-16];
    while((v = steps[dir++])) {
      int to = sqr + v;                    // target square
      int victim = board[to];              // occupant of that square
//      if(victim) {                         // ignore empty squares
//if((PATH) && ply == 4) printf("Leaper: %d @ %02x by %d @ %02x a=%016llx dir=%2d dv=%2d\n", victim, to, piece, sqr, neighbor[sqr].all, dir-1, v),fflush(stdout);
        attackers[victim] += bit;          // set/clear the bit for this attacker
//      }
    }
  }
}

int OldAddMoves(int piece, int sqr, int addsub)
{ // generate moves for the piece from the square, and add or remove them from the attack map
  int bit = attacker2bit[piece-16]*addsub;  // addsub is 1 or -1, and determines we set or clear
  if(code[piece] & 0200) { // piece is slider
    int d, dir = firstSlide[slideOffs[piece-16] + sqr];
    while((d = slides[dir++])) {            // direction nr (+1)
      int to = neighbor[sqr].d[d-1];        // the directions were offset by 1 to avoid the 0 sentinel
      int victim = board[to];
      if(victim != COLOR) {                 // ignore edge guards
        attackers[victim] += bit;           // set/clear the bit for this attacker
//if(!noprint) printf("Slider at %d @ %02x by %d @ %02x: kind=%d vKind=%d old=%08x @ %3d %d\n", victim, to, piece, sqr, kind, victimKind, oldAtt, ptr-attackers, msp),fflush(stdout);
//if((PATH) && ply == 4) printf("Slider: %d @ %02x by %d @ %02x a=%016llx dir=%2d d=%2d\n", victim, to, piece, sqr, neighbor[sqr].all, dir-1, d-1),fflush(stdout);
      }
    }
  } else {
    int v, dir = firstLeap[slideOffs[piece-16] + sqr];
    while((v = leaps[dir++])) {
      int to = sqr + v;                    // target square
      int victim = board[to];              // occupant of that square
      if(victim) {                         // ignore empty squares
//if((PATH) && ply == 4) printf("Leaper: %d @ %02x by %d @ %02x a=%016llx dir=%2d dv=%2d\n", victim, to, piece, sqr, neighbor[sqr].all, dir-1, v),fflush(stdout);
        attackers[victim] += bit;          // set/clear the bit for this attacker
      }
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

void HalfMapFromScratch(int *attackMap)
{
  int i, first = msp;
//  for(i=16; i<48; i++) attackMap[i] = 0;
//  memset((void*)(attackMap+COLOR-stm), 0, 64);
  memcpy((void*)(attackMap+16), (void*) attackersStack, 64);
noprint++;
  for(i=0; i<16; i++) {
    int sqr = location[i+stm];
    if(sqr != CAPTURED) AddMoves(i+stm, sqr, 1);
  }
  msp = first;
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
  u->newThreats = Discover(u->moverSet & presence[stm], u->from); // discover opponent slider moves
  attackers[u->piece] = attackers[u->victim] - u->moverMask;      // inherit attackers, but remove self-attack
  attackers[u->victim] = u->moverMask;                            // this last attack will be removed with other old moves of mover
  if(attackers[u->piece] & presence[stm])                         // we can be recaptured
    u->newThreats |= victim2bit[u->piece-16];                     // so add mover to the possible retaliation victims
if(PATH) printf("       att=%08x pres=%08x bit=%08x nt=%08x\n", attackers[u->piece], presence[stm], victim2bit[u->piece-16], u->newThreats);
//printf("mover=%08x victim=%08x presence=%08x\n", attackers[u->piece], attackers[u->victim], presence[stm]);
  STATS(makeCnt++;)
}

void UndoPreUpdate(UndoInfo *u)
{
  int sliders = u->moverSet & presence[stm] & SLIDERS;     // opponent slider attackers of evacuated square
  while(sliders) { // slider discovery
    int bit; BSF(sliders);
    int pinner = bit2attacker[bit];
    int src = location[pinner];
    int d = direction[u->from - src];
    int target = neighbor[u->from].d[d-1];
    int mask = 1 << bit;
    if(target != COLOR) {
      attackers[target] -= mask;
    }
    sliders -= mask;
  }
  presence[stm] += u->victimMask;      // and neither do they attack anything
  attackers[u->piece] = u->moverSet;   // inherit, but remove the self-attack
  attackers[u->victim] = u->victimSet; // already captured pieces are no longer attacked!
}

void FullMapRestore(UndoInfo *u)
{
  AddMoves(u->piece, u->to, 1);
  AddMoves(u->piece, u->from, -1);
  UndoPreUpdate(u);
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
}

#define MOVE(F, T) (256*(F) + (T))

int GenNoncapts()
{
  int i, xstm = stm ^ COLOR;

  for(i=0; i<16; i++) {
    int piece = xstm + i;
    int from = location[piece];
    if(from != CAPTURED) {
      int step, dir = firstDir[piece-16];
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
            attackers[victim] -= attacker2bit[piece-16];
          mask |= attacker2bit[piece-16];         // record this attack
//printf("slider %d attacks at %02x\n", piece, sqr);
        }
      } else {                                    // it is a leaper
        if(c & capt[d] && src == sqr + vec[d]) {  // hits us (only when adjacent)
          mask |= attacker2bit[piece-16];         // record this attack
//printf("leaper %d attacks at %02x\n", piece, sqr);
        }
      }
    }
  }
  for(i=8; i<10; i++) { // Knights
    int from = location[WHITE+i];
    if(from != CAPTURED && hippo[sqr-from])
      mask |= attacker2bit[i+WHITE-16];
    from = location[BLACK+i];
    if(from != CAPTURED && hippo[sqr-from])
      mask |= attacker2bit[i+BLACK-16];
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
  if(u->victim) presence[stm] += attacker2bit[u->victim-16], vPresence[stm] += victim2bit[u->victim-16];
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
    u->moverMask  = attacker2bit[u->piece-16];
    board[u->from] = 0;                                             // 'lift' the moved piece
    AddMoves(u->piece, u->from, -1);                                // and remove its moves
    Evacuate(u->from);
    u->newThreats = Discover(u->moverSet & presence[stm], u->from); // discover opponent slider moves
    Discover(u->moverSet & presence[COLOR-stm], u->from);           // and own own slider moves
    // the mover is now consistently gone
    if(!u->victim) u->savedNeighbor = neighbor[u->to], Occupy(u->to);
    attackers[u->piece] = InterceptAttacks(u->to);                  // build attackers set, and remove the blocked sliders from their old targets
    if(attackers[u->piece])                                         // we went to an attacked square
      u->newThreats |= victim2bit[u->piece-16];                     // so add mover to the possible retaliation victims
    AddMoves(u->piece, u->to, 1);
if(PATH) printf("       att=%08x pres=%08x bit=%08x nt=%08x\n", attackers[u->piece], presence[stm], victim2bit[u->piece-16], u->newThreats);
//printf("mover=%08x victim=%08x presence=%08x\n", attackers[u->piece], attackers[u->victim], presence[stm]);
  }

  // update board and piece list
  board[u->to]   = u->piece;
  location[u->piece]  = u->to;
  location[u->victim] = CAPTURED;
  if(u->victim) presence[stm] -= attacker2bit[u->victim-16], vPresence[stm] -= victim2bit[u->victim-16];

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
#define MERGED_A 3*16
  29, 28, 29, 28, 29, 28, 29, 28, 29, 28, 29, 28, 29, 28, 29, 28,
  29, 28, 29, 28, 29, 28, 29, 28, 29, 28, 29, 28, 29, 28, 29, 28,
#define BV_R 5*16
  45, 44, 45, 44, 45, 44, 45, 44, 45, 44, 45, 44, 45, 44, 45, 44,
  45, 44, 45, 44, 45, 44, 45, 44, 45, 44, 45, 44, 45, 44, 45, 44,
#define WA_R 7*16
  16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23,
  24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31,
#define BA_R 9*16
  32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39,
  40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47,
#define WV_MP 11*16
  27, 26, 27, 26, 27, 26, 27, 26, 27, 26, 27, 26, 27, 26, 27, 26,
  25, 24, 25, 24, 25, 24, 25, 24, 25, 24, 25, 24, 25, 24, 25, 24,
#define WA_MP 13*16
  16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23,
  16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23,
#define BV_MP 15*16
  43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42,
  41, 40, 41, 40, 41, 40, 41, 40, 41, 40, 41, 40, 41, 40, 41, 40,
#define BA_MP 17*16
  24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31,
  24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31,

  16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23,
  24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31,
  0, 0, 1, 1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,
  8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,

  10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11,
   8,  9,  8,  9,  8,  9,  8,  9,  8,  9,  8,  9,  8,  9,  8,  9,

  0, 0, 1, 1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,
  0, 0, 1, 1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,
};

char Qvictim[] = {
  46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30,
  46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30, 46, 30,
};

#define MERGE(A, OP, B) capts = (A & myPresence) OP (B & myPresence)

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

int Search(UndoInfo *u) // pass all parameters in a struct
{
  UndoInfo undo;
  int pvMove;
  int curMove, noncapts = 10000;
  undo.pv = pvPtr;
  undo.first = msp;
  nodeCnt++;
  *pvPtr++ = 0; // empty PV
  undo.parentAlpha = u->alpha;
  undo.inCheck = attackers[stm+15] & presence[COLOR-stm];
  undo.oldMap  = attackers;

if(PATH) printf("%2d    Search(%d,%d,%d) eval=%d nb=%016llx check=%08x\n", ply, u->alpha, u->beta, u->depth, u->pstEval, neighbor[0].all, undo.inCheck),PrintBoard(),PrintMaps(attackers), fflush(stdout);
  // QS / stand pat
  if(u->depth <= 0) {
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
  undo.existingThreats  = 0;               // at this point we have not identified any tattacks on our pieces
  undo.undetectedThreats = vPresence[stm]; // but any piece of ours that is not yet captured could be under attack
  stm ^= COLOR;

  // generate moves
  if(followPV >= 0) {              // first branch
    pvMove = pv[followPV++];       // get the move
    if(pvMove) {
      undo.beta = -undo.parentAlpha;
      undo.searched++;
      MakeMove(pvMove, &undo);
      undo.parentAlpha = -Search(&undo);
      UnMake(&undo);
    } else followPV = -1;
  } else pvMove = 0;

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

  if(u->depth <= 0) goto cutoff;
  GenNoncapts(); genCnt++;
  undo.targets = vPresence[COLOR - stm];

  // move loop
  for(curMove = undo.first; curMove < msp; curMove++) {
    int score;
    unsigned int move;

    move = moveStack[curMove];
//if(PATH) {int i; for(i=undo.first; i<msp; i++) printf("      %d. %s\n", i, Move2text(moveStack[i])); }

    if(move == pvMove && !followPV) continue; // duplicat moves

    // recursion
    undo.beta = -undo.parentAlpha;
if(PATH) printf("%2d:%d %3d. %08x %s               %016llx\n", ply, u->depth, curMove, move, Move2text(move), neighbor[0].all), fflush(stdout);
    score = MakeMove(move, &undo); // rejected moves get their score here
//if(PATH) printf("      %s: score=%d\n", Move2text(move), score);
    if(score < -INF) break;        // move is futile, and so will be all others
    if(score > INF) { // move successfully made
      undo.searched++;
ply--;
if(PATH) printf("%2d:%d %3d. %08x %s               %016llx\n", ply, u->depth, curMove, move, Move2text(move), neighbor[0].all), fflush(stdout);
ply++;
      score = -Search(&undo);
      UnMake(&undo);
    }
if(PATH) printf("%2d:%d %3d. %08x %s %5d %5d n %016llx\n", ply, u->depth, curMove, move, Move2text(move), score, undo.parentAlpha, neighbor[0].all), fflush(stdout);

    // minimaxing
    if(score > undo.parentAlpha) {
      int *p;
      if(score >= u->beta) {
        undo.parentAlpha = u->beta;
        break;
      }
      undo.parentAlpha = score;
      p = pvPtr; pvPtr = undo.pv; // pop old PV
      *pvPtr++ = move;            // push new PV, starting with this move
      while((*pvPtr++ = *p++)) {} // and append child PV
    }
  }
 cutoff:
if(PATH) printf("%2d    done\n", ply),fflush(stdout);
  stm ^= COLOR;
 abort:
  leafCnt += !undo.searched;
  msp = undo.first;    // pop move list
  pvPtr = undo.pv;     // pop PV (but remains above stack top)
  return undo.parentAlpha;
}

unsigned char gapOffset[] = { // offset in nonFutile[] array below, indexed by (gap/8)
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,10,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,20,24,
 24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
 24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,26,32,32,32,32,32,32,32,32,
};

int nonFutile[] = { // victim sets of non-futile victims (indexed by (gap&7), with offset)
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, //  0 all pieces
  0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, //  8 only non-Pawns
  0x000F000F, 0x000F000F, 0x000F000F, 0x000F000F, 0x000F000F, 0x000F000F, 0x000F000F, 0x000F000F, // 16 only majors
  0x00030003, 0x00030003, 0x00030003, 0x00030003, 0x00030003, 0x00030003, 0x00030003, 0x00030003, // 24 only QK
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 32 nothing
};

int value[] = { // indexed by piece-15!
  0, PVAL, PVAL, PVAL, PVAL, PVAL, PVAL, PVAL, PVAL, BVAL, BVAL, BVAL, BVAL, RVAL, RVAL, QVAL,
  0, PVAL, PVAL, PVAL, PVAL, PVAL, PVAL, PVAL, PVAL, BVAL, BVAL, BVAL, BVAL, RVAL, RVAL, QVAL,
  0,
};

int SearchCapture(UndoInfo *u)
{ // make / search / unmake and score minimaxing all in one
  int move;
  int score;
  int victimBit = victim2bit[u->victim-16];
  u->victimMask = attacker2bit[u->victim-16];               // save for unmake
if(PATH) printf("%2d      %s check=%08x mask=%08x\n", ply, Move2text(256*location[u->piece] + location[u->victim]), u->inCheck, u->victimMask);
  if(u->inCheck && u->inCheck & ~u->victimMask              // we were in check, and did not capture checker,
                && u->piece != COLOR + 15 - stm) return 0;  // nor moved the King => move is illegal

  // decode move
  u->to = location[u->victim];

  // detect futility at earliest opportunity
  u->pstEval = u->oldEval + PST(u->victim, u->to);
//if(PATH) printf("      search, lazyEval=%d, alpha=%d\n",u->pstEval, u->parentAlpha),fflush(stdout);\
  if(u->depth <= 0 && u->pstEval < u->parentAlpha - MARGIN) return 1; // futility (child will stand pat)\}

  // finish decoding move and updating evaluation
  u->from = location[u->piece];
  u->pstEval = -(u->pstEval + PST(u->piece, u->to) - PST(u->piece, u->from));

  // update hash key, and possibly abort on repetition
  u->hashKey =   u->oldKey  ^ KEY(u->piece, u->to) ^ KEY(u->piece, u->from) ^ KEY(u->victim, u->to);
  // if(REPEAT) score = 0; else

  memcpy((void*) (attackers + WHITE + SZ), (void*) (attackers + WHITE), 128);
  attackers += SZ;
  PreUpdate(u);

  if(attackers[COLOR+15-stm] & presence[stm]) { attackers -= SZ; presence[stm] = u->oldPresence; return 0; } // illegal move, abort

  if(u->depth <= 1 SPOIL) { // overkill pruning: test if child will be futile
    int todo, worseThreats, futMask, index;
    int oppoPresence = presence[stm];
    int gap = u->alpha - MARGIN - 10 - u->pstEval;     // futility gap child will have to bridge with a capture
if(PATH) printf("       vbit=%08x\n", victimBit);
    if(u->newThreats & victimBit) {                    // Yeghh!: victim could have been pin anchor, and collect attack meant for piece in discovery
      u->newThreats |= victim2bit[u->piece-16];        // and displace it to the mover
    }
    u->newThreats &= vPresence[COLOR-stm];
    index = gap >> 3; index = (gap > 0 ? index : 0); index = (gap > 1023 ? 127 : index);
    futMask = nonFutile[gapOffset[index] + (gap & 3)]; // victim set of non-futiles
    worseThreats = u->newThreats - 1 & ~u->newThreats; // all more valuable pieces than what the move attacks
    worseThreats &= futMask;                           // remove the futile targets
    todo = u->existingThreats | u->undetectedThreats;  // set of potential victims
    todo &= worseThreats;                              // only keep the non-futile more valuable than the newly attacked
    todo &= ~victim2bit[u->piece-16];                  // remove the moved piece; attacks on it now hit thin air
if(PATH) printf("       new=%08x existing=%08x worse=%08x\n       undetected=%08x fut=%08x gap=%d\n", u->newThreats, u->existingThreats, worseThreats, u->undetectedThreats, futMask, gap);
    while(todo) {
      int bit; BSF(todo);                              // get next (MVV order)
      int victim = COLOR-1 - bit;                      // calculate piece number
      int bitMask = 1 << bit;                          // bit representing this victim in victim set
      int attacks = attackers[victim-SZ];              // pre-existing attacks on it (from old attack map)
if(PATH) printf("       bit=%d victim=%d attacks=%08x mask=%08x todo=%08x\n", bit, victim, attacks, bitMask, todo);
      u->undetectedThreats &= ~bitMask;                // this threat will now be revealed
      if(attacks & u->oldPresence) {                   // it was indeed a threat
        u->existingThreats |= bitMask;                 // record this fact (so sibbling nodes can use it)
        if(attacks & oppoPresence) break;              // the threat still exists => MVV identified
      }
      todo -= bitMask;                                 // this one done; remove to extract next
    }
if(PATH) printf("       new=%08x existing=%08x worse=%08x\n       undetected=%08x todo=%08x\n", u->newThreats, u->existingThreats, worseThreats, u->undetectedThreats, todo);
    todo |= u->undetectedThreats;                      // unconfirmed threats must also be tried
if(PATH) printf("       new=%08x existing=%08x worse=%08x\n       undetected=%08x todo=%08x\n", u->newThreats, u->existingThreats, worseThreats, u->undetectedThreats, todo);
   todo &= ~victim2bit[u->piece-16];                  // except those on the mover (which would be in newThreats)
if(PATH) printf("       new=%08x existing=%08x worse=%08x\n       undetected=%08x todo=%08x\n", u->newThreats, u->existingThreats, worseThreats, u->undetectedThreats, todo);
    todo |= u->newThreats;                             // add the newly created threats
if(PATH) printf("       new=%08x existing=%08x worse=%08x\n       undetected=%08x todo=%08x\n", u->newThreats, u->existingThreats, worseThreats, u->undetectedThreats, todo);
    todo &= futMask;                                   // and remove the futile victims it might have contained
if(PATH) printf("       new=%08x existing=%08x worse=%08x\n       undetected=%08x todo=%08x\n", u->newThreats, u->existingThreats, worseThreats, u->undetectedThreats, todo);
    if(!todo && gap > 0) {                             // opponent has no non-futile move
      u-> parentAlpha = -u->alpha;                     // fail-high score (-u->alpha is parent's beta)
      attackers -= SZ; presence[stm] = u->oldPresence; // undo updates
      u->searched++; nodeCnt++; qsCnt += (u->depth <= 0);
      return 1;                                        // and abort move ('overkill pruning')
    }
    u->targets = todo;                                 // pass set of victims to capture to child

  } else u->targets = vPresence[COLOR - stm]; // at higher depth we let child try all present pieces

  // committed to move
  move = MOVE(u->from, u->to);
  u->searched++;
  {

if(PATH) { printf("%2d:%d %3d. %08x %s (%d)\n", ply, u->depth+1, 0, move, Move2text(move), stm), fflush(stdout);}
    // finish map update and apply move to board
//    AddMoves(u->piece, u->from, -1);

//    board[u->from] = 0;
//    board[u->to]   = u->piece;
    location[u->piece]  = u->to;
    location[u->victim] = CAPTURED;
    vPresence[stm] -= victimBit;
//    Evacuate(u->from);

    MovePiece(u);

    STATS(int cnt = discoCnt;)
    Discover(u->moverSet & presence[COLOR-stm], u->from);
    STATS(discoCnt = cnt;)
//    AddMoves(u->piece, u->to, 1);

    path[ply++] = move; captCnt[0]++;

//    attackers += SZ;
//    presence[stm] -= attacker2bit[u->victim-16];
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
    vPresence[stm] += victimBit;
    Reoccupy(u->from);

//if(PATH) printf("%2d    Restore(%d,%d)\n", ply, u->first, u->piece),fflush(stdout);
//    FullMapRestore(u);
//    VerifyMap("after unmake");
    attackers -= SZ;


    ply--;
//if(PATH) printf("%2d    Reoccupy(%02x)\n", ply, u->from),fflush(stdout);
  }
if(PATH) { printf("%2d:%d %3d. %08x %s %5d %5d   %016llx\n", ply, u->depth+1, 0, move, Move2text(move), score, u->parentAlpha, neighbor[0].all), fflush(stdout);}

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
  nodeCnt = patCnt = evalCnt = genCnt = qsCnt = leafCnt = captCnt[0] = captCnt[1] = discoCnt = makeCnt = mapCnt = 0;
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
  PrintMaps();
  t = clock();
  SearchRoot(&root, 10);
  t = clock() - t;
  printf("t = %6.3f sec\n", t * (1./CLOCKS_PER_SEC));
  printf("%10d nodes (%3.1f Mnps)\n%10d QS (%3.1f%%)\n", nodeCnt, nodeCnt*1e-6*CLOCKS_PER_SEC/t, qsCnt, qsCnt*100./nodeCnt);
  printf("%10d evals (%3.1f%%)\n%10d stand-pats (%3.1f%%)\n", evalCnt, evalCnt*100./qsCnt, patCnt, patCnt*100./qsCnt);
  printf("%10d leaves (%3.1f%%)\n", leafCnt, leafCnt*100./qsCnt);
  printf("%10d move gens\n", genCnt);
  printf("%10d map gens\n", mapCnt);
  printf("%4.2f pins/move\n", discoCnt/(double)makeCnt);
  printf("captures: %3.1f%%\n", captCnt[0]*100./(captCnt[0] + captCnt[1]));
  return 0;
}
